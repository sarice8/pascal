/*

  jit.c

  This converts the intermediate t-code produced by my Pascal compiler
  into x86 machine code, and executes it.

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
//  -- for mmap, mprotect
#include <errno.h>

#include <string>
#include <stack>
#include <vector>


// This is where we get the definitions for tCode,
// which originate in SSL.
// Note, some of the symbols conflict with std headers,
// which I could resolve  by not using preprocessor symbols.
// Meanwhile as long as I include this after the std headers, it's ok.
#include "pascal.h"


typedef void (*funcptr)();


void parseArgs( int argc, char* argv[] );
void usage( int status );
void loadTCodeAndAllocNativeCode();
void allocNativeCode();
void protectNativeCode();
void outB( char c );
void outI( int i );
void outL( long l );
void generateCode();
void tCodeNotImplemented( int opcode );
void executeCode();
void fatal( const char* msg, ... );
void toDo( const char* msg, ... );


char* filename = 0;

int* tCode = 0;
int  tCodeLen = 0;


// Runtime data memory, for globals.
// Right now we don't actually know how much we need!
//
// The call stack will not live in here.  It will be part of the
// same call stack used by this calling program.
//
// Static data is allocated near code, so we can use rip-relative addressing.
//
#define dataSize 100000
char* data = 0;



char* nativeCode = 0;
size_t nativeCodeLen = 10000;
size_t nativeCodeAllocSize = 0;
size_t PAGE_SIZE = 8192;
char* nativePc = 0;


// Offset on call stack from rbp to the first parameter
#define FRAME_PARAMS_OFFSET (2 * sizeof(void*))


// Operand stack
//
typedef enum {
  jit_Operand_Kind_Illegal = 0,
  jit_Operand_Kind_None,

  // operand is the value of a variable
  jit_Operand_Kind_GlobalB,
  jit_Operand_Kind_GlobalI,
  jit_Operand_Kind_GlobalP,
  jit_Operand_Kind_LocalB,
  jit_Operand_Kind_LocalI,
  jit_Operand_Kind_LocalP,
  jit_Operand_Kind_ParamB,
  jit_Operand_Kind_ParamI,
  jit_Operand_Kind_ParamP,
  // Actual* won't occur on expression stack, but is used internally to help assign to actuals
  jit_Operand_Kind_ActualB,
  jit_Operand_Kind_ActualI,
  jit_Operand_Kind_ActualP,

  // operand is the address of a variable
  jit_Operand_Kind_Addr_Global,
  jit_Operand_Kind_Addr_Local,
  jit_Operand_Kind_Addr_Param,
  jit_Operand_Kind_Addr_Actual,

  jit_Operand_Kind_ConstI,

  // operand is a value in a register.
  // We either have one byte valid, 4 bytes valid, or 8 bytes valid.
  // In the case of 4 bytes, we are guaranteed that the higher 4 bytes
  // of the register are 0 (note: not sign extended), thanks to x86-64 rules.
  // This is not the case if the valid size is one byte.
  jit_Operand_Kind_RegB,
  jit_Operand_Kind_RegI,
  jit_Operand_Kind_RegP,

  // operand is a value pointed to by a register.
  // For now, these are just used by emitMov, and won't appear on expression stack.
  jit_Operand_Kind_RegP_DerefB,
  jit_Operand_Kind_RegP_DerefI,
  jit_Operand_Kind_RegP_DerefP

} jitOperandKind;


constexpr int
KindPair( jitOperandKind x, jitOperandKind y ) {
  return ( ( int )x ) << 8 | ( int )y;
}


class Register
{
public:
  // The native id is 4 bits.
  // The upper bit gives access to registers r8-r15.
  // If needed, this bit goes in the REX prefix.  See x86=64 docs for more details.
  //
  // CalleeSave means that callers are promised that their value will remain intact.
  // So callees must preserve it if they will use it.
  //
  Register( const std::string& name, int nativeId, bool calleeSave )
    : _name( name ), _nativeId( nativeId ), _calleeSave( calleeSave )
    {}

  std::string _name = "";
  int _nativeId = 0;
  bool _calleeSave = false;
  bool _inUse = false;
};

// Note: calling convention for "x86-64 System V" is:
//   arguments that are classified as "integer/pointer" rather than "memory"
//   are passed in registers, left to right:  rdi, rsi, rdx, rcx, r8, r9.
//   Floating point args are passed in xmm0 - xmm7.
//   See https://stackoverflow.com/questions/2535989/what-are-the-calling-conventions-for-unix-linux-system-calls-and-user-space-f
// TO DO: Am I really seeing this in my linux programs?
//   If not, make sure I'm compiling for 64-bit.
// Windows x64 calling convention is different.  Ugh.
// These might depend on an off-by-default fastcall pragma or compiler switch.
// BUT, must be a standard for library calls and extern methods!

// Microsoft describes Windows x64 calling convention ("__fastcall"):
// https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170&viewFallbackFrom=vs-2017


Register* regRax = new Register( "rax", 0, false );
Register* regRcx = new Register( "rcx", 1, false );
Register* regRdx = new Register( "rdx", 2, false ); 
Register* regRbx = new Register( "rbx", 3, true );
Register* regRsp = new Register( "rsp", 4, true );
Register* regRbp = new Register( "rbp", 5, true );
Register* regRsi = new Register( "rsi", 6, false );   // "Function arg #2 in 64-bit linux"
Register* regRdi = new Register( "rdi", 7, false );   // "Function arg #1 in 64-bit linux"
Register* regR8  = new Register( "r8", 8, false );
Register* regR9  = new Register( "r9", 9, false );
Register* regR10 = new Register( "r10", 10, false );
Register* regR11 = new Register( "r11", 11, false );
Register* regR12 = new Register( "r12", 12, true );
Register* regR13 = new Register( "r13", 13, true );
Register* regR14 = new Register( "r14", 14, true );
Register* regR15 = new Register( "r15", 15, true );


// General purpose registers, in the order we prefer to allocate them.
//
Register* registers[] = {
  regRax,
  regRcx,
  regRbx,
  regRdx,
  regR8,
  regR9,
  regR10,
  regR11,
  regR12,
  regR13,
  regR14,
  regR15
};
int numRegisters = sizeof(registers) / sizeof(Register*); 

int regIdLowBits( int nativeId ) { return nativeId & 0x7; }
int regIdHighBit( int nativeId ) { return nativeId >> 3; }


class Operand
{
public:
  Operand( jitOperandKind k, int val )
    : _kind( k ), _value( val )
  {}

  Operand( jitOperandKind k, Register* r )
    : _kind( k ), _reg( r )
  {}

  Operand( jitOperandKind k, Register* r, int val )
    : _kind( k ), _reg( r ), _value( val )
  {}

  bool isReg() const { return _kind >= jit_Operand_Kind_RegB &&
                              _kind <= jit_Operand_Kind_RegP; }
  bool isVar() const { return _kind >= jit_Operand_Kind_GlobalB &&
                              _kind <= jit_Operand_Kind_ActualP; }
  bool isAddrOfVar() const { return _kind >= jit_Operand_Kind_Addr_Global &&
                                    _kind <= jit_Operand_Kind_Addr_Actual; }
  bool isVarOrAddrOfVar() const { return isVar() || isAddrOfVar(); }
  bool isDeref() const { return _kind >= jit_Operand_Kind_RegP_DerefB &&
                                _kind <= jit_Operand_Kind_RegP_DerefP; }
  bool isMem() const { return isVar() || isAddrOfVar() || isDeref(); }
  bool isConst() const { return _kind == jit_Operand_Kind_ConstI; }
  int  size() const;  // valid size of the operand value in bytes (1, 4, or 8)
  void release();     // don't need this register anymore (if any)

  // DATA

  jitOperandKind _kind = jit_Operand_Kind_Illegal;
  int _value = 0;         // for a var: the var offset
                         // for a const: the const value
  Register* _reg = nullptr;    // for jit_Operand_Kind_Reg*
};


// Don't need this register anymore (if any)
//
void
Operand::release()
{
  if ( _reg ) {
    _reg->_inUse = false;
  }
}


std::stack<Operand> operandStack;

void swap( Operand& x, Operand& y );
void operandIntoReg( Operand& x );
void operandIntoRegCommutative( Operand& x, Operand& y );
void operandKindAddrIntoReg( Operand& x );
void operandNotMem( Operand& x );
void operandDeref( Operand& x, int valueSize );
Register* allocateReg();


// x86 operations
//
void emitAdd( const Operand& x, const Operand& y );
void emitSub( const Operand& x, const Operand& y );
void emitMov( const Operand& x, const Operand& y );

void emitRex( bool overrideWidth, const Register* operandReg, const Register* baseReg );
void emitModRMMem( const Register* operandReg, const Register* baseReg, int offset );
void emitModRM_OpcRMMem( int opcodeExtension, const Register* baseReg, int offset );
void emitModRM_OpcRM( int opcodeExtension, const Register* rm );
void emitModRM_RegReg( const Register* opcodeReg, const Register* rm );
void emitModRMMemRipRelative( const Register* operandReg, void* globalPtr, int numAdditionalInstrBytes );
void emitModRM_OpcRMMemRipRelative( int opcodeExtension, void* globalPtr, int numAdditionalInstrBytes );


int
main( int argc, char* argv[] )
{
  parseArgs( argc, argv );
  loadTCodeAndAllocNativeCode();
  generateCode();
  protectNativeCode();
  executeCode();
}

void
parseArgs( int argc, char* argv[] )
{
  int arg = 1;
  while ( arg < argc ) {
    if ( strcmp( argv[arg], "-h" ) == 0 ) {
      usage( 0 );
    } else if ( argv[arg][0] == '-' ) {
      usage( 1 );
    } else {
      break;
    }
    ++arg;
  }
  if ( arg != argc-1 ) {
    usage( 1 );
  }
  filename = argv[arg];
}


void
usage( int status )
{
  printf( "Usage:   jit <file>\n" );
  exit( status );
}


// ----------------------------------------------------------------------------

// The general approach is:
// Operands such as constants and variable references are pushed onto the operand stack,
// which is a jit-time data structure.  Code for these is not generated immediately.
// When an operation is seen, the operands are taken from the operand stack,
// and the appropriate code is generated for that operation with those operands.
// Results of an operation may be held in a register, which is pushed back onto
// the operand stack.
// 
// I'll try to simplify constants as I go.
// I might not be able to avoid generation of dead code, but at least I can
// jump around it, by turning conditional jumps into unconditional jumps.
//

void
generateCode()
{
  // Fold constants? May want to turn off during some unit testing.
  bool doConst = true;

  int jitSp = 0;

  int* tCodePc = tCode;
  int* tCodeEnd = tCode + tCodeLen;

  for ( tCodePc = tCode; tCodePc < tCodeEnd; ) {
    switch ( *tCodePc++ ) {

      case tPushGlobalI : {
          operandStack.emplace( jit_Operand_Kind_GlobalI, *tCodePc++ );
        }
        break;
      case tPushGlobalB : {
          operandStack.emplace( jit_Operand_Kind_GlobalB, *tCodePc++ );
        }
        break;
      case tPushGlobalP : {
          operandStack.emplace( jit_Operand_Kind_GlobalP, *tCodePc++ );
        }
        break;
      case tPushLocalI : {
          operandStack.emplace( jit_Operand_Kind_LocalI, *tCodePc++ );
        }
        break;
      case tPushLocalB : {
          operandStack.emplace( jit_Operand_Kind_LocalB, *tCodePc++ );
        }
        break;
      case tPushLocalP : {
          operandStack.emplace( jit_Operand_Kind_LocalP, *tCodePc++ );
        }
        break;
      case tPushParamI : {
          operandStack.emplace( jit_Operand_Kind_ParamI, *tCodePc++ );
        }
        break;
      case tPushParamB : {
          operandStack.emplace( jit_Operand_Kind_ParamB, *tCodePc++ );
        }
        break;
      case tPushParamP : {
          operandStack.emplace( jit_Operand_Kind_ParamP, *tCodePc++ );
        }
        break;
      case tPushConstI : {
          operandStack.emplace( jit_Operand_Kind_ConstI, *tCodePc++ );
        }
        break;
      case tPushAddrGlobal : {
          operandStack.emplace( jit_Operand_Kind_Addr_Global, *tCodePc++ );
        }
        break;
      case tPushAddrLocal : {
          operandStack.emplace( jit_Operand_Kind_Addr_Local, *tCodePc++ );
        }
        break;
      case tPushAddrParam : {
          operandStack.emplace( jit_Operand_Kind_Addr_Param, *tCodePc++ );
        }
        break;
      case tPushAddrActual : {
          operandStack.emplace( jit_Operand_Kind_Addr_Actual, *tCodePc++ );
        }
        break;
      case tFetchI : {
          Operand x = operandStack.top();   operandStack.pop();
          // x is a pointer to a value of size 4.
          // Make x refer to the pointed-to value
          operandDeref( x, 4 );
          // If I allow jit_Operand_Kind_RegP_Deref* on the operand stack,
          // then we could just push x and be done.
          // The actual retrieval of the value can be included in the next operation.
          // But, for now, we only handle that operand kind in emitMov.
          // So we have to retrieve it into a register now.
          Operand result( jit_Operand_Kind_RegI, allocateReg() );
          emitMov( result, x );
          operandStack.push( result );
          x.release();
        }
        break;
      case tFetchB : {
          Operand x = operandStack.top();   operandStack.pop();
          // x is a pointer to a value of size 1.
          // Make x refer to the pointed-to value
          operandDeref( x, 1 );
          // If I allow jit_Operand_Kind_RegP_Deref* on the operand stack,
          // then we could just push x and be done.
          // The actual retrieval of the value can be included in the next operation.
          // But, for now, we only handle that operand kind in emitMov.
          // So we have to retrieve it into a register now.
          Operand result( jit_Operand_Kind_RegB, allocateReg() );
          emitMov( result, x );
          operandStack.push( result );
          x.release();
        }
        break;
      case tFetchP : {
          Operand x = operandStack.top();   operandStack.pop();
          // x is a pointer to a value of size 8.
          // Make x refer to the pointed-to value
          operandDeref( x, 8 );
          // If I allow jit_Operand_Kind_RegP_Deref* on the operand stack,
          // then we could just push x and be done.
          // The actual retrieval of the value can be included in the next operation.
          // But, for now, we only handle that operand kind in emitMov.
          // So we have to retrieve it into a register now.
          Operand result( jit_Operand_Kind_RegP, allocateReg() );
          emitMov( result, x );
          operandStack.push( result );
          x.release();
        }
        break;
      case tAssignI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // x is a pointer to a value of size 4.
          // Make x refer to the pointed-to value (still usable as target of mov)
          operandDeref( x, 4 );
          // x will be a memory reference, so y must not also be in memory.
          operandNotMem( y );
          emitMov( x, y );
          x.release();
          y.release();
        }
        break;
      case tAssignB : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // x is a pointer to a value of size 1.
          // Make x refer to the pointed-to value (still usable as target of mov)
          operandDeref( x, 1 );
          // x will be a memory reference, so y must not also be in memory.
          operandNotMem( y );
          emitMov( x, y );
          x.release();
          y.release();
        }
        break;
      case tAssignP : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // x is a pointer to a value of size 8.
          // Make x refer to the pointed-to value (still usable as target of mov)
          operandDeref( x, 8 );
          // x will be a memory reference, so y must not also be in memory.
          operandNotMem( y );
          emitMov( x, y );
          x.release();
          y.release();
        }
        break;
      case tCopy : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          tCodePc++;
          tCodeNotImplemented( tCodePc[-1] );
          x.release();
          y.release();
        }
        break;
      case tIncI : {
          tCodeNotImplemented( tCodePc[-1] );
        }
        break;
      case tDecI : {
          tCodeNotImplemented( tCodePc[-1] );
        }
        break;
      case tMultI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace( x._kind, x._value * y._value );
          } else {
            if ( x.isConst() ) {
              swap( x, y );
            }
            operandIntoReg( x );
            tCodeNotImplemented( tCodePc[-1] );
            operandStack.push( x );
          }
          y.release();
        }
        break;
      case tDivI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tAddPI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          if ( doConst && x.isAddrOfVar() && y.isConst() ) {
            operandStack.emplace( x._kind, x._value + y._value );
          } else {
            operandKindAddrIntoReg( x );
            operandKindAddrIntoReg( y );
            operandIntoReg( x );
            // TO DO: something to indicate that we want y sign-extended to ptr,
            //    or that we really do want to produce a 64-bit result.
            //    Maybe it's enough that x will already be a regP.
            emitAdd( x, y );
            operandStack.push( x );
          }
          y.release();
        }
        break;
      case tAddI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace( x._kind, x._value + y._value );
          } else {
            operandKindAddrIntoReg( x );
            operandKindAddrIntoReg( y );
            operandIntoRegCommutative( x, y );
            emitAdd( x, y );
            operandStack.push( x );
          }
          y.release();
        }
        break;
      case tSubI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace( x._kind, x._value - y._value );
          } else {
            operandKindAddrIntoReg( x );
            operandKindAddrIntoReg( y );
            operandIntoReg( x );
            tCodeNotImplemented( tCodePc[-1] );
            operandStack.push( x );
          }
          y.release();
        }
        break;
      case tNegI : {
          tCodeNotImplemented( tCodePc[-1] );
        }
        break;
      case tNot : {
          tCodeNotImplemented( tCodePc[-1] );
        }
        break;
      case tAnd : {
          // TO DO: don't I need to implement short-circuit and / or somewhere?
          //   I imagine needs to be in higher level front end?
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tOr : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tEqualI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tNotEqualI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tGreaterI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tLessI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tGreaterEqualI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tLessEqualI : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tEqualP : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tNotEqualP : {
          Operand y = operandStack.top();   operandStack.pop();
          Operand x = operandStack.top();   operandStack.pop();
          // The following is dummy code that all needs to be replaced.
          operandIntoReg( x );
          tCodeNotImplemented( tCodePc[-1] );
          operandStack.push( x );
          y.release();
        }
        break;
      case tAllocActuals : {
          int size = *tCodePc++;
          emitSub( Operand( jit_Operand_Kind_RegP, regRsp ),
                   Operand( jit_Operand_Kind_ConstI, size ) );
        }
        break;
      case tFreeActuals : {
          int size = *tCodePc++;
          emitAdd( Operand( jit_Operand_Kind_RegP, regRsp ),
                   Operand( jit_Operand_Kind_ConstI, size ) );
        }
        break;
      case tCall : {
          tCodeNotImplemented( tCodePc[-1] );
          tCodePc++;
        }
        break;
      case tReturn : {
          tCodeNotImplemented( tCodePc[-1] );
        }
        break;
      case tEnter : {
          tCodeNotImplemented( tCodePc[-1] );
          // TO DO: remember the address of the local-space value,
          //        so jit can bump it for tempraries too.
          //        We'll need to keep it a 32-bit offset regardless of current value.
          tCodePc++;
        }
        break;
      case tJump : {
          tCodeNotImplemented( tCodePc[-1] );
          tCodePc++;
        }
        break;
      case tJumpTrue : {
          Operand x = operandStack.top();   operandStack.pop();
          tCodePc++;
          tCodeNotImplemented( tCodePc[-1] );
          x.release();
        }
        break;
      case tJumpFalse : {
          Operand x = operandStack.top();   operandStack.pop();
          tCodePc++;
          tCodeNotImplemented( tCodePc[-1] );
          x.release();
        }
        break;
      case tWriteI : {
          Operand x = operandStack.top();   operandStack.pop();
          tCodeNotImplemented( tCodePc[-1] );
          x.release();
        }
        break;
      case tWriteBool : {
          Operand x = operandStack.top();   operandStack.pop();
          tCodeNotImplemented( tCodePc[-1] );
          x.release();
        }
        break;
      case tWriteStr : {
          Operand x = operandStack.top();   operandStack.pop();
          tCodeNotImplemented( tCodePc[-1] );
          x.release();
        }
        break;
      case tWriteP : {
          Operand x = operandStack.top();   operandStack.pop();
          tCodeNotImplemented( tCodePc[-1] );
          x.release();
        }
        break;
      case tWriteCR : {
          tCodeNotImplemented( tCodePc[-1] );
        }
        break;

      default:
        --tCodePc;
        fatal( "bad instruction %d\n", *tCodePc );
    }
  }
}


// ----------------------------------------------------------------------------

// Provide a register.
// If all are in use, one will be made available by dumping its value
// into a temporary.
//
Register*
allocateReg()
{
  for ( Register* reg : registers ) {
    if ( !reg->_inUse ) {
      reg->_inUse = true;
      return reg;
    }
  }  

  fatal( "TO DO: allocReg dump a reg into temporary\n" );
  return nullptr;
}


// Returns the byte size of the value in the operand.
// Possible values are 1, 4, 8.
//
int
Operand::size() const
{
  switch ( _kind ) {
    case jit_Operand_Kind_GlobalB:
    case jit_Operand_Kind_LocalB:
    case jit_Operand_Kind_ParamB:
    case jit_Operand_Kind_RegB:
      return 1;

    case jit_Operand_Kind_GlobalI:
    case jit_Operand_Kind_LocalI:
    case jit_Operand_Kind_ParamI:
    case jit_Operand_Kind_ConstI:
    case jit_Operand_Kind_RegI:
      return 4;

    case jit_Operand_Kind_GlobalP:
    case jit_Operand_Kind_LocalP:
    case jit_Operand_Kind_ParamP:
    case jit_Operand_Kind_Addr_Global:
    case jit_Operand_Kind_Addr_Local:
    case jit_Operand_Kind_Addr_Param:
    case jit_Operand_Kind_Addr_Actual:
    case jit_Operand_Kind_RegP:
      return 8;

    default:
      fatal( "Operand::size() - unexpected operand kind %d\n", (int) _kind );
  }
  return 1;  // won't reach here
}


// Force (at least) one of the operands into a register.
// On return, x will be in a register.
// This is for a commutative operation, so x and y may be swapped.
//
void
operandIntoRegCommutative( Operand& x, Operand& y )
{
  if ( x.isReg() ) {
    return;
  }
  if ( y.isReg() ) {
    swap( x, y );
    return;
  }
  // we'll prefer to put a variable into a register
  if ( !x.isVarOrAddrOfVar() && y.isVarOrAddrOfVar() ) {
    swap( x, y );
  }
  operandIntoReg( x );
}


// Force the operand into a register, if it isn't already.
//
void
operandIntoReg( Operand& x )
{
  if ( x.isReg() ) {
    return;
  }

  // The size of the register will depend on the value we're moving into it
  jitOperandKind regKind;
  switch ( x.size() ) {
    case 1:
      regKind = jit_Operand_Kind_RegB;
      break;
    case 4:
      regKind = jit_Operand_Kind_RegI;
      break;
    case 8:
      regKind = jit_Operand_Kind_RegP;
      break;
  }
  Operand newX( regKind, allocateReg() );
  emitMov( newX, x );
  x = newX;
}


// If operand is a memory reference, move it into a register.
// This is for instructions that already use a memory reference for another operand.
//
void
operandNotMem( Operand& x )
{
  if ( x.isMem() ) {
    operandIntoReg( x );
  }
}


// If operand has kind Addr_*, move it into a register.
//
void
operandKindAddrIntoReg( Operand& x )
{
  if ( !x.isAddrOfVar() ) {
    return;
  }

  Operand newX( jit_Operand_Kind_RegP, allocateReg() );
  emitMov( newX, x );
  x = newX;
}


// x is a pointer to a value of the given byte size.
// Make x refer to the pointed-to value.
// The resulting operand can be used as the left or right argument of emitMov().
//
void
operandDeref( Operand& x, int size )
{
  if ( x.isVar() ) {
    // the pointer is stored in a variable, so we need to fetch the pointer first.
    operandIntoReg( x );
  }

  // Create an opcond that represents the pointed-to value.
  switch ( x._kind ) {
    case jit_Operand_Kind_Addr_Global:
      switch ( size ) {
        case 1:
          x = Operand( jit_Operand_Kind_GlobalB, x._value );
          break;
        case 4:
          x = Operand( jit_Operand_Kind_GlobalI, x._value );
          break;
        case 8:
          x = Operand( jit_Operand_Kind_GlobalP, x._value );
          break;
        default:
          break;
      }
      break;
    case jit_Operand_Kind_Addr_Local:
      switch ( size ) {
        case 1:
          x = Operand( jit_Operand_Kind_LocalB, x._value );
          break;
        case 4:
          x = Operand( jit_Operand_Kind_LocalI, x._value );
          break;
        case 8:
          x = Operand( jit_Operand_Kind_LocalP, x._value );
          break;
        default:
          break;
      }
      break;
    case jit_Operand_Kind_Addr_Param:
      switch ( size ) {
        case 1:
          x = Operand( jit_Operand_Kind_ParamB, x._value );
          break;
        case 4:
          x = Operand( jit_Operand_Kind_ParamI, x._value );
          break;
        case 8:
          x = Operand( jit_Operand_Kind_ParamP, x._value );
          break;
        default:
          break;
      }
      break;
    case jit_Operand_Kind_Addr_Actual:
      switch ( size ) {
        case 1:
          x = Operand( jit_Operand_Kind_ActualB, x._value );
          break;
        case 4:
          x = Operand( jit_Operand_Kind_ActualI, x._value );
          break;
        case 8:
          x = Operand( jit_Operand_Kind_ActualP, x._value );
          break;
        default:
          break;
      }
      break;
    case jit_Operand_Kind_RegP:
      switch ( size ) {
        case 1:
          x = Operand( jit_Operand_Kind_RegP_DerefB, x._reg, 0 );
          break;
        case 4:
          x = Operand( jit_Operand_Kind_RegP_DerefI, x._reg, 0 );
          break;
        case 8:
          x = Operand( jit_Operand_Kind_RegP_DerefP, x._reg, 0 );
          break;
        default:
          break;
      }
      break;    
    default:
      fatal( "operandDeref: unexpected operands\n" );
      break;
  }
}


void
swap( Operand& x, Operand& y)
{
  Operand temp = x;
  x = y;
  y = temp;
}


// ----------------------------------------------------------------------------
//
// x86 Operations
//
// All my comments use Intel syntax:     instruction  dest, src
// in order to match Intel's documentation.
// Note that linux gdb shows the opposite AT&T syntax:   instruction  src, dest
//
// ----------------------------------------------------------------------------


// NOTES
//
// Some special cases that I need to be aware of.
//
//   rbp and r13 can't be used as a base register with no displacement.
//   Instead can use an explicit displacement of 0.
//   And if you want e.g. [r13+rdx], can avoid the problem by switching base/index
//   i.e. [rdx+r13] when that's an option.
//   I built this rule into emitModRMMem().
//
//   rsp / r12 as a base register always needs a SIB byte.
//   That's because the ModR/M encoding of base=rsp is the escape code to signal
//   a SIB byte.  And r12 hits the same limit to simplify decode logic.
//   I was able to build this rule into emitModRMMem().
//
//   rsp can't be an index register.    But, I don't use index registers yet anyway.
//   (r12 -can- be an index register.)
//


// x += y
// x is a register
//
void
emitAdd( const Operand& x, const Operand& y )
{
  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_GlobalB ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_LocalB ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ParamB ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ConstI ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_RegB ):
      toDo( "emitAdd\n" );
      break;


    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalI ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
      // add reg_x, [rbp + y.value];
      emitRex( false, x._reg, regRbp );
      outB( 0x03 );
      emitModRMMem( x._reg, regRbp, y._value );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
      // add reg_x, [rbp + y.value + FRAME_PARAMS_OFFSET]
      emitRex( false, x._reg, regRbp );
      outB( 0x03 );
      emitModRMMem( x._reg, regRbp, y._value + FRAME_PARAMS_OFFSET );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
      emitRex( false, nullptr, x._reg );
      outB( 0x81 );
      emitModRM_OpcRM( 0, x._reg );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      toDo( "emitAdd\n" );
      break;


    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalP ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalI ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalP ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalI ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamP ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamI ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ConstI ):
      emitRex( true, nullptr, x._reg );
      outB( 0x81 );
      emitModRM_OpcRM( 0, x._reg );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP ):
      toDo( "emitAdd\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegI ):
      toDo( "emitAdd\n" );
      break;

    default:
      fatal( "emitAdd: unexpected operands\n" );
  }
}


// x -= y
// x is a register
//
void
emitSub( const Operand& x, const Operand& y )
{
  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_GlobalB ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_LocalB ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ParamB ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ConstI ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_RegB ):
      toDo( "emitSub\n" );
      break;


    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalI ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
      emitRex( false, nullptr, x._reg );
      outB( 0x81 );
      emitModRM_OpcRM( 5, x._reg );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      toDo( "emitSub\n" );
      break;


    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalP ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalI ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalP ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalI ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamP ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamI ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ConstI ):
      emitRex( true, nullptr, x._reg );
      outB( 0x81 );
      emitModRM_OpcRM( 5, x._reg );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP ):
      toDo( "emitSub\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegI ):
      toDo( "emitSub\n" );
      break;

    default:
      fatal( "emitSub: unexpected operands\n" );
  }
}


// x = y
// x and y are not both memory references.
//
void
emitMov( const Operand& x, const Operand& y )
{
  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_GlobalB, jit_Operand_Kind_ConstI ):
      // mov [rip + offsetToGlobal], immmediate8
      emitRex( false, nullptr, nullptr );
      outB( 0xc6 );
      emitModRM_OpcRMMemRipRelative( 0, &data[x._value], 1 );
      outB( y._value );
      break;

    case KindPair( jit_Operand_Kind_GlobalB, jit_Operand_Kind_RegB ):
      // mov [rip + offsetToGlobal], reg8
      emitRex( false, y._reg, nullptr );
      outB( 0x88 );
      emitModRMMemRipRelative( y._reg, &data[x._value], 0 );
      break;

    case KindPair( jit_Operand_Kind_GlobalI, jit_Operand_Kind_ConstI ):
      // mov [rip + offsetToGlobal], immmediate32
      emitRex( false, nullptr, nullptr );
      outB( 0xc7 );
      emitModRM_OpcRMMemRipRelative( 0, &data[x._value], 4 );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_GlobalI, jit_Operand_Kind_RegI ):
      // mov [rip + offsetToGlobal], reg32
      emitRex( false, y._reg, nullptr );
      outB( 0x89 );
      emitModRMMemRipRelative( y._reg, &data[x._value], 0 );
      break;

    case KindPair( jit_Operand_Kind_GlobalP, jit_Operand_Kind_ConstI ):
      // mov [rip + offsetToGlobal], immmediate32 sign-extended to 64 bits
      emitRex( true, nullptr, nullptr );
      outB( 0xc7 );
      emitModRM_OpcRMMemRipRelative( 0, &data[x._value], 4 );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_GlobalP, jit_Operand_Kind_RegP ):
      // mov [rip + offsetToGlobal], reg64
      emitRex( true, y._reg, nullptr );
      outB( 0x89 );
      emitModRMMemRipRelative( y._reg, &data[x._value], 0 );
      break;

    case KindPair( jit_Operand_Kind_LocalB, jit_Operand_Kind_ConstI ):
      // mov [rbp+offset], immediate8
      emitRex( false, nullptr, regRbp );
      outB( 0xc6 );
      emitModRM_OpcRMMem( 0, regRbp, x._value );
      outB( y._value );
      break;

    case KindPair( jit_Operand_Kind_LocalB, jit_Operand_Kind_RegB ):
      // mov [rbp+offset], reg8
      emitRex( false, y._reg, regRbp );
      outB( 0x88 );
      emitModRMMem( y._reg, regRbp, x._value );
      break;

    case KindPair( jit_Operand_Kind_LocalI, jit_Operand_Kind_ConstI ):
      // mov [rbp+offset], immediate32
      emitRex( false, nullptr, regRbp );
      outB( 0xc7 );
      emitModRM_OpcRMMem( 0, regRbp, x._value );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_LocalI, jit_Operand_Kind_RegI ):
      // mov [rbp+offset], reg32
      emitRex( false, y._reg, regRbp );
      outB( 0x89 );
      emitModRMMem( y._reg, regRbp, x._value );
      break;

    case KindPair( jit_Operand_Kind_LocalP, jit_Operand_Kind_ConstI ):
      // mov [rbp+offset], immediate32 sign-extended to 64 bits
      emitRex( true, nullptr, regRbp );
      outB( 0xc7 );
      emitModRM_OpcRMMem( 0, regRbp, x._value );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_LocalP, jit_Operand_Kind_RegP ):
      // mov [rbp+offset], reg64
      emitRex( true, y._reg, regRbp );
      outB( 0x89 );
      emitModRMMem( y._reg, regRbp, x._value );
      break;

    case KindPair( jit_Operand_Kind_ParamB, jit_Operand_Kind_ConstI ):
      // mov [rbp + offset + FRAME_PARAMS_OFFSET], immediate8
      emitRex( false, nullptr, regRbp );
      outB( 0xc6 );
      emitModRM_OpcRMMem( 0, regRbp, x._value + FRAME_PARAMS_OFFSET );
      outB( y._value );
      break;

    case KindPair( jit_Operand_Kind_ParamB, jit_Operand_Kind_RegB ):
      // mov [rbp + offset + FRAME_PARAMS_OFFSET], reg8
      emitRex( false, y._reg, regRbp );
      outB( 0x88 );
      emitModRMMem( y._reg, regRbp, x._value + FRAME_PARAMS_OFFSET );
      break;

    case KindPair( jit_Operand_Kind_ParamI, jit_Operand_Kind_ConstI ):
      // mov [rbp + offset + FRAME_PARAMS_OFFSET], immediate32
      emitRex( false, nullptr, regRbp );
      outB( 0xc7 );
      emitModRM_OpcRMMem( 0, regRbp, x._value + FRAME_PARAMS_OFFSET );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_ParamI, jit_Operand_Kind_RegI ):
      // mov [rbp + offset + FRAME_PARAMS_OFFSET], reg32
      emitRex( false, y._reg, regRbp );
      outB( 0x89 );
      emitModRMMem( y._reg, regRbp, x._value + FRAME_PARAMS_OFFSET );
      break;

    case KindPair( jit_Operand_Kind_ParamP, jit_Operand_Kind_ConstI ):
      // mov [rbp + offset + FRAME_PARAMS_OFFSET], immediate32 sign-extended to 64 bits
      emitRex( true, nullptr, regRbp );
      outB( 0xc7 );
      emitModRM_OpcRMMem( 0, regRbp, x._value + FRAME_PARAMS_OFFSET );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_ParamP, jit_Operand_Kind_RegP ):
      // mov [rbp + offset + FRAME_PARAMS_OFFSET], reg64
      emitRex( true, y._reg, regRbp );
      outB( 0x89 );
      emitModRMMem( y._reg, regRbp, x._value + FRAME_PARAMS_OFFSET );
      break;

    // Note, Kind_Actual* only appear on the left side of emitMov,
    // and not in any other operations.
    // They are used to assign to actual space, when setting up a call.

    case KindPair( jit_Operand_Kind_ActualB, jit_Operand_Kind_ConstI ):
      // mov [rsp + offset], immediate8
      emitRex( false, nullptr, regRsp );
      outB( 0xc6 );
      emitModRM_OpcRMMem( 0, regRsp, x._value );
      outB( y._value );
      break;

    case KindPair( jit_Operand_Kind_ActualB, jit_Operand_Kind_RegB ):
      // mov [rsp + offset], reg8
      emitRex( false, y._reg, regRsp );
      outB( 0x88 );
      emitModRMMem( y._reg, regRsp, x._value );
      break;

    case KindPair( jit_Operand_Kind_ActualI, jit_Operand_Kind_ConstI ):
      // mov [rsp + offset], immediate32
      emitRex( false, nullptr, regRsp );
      outB( 0xc7 );
      emitModRM_OpcRMMem( 0, regRsp, x._value );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_ActualI, jit_Operand_Kind_RegI ):
      // mov [rsp + offset], reg32
      emitRex( false, y._reg, regRsp );
      outB( 0x89 );
      emitModRMMem( y._reg, regRsp, x._value );
      break;

    case KindPair( jit_Operand_Kind_ActualP, jit_Operand_Kind_ConstI ):
      // mov [rsp + offset], immediate32 sign-extended to 64 bits
      emitRex( true, nullptr, regRsp );
      outB( 0xc7 );
      emitModRM_OpcRMMem( 0, regRsp, x._value );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_ActualP, jit_Operand_Kind_RegP ):
      // mov [rsp + offset], reg64
      emitRex( true, y._reg, regRsp );
      outB( 0x89 );
      emitModRMMem( y._reg, regRsp, x._value );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_GlobalB ):
      // mov reg8, [rip + offsetToGlobal]
      emitRex( false, x._reg, nullptr );
      outB( 0x8a );
      emitModRMMemRipRelative( x._reg, &data[y._value], 0 );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_LocalB ):
      // mov x_reg8, [rbp + y._value]
      emitRex( false, x._reg, regRbp );
      outB( 0x8a );
      emitModRMMem( x._reg, regRbp, y._value );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ParamB ):
      // mov x_reg8, [rbp + y._value + FRAME_PARAMS_OFFSET]
      emitRex( false, x._reg, regRbp );
      outB( 0x8a );
      emitModRMMem( x._reg, regRbp, y._value + FRAME_PARAMS_OFFSET );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ConstI ):
      // mov reg8, immediate8
      // TO DO: confirm that this works correctly for all registers to r15,
      //        and doesn't get confused by encodings for ah bh ch dh etc.
      emitRex( false, nullptr, x._reg );
      outB( 0xb0 | regIdLowBits( x._reg->_nativeId ) );
      outB( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_RegB ):
      // mov x_reg8, y_reg8
      emitRex( false, x._reg, y._reg );
      outB( 0x8a );
      emitModRM_RegReg( x._reg, y._reg );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_RegP_DerefB ):
      // mov x_reg8, [y_reg64 + offset]
      emitRex( false, x._reg, y._reg );
      outB( 0x8a );
      emitModRMMem( x._reg, y._reg, y._value );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalI ):
      // mov x_reg32, [rip + offsetToGlobal]
      emitRex( false, x._reg, nullptr );
      outB( 0x8b );
      emitModRMMemRipRelative( x._reg, &data[y._value], 0 );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
      // mov x_reg32, [rbp + y._value]
      emitRex( false, x._reg, regRbp );
      outB( 0x8b );
      emitModRMMem( x._reg, regRbp, y._value );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
      // mov x_reg32, [rbp + y._value + FRAME_PARAMS_OFFSET]
      emitRex( false, x._reg, regRbp );
      outB( 0x8b );
      emitModRMMem( x._reg, regRbp, y._value + FRAME_PARAMS_OFFSET );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
      // mov x_reg32, immediate32
      emitRex( false, nullptr, x._reg );
      outB( 0xb8 | regIdLowBits( x._reg->_nativeId ) );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      // mov x_reg32, y_reg32
      emitRex( false, x._reg, y._reg );
      outB( 0x8b );
      emitModRM_RegReg( x._reg, y._reg );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegP_DerefI ):
      // mov x_reg32, [y_reg64 + offset]
      emitRex( false, x._reg, y._reg );
      outB( 0x8b );
      emitModRMMem( x._reg, y._reg, y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalP ):
      // mov x_reg64, [rip + offsetToGlobal]
      emitRex( true, x._reg, nullptr );
      outB( 0x8b );
      emitModRMMemRipRelative( x._reg, &data[y._value], 0 );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalP ):
      // mov x_reg, [rbp + y._value]
      emitRex( true, x._reg, regRbp );
      outB( 0x8b );
      emitModRMMem( x._reg, regRbp, y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamP ):
      // mov reg_x, [rbp + y._value + FRAME_PARAMS_OFFSET]
      emitRex( true, x._reg, regRbp );
      outB( 0x8b );
      emitModRMMem( x._reg, regRbp, y._value + FRAME_PARAMS_OFFSET );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Global ):
      // lea reg, [rip + offsetToGlobal]
      emitRex( true, x._reg, nullptr );
      outB( 0x8d );
      emitModRMMemRipRelative( x._reg, &data[y._value], 0 );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Local ):
      // lea reg64, [rbp+offset]
      emitRex( true, x._reg, regRbp );
      outB( 0x8d );
      emitModRMMem( x._reg, regRbp, y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Param ):
      // lea reg64, [rbp + y._value + FRAME_PARAMS_OFFSET]
      emitRex( true, x._reg, regRbp );
      outB( 0x8d );
      emitModRMMem( x._reg, regRbp, y._value + FRAME_PARAMS_OFFSET );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Actual ):
      // lea reg64, [rsp+offset]
      emitRex( true, x._reg, regRsp );
      outB( 0x8d );
      emitModRMMem( x._reg, regRsp, y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ConstI ):
      // mov reg64, immediate64 (which I sign-extend from the given 32-bit constant).
      // NOTE: this instruction really does take immediate64 rather than simply
      // sign-extending immediate32.
      emitRex( true, nullptr, x._reg );
      outB( 0xb8 | regIdLowBits( x._reg->_nativeId ) );
      outL( (long) y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP ):
      // mov x_reg64, y_reg64
      emitRex( true, x._reg, y._reg );
      outB( 0x8b );
      emitModRM_RegReg( x._reg, y._reg );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP_DerefP ):
      // mov x_reg64, [y_reg64 + offset]
      emitRex( true, x._reg, y._reg );
      outB( 0x8b );
      emitModRMMem( x._reg, y._reg, y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefB, jit_Operand_Kind_ConstI ):
      // mov [reg64+offset], immediate8
      emitRex( false, nullptr, x._reg );
      outB( 0xc6 );
      emitModRM_OpcRMMem( 0, x._reg, x._value );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefB, jit_Operand_Kind_RegB ):
      // mov [x_reg64 + offset], y_reg8
      emitRex( false, y._reg, x._reg );
      outB( 0x88 );
      emitModRMMem( y._reg, x._reg, x._value );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefI, jit_Operand_Kind_ConstI ):
      // mov [reg64+offset], immediate32
      emitRex( false, nullptr, x._reg );
      outB( 0xc7 );
      emitModRM_OpcRMMem( 0, x._reg, x._value );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefI, jit_Operand_Kind_RegI ):
      // mov [x_reg64 + offset], y_reg32
      emitRex( false, y._reg, x._reg );
      outB( 0x89 );
      emitModRMMem( y._reg, x._reg, x._value );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefP, jit_Operand_Kind_ConstI ):
      // mov [reg64+offset], immediate32 sign-extended to 64 bits
      emitRex( true, nullptr, x._reg );
      outB( 0xc7 );
      emitModRM_OpcRMMem( 0, x._reg, x._value );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefP, jit_Operand_Kind_RegP ):
      // mov [x_reg64 + offset], y_reg64
      emitRex( true, y._reg, x._reg );
      outB( 0x89 );
      emitModRMMem( y._reg, x._reg, x._value );
      break;

    default:
      fatal( "emitMov: unexpected operands\n" );
  }
}


// Emit a REX prefix if necessary.
// overrideWidth is true if we need to change a 32-bit operation to 64-bit.
// operandReg is the register that will be specified in the instruction's ModRM.reg field (if any).
//     The REX.R bit gives access to the extended set of registers there.
// baseReg is the register that will be specified in the instruction's ModRM.rm field
//     or the SIB.base field (if any).
//     Or for instuctions without any ModR/M byte, the register specified in the opcode's own reg field.
//     The REX.B field gives access to extended registers there.
//     Note that some instructions (e.g. ADD REG, immediate) use the Mod/RM rm field for the register,
//     because they use the reg field as an opcode extension.   So be sure to pass the register
//     to this method's "baseReg" in that case.
// I'm not using SIB index registers, so far, so don't need the REX.X bit.
//
// Here is Intel's rex description, which may be clearer:
//   REX.R modifies the ModR/M reg field when that field encodes a general purpose register,
//         SSE, control or debug register.   REX.R is ignored when ModR/M specifies other registers
//         or defines an extended opcode.
//   REX.X modifies the SIB index field.
//   REX.B either modifies the base inthe ModR/M r/m field or SIB base field;
//         or it modifies the opcode reg field used for accessing general purpose registers.
// 
void
emitRex( bool overrideWidth, const Register* operandReg, const Register* baseReg )
{
  int rex = 0x40;
  if ( overrideWidth ) {
    rex |= 8;  // REX.W
  }
  if ( operandReg && operandReg->_nativeId >= 8 ) {
    rex |= 4;  // REX.R
  }
  if ( baseReg && baseReg->_nativeId >= 8 ) {
    rex |= 1;  // REX.B
  }
  if ( rex != 0x40 ) {
    outB( rex );
  }
}


// Emit a ModR/M sequence suitable for a memory reference.
// operandReg is the standalone register used by the opcond (and might be null).
// baseReg is the base of the memory reference.
// offset is the offset from the baseReg.
// For now, I'm not allowing for use of an index register in addition to the base.
//
void
emitModRMMem( const Register* operandReg, const Register* baseReg, int offset )
{
  int regField = regIdLowBits( operandReg->_nativeId ) << 3;
  int baseRegId = baseReg->_nativeId;

  // Special case: if baseReg is rsp or r12, we must use the SIB byte
  // to do a [base + index].  This is because esp/rsp in ModRM's r/m field
  // is how one requests an SIB byte.
  // We can still do plain [base] via SIB too - by specifying esp/rsp as the index
  // which is treated as no-index.
  // The SIB byte goes between ModRM byte and any displacement.
  bool needSib = false;
  int sib = 0;
  if ( baseReg == regRsp || baseReg == regR12 ) {
    needSib = true;
    sib = ( regIdLowBits( regRsp->_nativeId ) << 3 ) | regIdLowBits( baseReg->_nativeId );
  }

  if ( ( offset == 0 ) &&
       ( baseReg != regRbp ) &&
       ( baseReg != regR13 ) ) {
    // mod 00 - no displacement
    // (Note another special case: rbp/r13 may not use this case. They fall through to next.)
    outB( regField | regIdLowBits( baseRegId ) );
    if ( needSib ) {
      outB( sib );
    }
  } else if ( offset <= 127 && offset >= -128 ) {
    // mod 01 - disp8
    outB( 0x40 | regField | regIdLowBits( baseRegId ) );
    if ( needSib ) {
      outB( sib );
    }
    outB( offset );
  } else {
    // mod 10 - disp32
    outB( 0x80 | regField | regIdLowBits( baseRegId ) );
    if ( needSib ) {
      outB( sib );
    }
    outI( offset );
  }
}


// This form of ModR/M is used for a memory reference using a register
// specified in the rm field, plus an offset.
// In addition, the ModR/M's reg field is used as an opcode extension field,
// rather than specifying another register.
//
void
emitModRM_OpcRMMem( int opcodeExtension, const Register* baseReg, int offset )
{
  int regField = opcodeExtension << 3;
  int baseRegId = baseReg->_nativeId;

  // Special case: if baseReg is rsp or r12, we must use the SIB byte
  // to do a [base + index].  This is because esp/rsp in ModRM's r/m field
  // is how one requests an SIB byte.
  // We can still do plain [base] via SIB too - by specifying esp/rsp as the index
  // which is treated as no-index.
  // The SIB byte goes between ModRM byte and any displacement.
  bool needSib = false;
  int sib = 0;
  if ( baseReg == regRsp || baseReg == regR12 ) {
    needSib = true;
    sib = ( regIdLowBits( regRsp->_nativeId ) << 3 ) | regIdLowBits( baseReg->_nativeId );
  }

  if ( ( offset == 0 ) &&
       ( baseReg != regRbp ) &&
       ( baseReg != regR13 ) ) {
    // mod 00 - no displacement
    // (Note another special case: rbp/r13 may not use this case. They fall through to next.)
    outB( regField | regIdLowBits( baseRegId ) );
    if ( needSib ) {
      outB( sib );
    }
  } else if ( offset <= 127 && offset >= -128 ) {
    // mod 01 - disp8
    outB( 0x40 | regField | regIdLowBits( baseRegId ) );
    if ( needSib ) {
      outB( sib );
    }
    outB( offset );
  } else {
    // mod 10 - disp32
    outB( 0x80 | regField | regIdLowBits( baseRegId ) );
    if ( needSib ) {
      outB( sib );
    }
    outI( offset );
  }
}


// This form of ModR/M references static memory, relative to rip.
// This is the common way to access static memory, since it only needs a 32-bit offset.
// (64-bit immediates are very limited in x86-64).  And it is relocatable.
//
// operandReg is the other operand of the instruction (in reg field).
//
// numAdditionalInstrBytes is how many bytes the caller is going to write for this instruction
// after the Mod/RM byte and displacement.   Typically 0, but more if there is an immediate argument.
// We need this because rip-relative is relative to the address after this entire instruction.
//
void
emitModRMMemRipRelative( const Register* operandReg, void* globalPtr, int numAdditionalInstrBytes )
{
  // mod 00
  // In this mode, r/m field 5 indicates [rip+offset32] rather than [rbp] 
  outB( ( regIdLowBits( operandReg->_nativeId ) << 3 ) | 0x5 );
  // rip-relative is relative to the next instruction.  Where will that be:
  char* nextInstrAddr = nativePc + 4 + numAdditionalInstrBytes;
  outI( (long) globalPtr - (long) nextInstrAddr );
}


// This form of ModR/M references static memory, relative to rip.
// Similar to emitModRMMemRipRelative(), but for an instruction that has an
// opcode extension stored in the ModR/M's reg field, rather than a register.
//
void
emitModRM_OpcRMMemRipRelative( int opcodeExtension, void* globalPtr, int numAdditionalInstrBytes )
{
  // mod 00
  // In this mode, r/m field 5 indicates [rip+offset32] rather than [rbp] 
  outB( ( opcodeExtension << 3 ) | 0x5 );
  // rip-relative is relative to the next instruction.  Where will that be:
  char* nextInstrAddr = nativePc + 4 + numAdditionalInstrBytes;
  outI( (long) globalPtr - (long) nextInstrAddr );
}


// This form of ModR/M is used for an immediate register.
// opcodeExtension is a bit pattern specified for the instruction,
// and is stored in the "Reg" field of ModR/M.    rm is a register stored in the "RM" field.
// So, this form is only used for instructions that use one register.
//
// It is necessary to check the Intel documentation for a specific instruction
// to know which of these ModR/M encodings should be used.
//
void
emitModRM_OpcRM( int opcodeExtension, const Register* rm )
{
  // mod 11
  outB( 0xc0 | (opcodeExtension << 3) | regIdLowBits( rm->_nativeId ) );
}


void
emitModRM_RegReg( const Register* opcodeReg, const Register* rm )
{
  // mod 11
  outB( 0xc0 | ( regIdLowBits( opcodeReg->_nativeId ) << 3) | regIdLowBits( rm->_nativeId ) );
}


// ----------------------------------------------------------------------------

void
fatal( const char* msg, ... )
{
  va_list args;
  va_start( args, msg );
  printf( "Error: " );
  vprintf( msg, args );
  va_end( args );
  exit( 1 );
}

void
toDo( const char* msg, ... )
{
  va_list args;
  va_start( args, msg );
  printf( "TO DO: " );
  vprintf( msg, args );
  va_end( args );
}



void
loadTCodeAndAllocNativeCode()
{
  FILE* src = fopen( filename, "r" );
  if ( !src ) {
    fatal( "can't open intermediate file %s\n", filename );
  }
  int read = fscanf( src, "%d", &tCodeLen );
  assert( read == 1 );
  tCode = (int*) malloc( tCodeLen * sizeof( int ) );
  assert( tCode != 0 );
  for ( int i = 0; i < tCodeLen; ++i ) {
    read = fscanf( src, "%d", &tCode[i] );
    assert( read == 1 );
  }

  // Need to alloc memory before loading initialized static data
  allocNativeCode();

  // Load string literals into data memory
  // File format: <addr> <#ints> <data ints ...>
  int address;
  int count;
  while ( fscanf( src, "%d", &address ) != EOF ) {
    read = fscanf( src, "%d", &count );
    int* intPtr = (int*) &(data[address]);
    while ( count-- ) {
      int word;
      read = fscanf( src, "%d", &word );
      assert( read == 1 );
      *intPtr++ = word;
    } 
  }

  fclose( src );
  printf( "- program loaded (%d words)\n", tCodeLen );
}


void
tCodeNotImplemented( int opcode )
{
  printf( "Warning: tCode operation %d not supported yet\n", opcode );
}


void
allocNativeCode()
{
  // We'll allocate memory for native code followed contiguously by static data.
  // This will ensure the static data is within 2GB of the code, and we can use
  // rip-relative addressing for it.
  //
  // Round up the code allocation to a multiple of pages, so we'll be able to
  // mprotect the code but not the data.   I think linux default page size is 4K
  // but I'll use 8K.
  //
  nativeCodeAllocSize = ( (nativeCodeLen / PAGE_SIZE) + 1 ) * PAGE_SIZE;
  int allocSize = nativeCodeAllocSize + dataSize;

  // Allocate memory for both code and static data.
  nativeCode = (char*) mmap( NULL, allocSize, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
  nativePc = nativeCode;
  data = nativeCode + nativeCodeAllocSize;
  if ( nativeCode == NULL ) {
    fatal( "mmap failed to allocate %ld bytes for native code and data\n", allocSize );
  }
}


void
protectNativeCode()
{
  // Switch memory from writable to executable.
  // This also ensures the instruction cache doesn't have stale data.
  if ( mprotect( nativeCode, nativeCodeAllocSize, PROT_READ | PROT_EXEC ) != 0 ) {
    fatal( "mprotect failed with status %d\n", errno );
  }
}


void
executeCode()
{
  printf( "Won't execute code until development is further along...\n" );
  return;


  printf( "Executing code ...\n" );

  funcptr f = (funcptr) nativeCode;
  f();

  printf( "Done\n" );
}


void
outB( char c )
{
  *(nativePc++) = c;
}


// emit a 32-bit value
void
outI( int i )
{
  int* p = (int*) nativePc;
  *(p++) = i;
  nativePc = (char*) p;
}


// emit a 64-bit value
void
outL( long l )
{  long* p = (long*) nativePc;
   *(p++) = l;
   nativePc = (char*) p;
}


#if 0
  // This little dummy function works.

   outB( 0x55 );  // push %rbp
   outB( 0x48 );  // mov  %rsp,%rbp
   outB( 0x89 );
   outB( 0xe5 );
   outB( 0x48 );  // sub $0x40,%rsp
   outB( 0x83 );
   outB( 0xec );
   outB( 0x40 );

   outB( 0xc9 );  // leaveq
   outB( 0xc3 );  // retq
#endif



#if 0
   // Old opcodes from very old experiment, which was 32-bit.
   // I'll need to figure out 64-bit instruction set.

   // start stack frame

   outB( 0x55 );  // push ebp       -- Now is push rbp
   outB( 0x8b );  // mov ebp, esp   -- Need to switch to rsp, rbp.
   outB( 0xec );
   outB( 0x81 );  // sub esp, (int) 20
   outB( 0xec );
   outI( 20 );
   outB( 0x56 );  // push esi
   outB( 0x57 );  // push edi

   outB( 0x8d );  // lea esi, [ebp-(int)20]
   outB( 0xb5 );
   outI( -20 );

   
   // end stack frame
   outB( 0x5f );  // pop edi
   outB( 0x5e );  // pop esi
   outB( 0xc9 );  // leave

   // ret
   outB( 0xc3 );  // ret
#endif



// Here's a bit of assembly from start of main of sm.c
// Note gdb is printing in AT&T syntax (since unix heritage) i.e.   "move from, to"
// as opposed to the Intel syntax prevelant in windows world i.e.  "move to, from"

// => 0x555555555269 <main>:	endbr64 
//    0x55555555526d <main+4>:	push   %rbp
//    0x55555555526e <main+5>:	mov    %rsp,%rbp
//    0x555555555271 <main+8>:	sub    $0x40,%rsp
//    0x555555555275 <main+12>:	mov    %edi,-0x34(%rbp)
//    0x555555555278 <main+15>:	mov    %rsi,-0x40(%rbp)
//    0x55555555527c <main+19>:	mov    %fs:0x28,%rax
//    0x555555555285 <main+28>:	mov    %rax,-0x8(%rbp)
//    0x555555555289 <main+32>:	xor    %eax,%eax
//    0x55555555528b <main+34>:	movb   $0x0,0x1d49e(%rip)        # 0x555555572730 <trace>
//    0x555555555292 <main+41>:	movl   $0x1,-0x18(%rbp)
//    0x555555555299 <main+48>:	mov    -0x18(%rbp),%eax
//    0x55555555529c <main+51>:	cmp    -0x34(%rbp),%eax
//    0x55555555529f <main+54>:	jl     0x5555555553a7 <main+318>
//    0x5555555552a5 <main+60>:	lea    0x2d5c(%rip),%rdi        # 0x555555558008
//    0x5555555552ac <main+67>:	callq  0x555555555100 <puts@plt>
//    0x5555555552b1 <main+72>:	mov    $0xffffffff,%edi
//    0x5555555552b6 <main+77>:	callq  0x555555555170 <exit@plt>
// 


