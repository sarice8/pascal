/*

  jit.c

  This converts the intermediate t-code produced by my Pascal compiler
  into x86 machine code, and executes it.

*/

/*

BUGS

I have these problems.

  - my tcode file doesn't say how much static data space it needs.
  - not allowing const expression in array bound definition

  - not confident that I have the ';' syntax right.

  - calling convention to standard external code.
      - only properly implemented for calls whose params fit in registers.  No complex types.
      - hardcoded for linux.  win64 version not implemented or triggered yet.
      - Stack-based params need to be aligned to 8-byte slots.

      My current approach for calling cdecl (native code) functions:
      tCode has my usual pascal calling convention for the parameters and return value.
      i.e. it copies params into actuals space on the stack.  Only difference is
      tCallCdecl <extern label> rather than tCall <label>.
      Jit will generate code to load the params from the stack (where I put them)
      into the necessary registers, and vice versa for the return value.
      It's not the most efficient, but it means the tcode doesn't depend on the platform
      (which have different calling conventions).  Maybe later I do need some sort of
      param tcode, that could help make this work better, but this is ok to start with.

  - I should support emitMov( int, byte ) e.g. for passing a byte to an extern method taking int,
      where that should be allowed.  Just need the appropriate movsx instruction.
  - I've done no align/padding of allocated vars, or record fields.

  - not yet honoring callee-save or caller-save of registers
  - have not yet implemented dumping registers into temp, when I run out
    (or when a required register is in use - though I could move that to a different reg.)
  - I think there's also a flaw in move-register-to-temp.  The caller has already
    popped operands off the stack, so when I try to update the contents of operand stack
    to reflect the new location, it misses those already-popped operands.
    A chance that those could thus refer to the old location, if they were not also
    the one passed into the dump-to-temp call.  Maybe solve by not removing from stack
    until the operation is done.

  - I may have some situations where there are Flags on the expr stack,
    but the operation using the operand doesn't expect that.
    In general I'd add operandFlagsToValue() in those places.  But can I find them
    systematically?  Or do I need to add that everywhere?


MISSING LANGUAGE FEATURES

real type not implemented.
file type not implemented.
set type not implemented.
unions ( "case" inside record definition )

exit[(value)] - an extension to leave a procedure / function (with optional return value) / program.

Support other flavors of integer - unsigned int, int64, int8.  (Look up the complete set).
These need their own tcode operators and jit operand types, e.g. to produce different comparision flags.

Dynamic memory allocation functions.  Mainly new, dispose (also ReAllocMem).  fpc has a bunch of others
but not sure how standard: https://www.tutorialspoint.com/pascal/pascal_memory.htm

Support type casts.

Proper type checking for method forward declaration vs body declaration.  Type matching rule
that folks apparently use is that must have the same type declaration, not just the same content of
the type declaration.

Add missing standard functions like sin().

If multiple pascal units (as opposed to native units) are used together,
those units need to be allocated within 2GB of each other so they can access
the public global variables and methods in my usual way.  This should probably be straightforward.


OUT OF SCOPE

Not considering Object Pascal extensions at this time.
Probably ot considering other extensions done by various vendors.


ROOM FOR IMPROVEMENT

Move pascal.c to C++, and use vectors instead of fixed-size arrays to remove limits.
Be able to allocate more code/data memory if needed.
Know how much static data memory is needed.
Might be nice to have static allocation of data (currently only does so for string literals).

Some improvements can be done directly on tCode, perhaps with an extra pass prior to jit:
 - clean up jumps: remove chains of jumps, or jumps-to-here.
 - no more need for label aliases after that, or multiple labels for the same location.
 - process constants, including constant conditions, so that can improve the jump cleanup too.
 - identify and remove unreachable code, after const cleanup.
   (trick is that it shouldn't be fooled by jumps from within the unreachable region).
Such a cleanup pass could read tCode1 and produce a simpler tCode2.

Optimizing across statements:

Keep track of when a register is an alias for a particular variable value.
So reading the variable can instead read the register (as long as the register is not modified,
or the variable is not modified by somebody else).  This is the sort of thing that can
fail if optimistic about what pointers might do - but perhaps could clear alias tracking
whenever assigning to a dereferenced pointer.  Also, could start by always writing through to
variables, but could even eliminate that if I know there will be another write to the variable
later before anybody could see it.

Operations that work on a register then immediately assign into a variable
could sometimes directly operate on the variable.
e.g. ++var  is currently  push addr var, push val var, inc, assign.  But could be inc var.

*/



#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
//  -- for mmap, mprotect
#include <errno.h>

#include <iostream>
#include <sstream>
#include <string>
#include <stack>
#include <vector>
#include <unordered_map>

#include <SDL.h>
//   -- for my graphics api


// This is where we get the definitions for tCode,
// which originate in SSL.
// Note, some of the symbols conflict with std headers,
// which I could resolve  by not using preprocessor symbols.
// Meanwhile as long as I include this after the std headers, it's ok.
#include "pascal.h"
#include "tcode.h"

#include "runlib.h"


typedef void (*funcptr)();


void parseArgs( int argc, char* argv[] );
void usage( int status );
void loadTCodeAndAllocNativeCode();
void allocNativeCode();
void protectNativeCode();
void dumpNativeCode();
void outB( char c );
void outI( int i );
void outL( long l );
void generateCode();
void finishMethod();
void tCodeNotImplemented();
void executeCode();
void fatal( const char* msg, ... );
void toDo( const char* msg, ... );


char* filename = 0;
bool optionListing = true;
FILE* listingFile = nullptr;
bool optionDumpCode = true;
FILE* dumpCodeFile = nullptr;

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
#define dataSize 1000000
char* data = 0;



char* nativeCode = 0;
size_t nativeCodeLen = 10000;
size_t nativeCodeAllocSize = 0;
size_t PAGE_SIZE = 8192;
char* nativePc = 0;


// Offset on call stack from rbp to the first parameter
#define FRAME_PARAMS_OFFSET (2 * sizeof(void*))
#define FPO  FRAME_PARAMS_OFFSET

// Hardcoded offset from fp to static link pointer,
// in a nested method.  This points to the frame of the enclosing scope.
// In my calling convention, it is actually the hidden first parameter
// to the method.  So it sits at the start of the params.
#define FRAME_STATIC_LINK_OFFSET  FRAME_PARAMS_OFFSET


// Operand stack
//
typedef enum {
  jit_Operand_Kind_Illegal = 0,
  jit_Operand_Kind_None,

  // operand is the value of a variable
  jit_Operand_Kind_GlobalB,
  jit_Operand_Kind_GlobalI,
  jit_Operand_Kind_GlobalP,
  jit_Operand_Kind_GlobalD,
  jit_Operand_Kind_LocalB,
  jit_Operand_Kind_LocalI,
  jit_Operand_Kind_LocalP,
  jit_Operand_Kind_LocalD,
  jit_Operand_Kind_ParamB,
  jit_Operand_Kind_ParamI,
  jit_Operand_Kind_ParamP,
  jit_Operand_Kind_ParamD,
  // Actual* won't occur on expression stack, but is used internally to help assign to actuals
  jit_Operand_Kind_ActualB,
  jit_Operand_Kind_ActualI,
  jit_Operand_Kind_ActualP,
  jit_Operand_Kind_ActualD,

  // operand is the address of a variable
  jit_Operand_Kind_Addr_Global,
  jit_Operand_Kind_Addr_Local,
  jit_Operand_Kind_Addr_Param,
  jit_Operand_Kind_Addr_Actual,
  jit_Operand_Kind_Addr_Reg_Offset,

  jit_Operand_Kind_ConstI,
  jit_Operand_Kind_ConstD,

  // operand is a value in a register.
  // We either have one byte valid, 4 bytes valid, or 8 bytes valid.
  // In the case of 4 bytes, we are guaranteed that the higher 4 bytes
  // of the register are 0 (note: not sign extended), thanks to x86-64 rules.
  // This is not the case if the valid size is one byte.
  jit_Operand_Kind_RegB,
  jit_Operand_Kind_RegI,
  jit_Operand_Kind_RegP,
  // operand is a value in a floating point register.
  jit_Operand_Kind_RegD,

  // operand is a value pointed to by a register.
  // For now, these are just used by emitMov, and won't appear on expression stack.
  jit_Operand_Kind_RegP_DerefB,
  jit_Operand_Kind_RegP_DerefI,
  jit_Operand_Kind_RegP_DerefP,
  jit_Operand_Kind_RegP_DerefD,

  // operand is held in the condition register.
  // Operand._flags indicates which conditions define a "true" result.
  jit_Operand_Kind_Flags,

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
  Register( const std::string& name, int nativeId, int isFloat,
            bool calleeSaveLinux64, bool calleeSaveWin64 )
    : _name( name ), _nativeId( nativeId ), _isFloat( isFloat ),
      _calleeSave( calleeSaveLinux64 ), _calleeSaveWin64( calleeSaveWin64 )
    {}

  const bool isFloat() { return _isFloat; }

  void release() { _inUse = false; }

  std::string _name = "";
  int _nativeId = 0;
  bool _isFloat = false;
  bool _calleeSave = false;
  bool _calleeSaveWin64 = false;   // to do. currently ignored.
  bool _inUse = false;
};

int regIdLowBits( int nativeId ) { return nativeId & 0x7; }
int regIdHighBit( int nativeId ) { return nativeId >> 3; }


// Note: calling convention for "x86-64 System V" is:
//   arguments that are classified as "integer/pointer" rather than "memory"
//   are passed in registers, left to right:  rdi, rsi, rdx, rcx, r8, r9.
//   Floating point args are passed in xmm0 - xmm7.
//   See https://stackoverflow.com/questions/2535989/what-are-the-calling-conventions-for-unix-linux-system-calls-and-user-space-f
// Microsoft describes Windows x64 calling convention ("__fastcall"):
//   https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170&viewFallbackFrom=vs-2017


Register* regRax = new Register( "rax", 0, false, false, false );
Register* regRcx = new Register( "rcx", 1, false, false, false );
Register* regRdx = new Register( "rdx", 2, false, false, false ); 
Register* regRbx = new Register( "rbx", 3, false, true, true );
Register* regRsp = new Register( "rsp", 4, false, false, false );
Register* regRbp = new Register( "rbp", 5, false, true, true );
Register* regRsi = new Register( "rsi", 6, false, false, true );
Register* regRdi = new Register( "rdi", 7, false, false, true );
Register* regR8  = new Register( "r8", 8,  false, false, false );
Register* regR9  = new Register( "r9", 9,  false, false, false );
Register* regR10 = new Register( "r10", 10, false, false, false );
Register* regR11 = new Register( "r11", 11, false, false, false );
Register* regR12 = new Register( "r12", 12, false, true, true );
Register* regR13 = new Register( "r13", 13, false, true, true );
Register* regR14 = new Register( "r14", 14, false, true, true );
Register* regR15 = new Register( "r15", 15, false, true, true );


// General purpose registers, in the order we prefer to allocate them.
//
// regR10 is reserved for pointer to parent static scope, in a nested method.
// This is the conventional register for a static scope link in x86-64.
// (In non-nested methods I could use it, but I won't for simplicity.)
//
Register* registers[] = {
  regRax,
  regRcx,
  regRbx,
  regRdx,
  regR8,
  regR9,
  regR11,
  regR12,
  regR13,
  regR14,
  regR15
};
int numRegisters = sizeof(registers) / sizeof(Register*); 


// Floating point registers (xmm) have overlapping native id's.
// The instruction itself dictates whether a floating point register is referenced.
// I'll be using the SSE2 instruction subset.

Register* regXmm0 = new Register( "xmm0", 0, true, false, false );
Register* regXmm1 = new Register( "xmm1", 1, true, false, false );
Register* regXmm2 = new Register( "xmm2", 2, true, false, false );
Register* regXmm3 = new Register( "xmm3", 3, true, false, false );
Register* regXmm4 = new Register( "xmm4", 4, true, false, false );
Register* regXmm5 = new Register( "xmm5", 5, true, false, false );
Register* regXmm6 = new Register( "xmm6", 6, true, false, false );
Register* regXmm7 = new Register( "xmm7", 7, true, false, false );
Register* regXmm8 = new Register( "xmm8", 8, true, false, false );
Register* regXmm9 = new Register( "xmm9", 9, true, false, false );
Register* regXmm10 = new Register( "xmm10", 10, true, false, false );
Register* regXmm11 = new Register( "xmm11", 11, true, false, false );
Register* regXmm12 = new Register( "xmm12", 12, true, false, false );
Register* regXmm13 = new Register( "xmm13", 13, true, false, false );
Register* regXmm14 = new Register( "xmm14", 14, true, false, false );
Register* regXmm15 = new Register( "xmm15", 15, true, false, false );

// Floating point registers, in the order we prefer to allocate them.
//
Register* floatRegisters[] = {
  regXmm0,
  regXmm1,
  regXmm2,
  regXmm3,
  regXmm4,
  regXmm5,
  regXmm6,
  regXmm7,
  regXmm8,
  regXmm9,
  regXmm10,
  regXmm11,
  regXmm12,
  regXmm13,
  regXmm14,
  regXmm15
};
int numFloatRegisters = sizeof(floatRegisters) / sizeof(Register*); 


std::vector<Register*> paramRegsLinux64 = { regRdi, regRsi, regRdx, regRcx, regR8, regR9 };
std::vector<Register*> paramRegsWin64 = { regRcx, regRdx, regR8, regR9 };
// TO DO: there are also differences in use of XMM/YMM/ZMM registers as params.  Floating point. (Also vector? or no?)
// TO DO: there are also differences for return value.

// For now, hardcoded for linux64
std::vector<Register*>& paramRegs = paramRegsLinux64;

// TO DO: needs work for windows
std::vector<Register*> paramFloatRegs = { regXmm0, regXmm1, regXmm2, regXmm3, regXmm4, regXmm5, regXmm6, regXmm7 };

// Condition flags, held in the condition register, and used by
// an operand of kind jit_Operand_Kind_Flags.
//
// My naming is based on the conditions in the J<cc> instructions
// (which actually comprise combinations of condition codes).
//// In fact, the instruction J<cc> <rip+offset32> is the byte sequence
//    0x0f <ConditionFlags> <offset32>
//
enum ConditionFlags {
  FlagInvalid = 0,
  FlagA  = 0x87,   // unsigned >
  FlagAE = 0x83,   // unsigned >=
  FlagB  = 0x82,   // unsigned <
  FlagBE = 0x86,   // unsigned <=
  FlagE  = 0x84,   // ==
  FlagG  = 0x8f,   // signed >
  FlagGE = 0x8d,   // signed >=
  FlagL  = 0x8c,   // signed <
  FlagLE = 0x8e,   // signed <=
  FlagNE = 0x85,   // <> 
};


// Change the condition flag to its negative condition.
//
void
negateConditionFlag( ConditionFlags& f ) {
  switch ( f ) {
    case FlagA :  f = FlagBE; break;
    case FlagAE : f = FlagB; break;
    case FlagB :  f = FlagAE; break;
    case FlagBE : f = FlagA; break;
    case FlagE :  f = FlagNE; break;
    case FlagG :  f = FlagLE; break;
    case FlagGE : f = FlagL; break;
    case FlagL :  f = FlagGE; break;
    case FlagLE : f = FlagG; break;
    case FlagNE : f = FlagE; break;
    default:
      fatal( "unexpected ConditionFlag\n" );
  }
}


// Change the condition flag to the equivalent that swaps
// the left and right sides of the condition.
//
void
swapConditionFlag( ConditionFlags& f ) {
  switch ( f ) {
    case FlagA :  f = FlagB; break;
    case FlagAE : f = FlagBE; break;
    case FlagB :  f = FlagA; break;
    case FlagBE : f = FlagAE; break;
    case FlagE :  f = FlagE; break;
    case FlagG :  f = FlagL; break;
    case FlagGE : f = FlagLE; break;
    case FlagL :  f = FlagG; break;
    case FlagLE : f = FlagGE; break;
    case FlagNE : f = FlagNE; break;
    default:
      fatal( "unexpected ConditionFlag\n" );
  }
}


class Operand
{
public:
  Operand() = default;

  Operand( jitOperandKind k, int val )
    : _kind( k ), _value( val )
  {}

  Operand( jitOperandKind k, double val )
    : _kind( k ), _double( val )
  {}

  Operand( jitOperandKind k, Register* r )
    : _kind( k ), _reg( r )
  {}

  Operand( jitOperandKind k, Register* r, int val )
    : _kind( k ), _reg( r ), _value( val )
  {}

  Operand( jitOperandKind k, ConditionFlags flags )
    : _kind( k ), _flags( flags )
  {}

  bool isReg() const { return _kind >= jit_Operand_Kind_RegB &&
                              _kind <= jit_Operand_Kind_RegD; }
  bool isFloatReg() const { return _kind == jit_Operand_Kind_RegD; }
  bool isVar() const { return _kind >= jit_Operand_Kind_GlobalB &&
                              _kind <= jit_Operand_Kind_ActualD; }
  bool isAddrOfVar() const { return _kind >= jit_Operand_Kind_Addr_Global &&
                                    _kind <= jit_Operand_Kind_Addr_Reg_Offset; }
  bool isVarOrAddrOfVar() const { return isVar() || isAddrOfVar(); }
  bool isDeref() const { return _kind >= jit_Operand_Kind_RegP_DerefB &&
                                _kind <= jit_Operand_Kind_RegP_DerefD; }
  bool isMem() const { return isVar() || isAddrOfVar() || isDeref(); }
  bool isConst() const { return _kind >= jit_Operand_Kind_ConstI &&
                                _kind <= jit_Operand_Kind_ConstD; }
  bool isFlags() const { return _kind == jit_Operand_Kind_Flags; }
  int  size() const;  // valid size of the operand value in bytes (1, 4, or 8)
  bool isFloat() const;
  void release();     // don't need this register anymore (if any)

  std::string describe() const;

  bool operator==( const Operand& other ) const {
    return _kind == other._kind &&
           _value == other._value &&
           _reg == other._reg;
  }
  bool operator!=( const Operand& other ) const {
    return !( *this == other );
  }

  // DATA

  jitOperandKind _kind = jit_Operand_Kind_Illegal;
  int _value = 0;         // for a var: the var offset
                          // for a const: the const value
  double _double = 0.0;   // for a const double: the const value
  Register* _reg = nullptr;    // for jit_Operand_Kind_Reg*
  ConditionFlags _flags = FlagInvalid;  // for jit_Operand_Kind_Flags
};


// For debugging
//
std::string
Operand::describe() const
{
  std::ostringstream str;

  switch ( _kind ) {
    case jit_Operand_Kind_Illegal:
    case jit_Operand_Kind_None:
      return "?";

    case jit_Operand_Kind_GlobalB:
      str << "GlobalB(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_GlobalI:
      str << "GlobalI(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_GlobalP:
      str << "GlobalP(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_GlobalD:
      str << "GlobalD(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_LocalB:
      str << "LocalB(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_LocalI:
      str << "LocalI(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_LocalP:
      str << "LocalP(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_LocalD:
      str << "LocalD(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_ParamB:
      str << "ParamB(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_ParamI:
      str << "ParamI(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_ParamP:
      str << "ParamP(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_ParamD:
      str << "ParamD(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_ActualB:
      str << "ActualB(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_ActualI:
      str << "ActualI(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_ActualP:
      str << "ActualP(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_ActualD:
      str << "ActualD(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_Addr_Global:
      str << "Addr_Global(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_Addr_Local:
      str << "Addr_Local(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_Addr_Param:
      str << "Addr_Param(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_Addr_Actual:
      str << "Addr_Actual(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_Addr_Reg_Offset:
      str << "Addr_Reg_Offset(" << _reg->_name << "," << _value << ")";
      return str.str();
    case jit_Operand_Kind_ConstI:
      str << "ConstI(" << _value << ")";
      return str.str();
    case jit_Operand_Kind_ConstD:
      str << "ConstD(" << _double << ")";
      return str.str();
    case jit_Operand_Kind_RegB:
      str << "RegB(" << _reg->_name << ")";
      return str.str();
    case jit_Operand_Kind_RegI:
      str << "RegI(" << _reg->_name << ")";
      return str.str();
    case jit_Operand_Kind_RegP:
      str << "RegP(" << _reg->_name << ")";
      return str.str();
    case jit_Operand_Kind_RegD:
      str << "RegD(" << _reg->_name << ")";
      return str.str();
    case jit_Operand_Kind_RegP_DerefB:
      str << "RegP_DerefB(" << _reg->_name << "," << _value << ")";
      return str.str();
    case jit_Operand_Kind_RegP_DerefI:
      str << "RegP_DerefI(" << _reg->_name << "," << _value << ")";
      return str.str();
    case jit_Operand_Kind_RegP_DerefP:
      str << "RegP_DerefP(" << _reg->_name << "," << _value << ")";
      return str.str();
    case jit_Operand_Kind_RegP_DerefD:
      str << "RegP_DerefD(" << _reg->_name << "," << _value << ")";
      return str.str();
    case jit_Operand_Kind_Flags:
      str << "Flags(" << _flags << ")";
      return str.str();

    default:
      return "?";
  }
}


// Don't need this register anymore (if any)
//
void
Operand::release()
{
  if ( _reg ) {
    _reg->release();
  }
}


// std::stack doesn't allow traversal of all elements
std::vector<Operand> operandStack;

void swap( Operand& x, Operand& y );
void operandIntoReg( Operand& x );
void operandIntoRegCommutative( Operand& x, Operand& y );
void operandIntoFloatReg( Operand& x );
void operandIntoFloatRegCommutative( Operand& x, Operand& y );
void operandIntoSpecificReg( Operand& x, Register* reg, int size );
void operandKindAddrIntoReg( Operand& x );
void operandNotMem( Operand& x );
void operandNotRegOrDeref( Operand& x );
void operandDeref( Operand& x, int valueSize );
void operandDerefFloat( Operand& x, int valueSize );
void operandExtendToP( Operand& x );
Operand operandCompare( Operand& x, Operand& y, ConditionFlags flags );
Operand operandCompareFloat( Operand& x, Operand& y, ConditionFlags flags );
void operandFlagsToValue( Operand& x, int size );
Operand operandLowWord( const Operand& x );
Operand operandHighWord( const Operand& x );

Register* allocateReg();
void forceAllocateReg( Register* reg );
Register* allocateFloatReg();
Register* getUpFrame( int uplevels );
Operand allocateTemp( int size, bool isFloat );
void preserveRegsAcrossCall();
jitOperandKind operandKindReg( int size, bool isFloat );

std::unordered_map<int, char*> labels;
std::unordered_map<int, int> labelTCodeAddrs;
std::unordered_map<int, int> labelAliases;
std::unordered_map<int, char*> unresolvedExternLabels;
std::vector< std::pair< int*, int> > patches;
int nextLabel = 0;

void defineLabels();
void defineLabel( int label, char* nativeAddr );
void defineLabelAlias( int label, int aliasToLabel );
void defineLabelTCode( int label, int tCodeAddr );
void defineLabelExtern( int label, char* name );
char* findLabel( int label );
int findLabelTCode( int label );
void requestPatch( int* patchAt, int label );
void makePatches();
int isPowerOf2( int x );


int* currLocalSpaceAddr = 0;
int currLocalSpace = 0;


// Tracking info about a call that's being constructed.
// This is necessary to support the use of the cdecl calling convention
// if necessary.
class CallInfo
{
public:
  CallInfo( bool cdecl )
    : _cdecl( cdecl )
  {}

  bool _cdecl;
  // For a cdecl call, these are the operands in actuals space
  // that should be moved into registers.
  std::vector<std::pair<Operand, Register*>> _actualsToRegs;
  // TO DO: track an offset where we should really be
  //  assigning non-register actuals to.
  //  For the moment, I don't support non-register params in cdecl.

  // How many param registers have been assigned for this call
  int _numGeneralRegs = 0;
  int _numFloatRegs = 0;

  bool _isFunc = false;
  // If isFunc, this is the location of the temporary in local space
  // that my tcode expects the return value to be stored in.
  Operand _returnValue;
};

// We might have calls within actual expressions, so a stack of calls
// in the process of construction.
std::vector<CallInfo> callInfos;

void cdeclCheckForAssignToActual( Operand& lhs, jitOperandKind actualKind );
void cdeclCheckForFunction( int* tCodePc );
void cdeclParamsIntoRegisters();
void cdeclSaveReturnValue();


// x86 operations
//
void emitEnter( int localSpace );
void emitReturn();
void emitReturnFromInitialStub();
void emitCall( char* addr );
void emitCallExtern( char* addr );
void emitJmpToLabel( int label );
void emitJccToLabel( int label, ConditionFlags flags );
void emitJmp( char* addr );
void emitJcc( char* addr, ConditionFlags flags );
void emitAdd( const Operand& x, const Operand& y );
void emitAddFloat( const Operand& x, const Operand& y );
void emitSub( const Operand& x, const Operand& y );
void emitSubFloat( const Operand& x, const Operand& y );
void emitNeg( const Operand& x );
void emitPxor( const Operand& x, const Operand& y );
void emitMult( const Operand& x, const Operand& y );
void emitMultFloat( const Operand& x, const Operand& y );
void emitCdq();
void emitCvtIntToDouble( const Operand& x, const Operand& y );
void emitDiv( const Operand& x, const Operand& y );
void emitDivFloat( const Operand& x, const Operand& y );
void emitShl( const Operand& x, const Operand& y );
void emitInc( const Operand& x );
void emitDec( const Operand& x );
void emitCmp( const Operand& x, const Operand& y );
void emitComisd( const Operand& x, const Operand& y );
void emitSet( const Operand& x, ConditionFlags flags );
void emitMov( const Operand& x, const Operand& y );

void emitRex( bool wide, const Register* reg, const Register* rmReg,
              bool reg8 = false, bool rmReg8 = false );
void emitModRMMem( const Register* operandReg, const Register* baseReg, int offset );
void emitModRM_OpcRMMem( int opcodeExtension, const Register* baseReg, int offset );
void emitModRM_OpcRMReg( int opcodeExtension, const Register* rm );
void emitModRM_RegReg( const Register* opcodeReg, const Register* rm );
void emitModRMMemRipRelative( const Register* operandReg, void* globalPtr, int numAdditionalInstrBytes );
void emitModRM_OpcRMMemRipRelative( int opcodeExtension, void* globalPtr, int numAdditionalInstrBytes );


int
main( int argc, char* argv[] )
{
  parseArgs( argc, argv );

  if ( optionListing ) {
    std::string listingFilename( filename );
    listingFilename.append( ".lst" );
    listingFile = fopen( listingFilename.c_str(), "w" );
    if ( !listingFile ) {
      printf(" jit can't open listing file %s\n", listingFilename.c_str() );
    }
  }
  loadTCodeAndAllocNativeCode();
  defineLabels();
  generateCode();
  protectNativeCode();

  if ( listingFile ) {
    fclose( listingFile );
    listingFile = nullptr;
  }
  executeCode();
  // do some atexit/cleanup stuff for the generated code:
  grTerm();
}

void
parseArgs( int argc, char* argv[] )
{
  int arg = 1;
  for ( ; arg < argc; ++arg ) {
    char* astr = argv[arg];
    if ( strcmp( astr, "-h" ) == 0 ) {
      usage( 0 );
    } else if ( strcmp( astr, "-l" ) == 0 ) {
      optionListing = true;
    } else if ( strcmp( astr, "-u" ) == 0 ) {
      optionDumpCode = true;
    } else if ( astr[0] == '-' ) {
      usage( 1 );
    } else {
      break;
    }
  }
  if ( arg == argc ) {
    printf( "Error: missing filename\n" );
    usage( 1 );
  } else if ( arg != argc-1 ) {
    printf( "Error: only one filename allowed\n" );
    usage( 1 );
  }
  filename = argv[arg];
}


void
usage( int status )
{
  printf( "Usage:   jit [-l] [-u] <file>\n" );
  printf( "  -l : create listing\n" );
  printf( "  -u : dump machine code\n" );
  exit( status );
}


// ----------------------------------------------------------------------------

// A quick initial pass through the tCode.
// Define the tCode address of each label.  We'll need these to find case tables.
// Knowing all the tCode label definitions up front will also help us
// simplify jump chains when we generate code.

void
defineLabels()
{
  int maxLabel = 0;

  for ( int pc = 0; pc < tCodeLen; ) {
    switch ( tCode[pc] ) {
      case tLabel: {
          int label = tCode[pc+1];
          maxLabel = std::max( maxLabel, label );
          defineLabelTCode( label, pc+2 );
        }
        break;
      case tLabelAlias: {
          int label = tCode[pc+1];
          maxLabel = std::max( maxLabel, label );
          defineLabelAlias( label, tCode[pc+2] );
        }
        break;
      case tLabelExtern: {
          int label = tCode[pc+1];
          maxLabel = std::max( maxLabel, label );
          defineLabelExtern( label, data + tCode[pc+2] );
        }
        break;
      default:
        break;
    }
    pc += tcodeInstrSize( tCode[pc] );
  }

  // Jit may issue additional labels itself, starting with this
  nextLabel = maxLabel + 1;
}


// Create a new label for use by Jit.
// The label has no tcode address, or native address at this time.
//
int
labelNew()
{
  return nextLabel++;
}


// ----------------------------------------------------------------------------

//
// Builds up an x86-64 instruction to be emitted.
//
class Instr
{
public:
  Instr( char opcode );
  Instr( char opcode1, char opcode2 );
  Instr( char opcode1, char opcode2, char opcode3 );

  // provide opcode2 if not done via constructor
  Instr& opcode2( char opcode2 );

  // provide opcode3 if not done via constructor
  Instr& opcode3( char opcode3 );

  // provide a prefix.  This goes first (before any rex prefix).
  Instr& prefix( char prefix );

  // requests the REX.W flag, converting a 32-bit instruction to 64-bit.
  Instr& w();

  // specifies an opcode extension, which lives in the reg field of the ModR/M byte.
  Instr& opc( char opcodeExtension );

  // specifies the reg field of ModR/M.
  Instr& reg( Register* r );

  // specifies the reg field of ModR/M.  The register is 8 bits (the low 8 bits of reg).
  Instr& reg8( Register* r );

  // specifies an immediate reg (not a memory reference) that goes in the r/m field of ModR/M.
  // This is used for a second register in instructions that take two immediate registers,
  // or the single immediate register in instructions that occupy the reg field with an opcode extension.
  Instr& rmReg( Register* r );

  // specifies an immediate reg in the r/m field of ModR/M.  The register is 8 bits (the low 8 bits).
  Instr& rmReg8( Register* r );

  // specifies an immediate reg that's encoded in low bits of the opcode itself, rather than ModR/M.
  Instr& regInOpc( Register* r );

  // Specifies a memory reference, involving a base register and an offset.
  // The register goes in the r/m field of ModR/M.
  Instr& mem( Register* base, int offset );

  // specifies a memory reference relative to the address of the instruction following this one.
  Instr& memRipRelative( char* targetAddr );

  // specifies an immediate 8-bit value.
  Instr& imm8( char value );

  // specifies an immediate 32-bit value.   For 64-bit instructions, the cpu will sign-extend it.
  Instr& imm32( int value );

  // specifies an immediate 64-bit value.
  Instr& imm64( long value );

  void emit() const;

private:
  char _opcode1 = 0;
  char _opcode2 = 0;
  char _opcode3 = 0;

  char _prefix = 0;
  bool _wide = false;
  char _opcodeExtension = 0;
  char* _memRipRelative = nullptr;
  char _imm8 = 0;
  int _imm32 = 0;
  long _imm64 = 0;
  Register* _reg = nullptr;
  Register* _base = nullptr;
  Register* _regInOpc = nullptr;
  bool _reg8 = false;
  bool _rmReg8 = false;
  int _offset = 0;

  bool _haveOpcode2 = false;
  bool _haveOpcode3 = false;
  bool _havePrefix = false;
  bool _haveOpcodeExtension = false;
  bool _haveMemRipRelative = false;
  bool _haveImm8 = false;
  bool _haveImm32 = false;
  bool _haveImm64 = false;
  bool _baseIsImmediate = false;
};


Instr::Instr( char opcode )
  : _opcode1( opcode )
{}

Instr::Instr( char opcode1, char opcode2 )
  : _opcode1( opcode1 ), _opcode2( opcode2 ), _haveOpcode2( true )
{}

Instr::Instr( char opcode1, char opcode2, char opcode3 )
  : _opcode1( opcode1 ), _opcode2( opcode2 ), _opcode3( opcode3 ),
    _haveOpcode2( true ), _haveOpcode3( true )
{}

// Can provide second opcode byte separately
Instr&
Instr::opcode2( char opcode2 ) {
  _opcode2 = opcode2;
  _haveOpcode2 = true;
  return *this;
}

// Can provide third opcode byte separately
Instr&
Instr::opcode3( char opcode3 ) {
  _opcode3 = opcode3;
  _haveOpcode3 = true;
  return *this;
}

// Specify a prefix byte.  This will go before any REX prefix.
Instr&
Instr::prefix( char prefix ) {
  // only allowing for one prefix at the moment
  assert( !_havePrefix );
  _prefix = prefix;
  _havePrefix = true;
  return *this;
}


// requests the REX.W flag, converting a 32-bit instruction to 64-bit.
Instr&
Instr::w()
{
  _wide = true;
  return *this;
}


// specifies an opcode extension, which lives in the reg field of the ModR/M byte.
Instr&
Instr::opc( char opcodeExtension )
{
  assert( !_reg );   // if failed: maybe  you wanted rmReg() for the other call.
  _opcodeExtension = opcodeExtension;
  _haveOpcodeExtension = true;
  return *this;
}


// specifies the reg field of ModR/M.
Instr&
Instr::reg( Register* r )
{
  assert( !_haveOpcodeExtension );  // if failed: maybe you wanted rmReg()
  _reg = r;
  return *this;
}


// specifies the reg field of ModR/M.  The register is 8 bits (the low 8 bits of reg).
// (We need to know it's 8-bit, to be able to encode low byte of rsi/rdi via REX prefix.)
//
Instr&
Instr::reg8( Register* r )
{
  reg( r );
  _reg8 = true;
  return *this;
}


// specifies an immediate reg (not a memory reference) that goes in the r/m field of ModR/M.
// This is used for a second register in instructions that take two immediate registers,
// or the single immediate register in instructions that occupy the reg field with an opcode extension.
Instr&
Instr::rmReg( Register* r )
{
  _base = r;
  _baseIsImmediate = true;
  return *this;
}


// specifies an immediate reg (not a memory reference) that goes in the r/m field of ModR/M.
// (We need to know it's 8-bit, to be able to encode low byte of rsi/rdi via REX prefix.)
//
Instr&
Instr::rmReg8( Register* r )
{
  rmReg( r );
  _rmReg8 = true;
  return *this;
}


Instr&
Instr::regInOpc( Register* r )
{
  _regInOpc = r;
  return *this;
}


// Specifies a memory reference, involving a base register and an offset.
// The register goes in the r/m field of ModR/M.
Instr&
Instr::mem( Register* base, int offset )
{
  _base = base;
  _offset = offset;
  return *this;
}


// specifies a memory reference relative to the address of the instruction following this one.
Instr&
Instr::memRipRelative( char* targetAddr )
{
  _memRipRelative = targetAddr;
  _haveMemRipRelative = true;
  return *this;
}


// specifies an immediate 8-bit value.
Instr&
Instr::imm8( char value )
{
  _imm8 = value;
  _haveImm8 = true;
  return *this;
}


// specifies an immediate 32-bit value.   For 64-bit instructions, the cpu will sign-extend it.
Instr&
Instr::imm32( int value )
{
  _imm32 = value;
  _haveImm32 = true;
  return *this;
}

Instr&
Instr::imm64( long value )
{
  _imm64 = value;
  _haveImm64 = true;
  return *this;
}

void
Instr::emit() const
{
  if ( _havePrefix ) {
    outB( _prefix );
  }

  // optional REX prefix.
  // This gives access to registers r8-r15, and/or reequests 64-bit data width.
  // If opcode encodes register itself, the upper bit goes in rex.base
  Register* rexBase = _regInOpc ? _regInOpc : _base;
  emitRex( _wide, _reg, rexBase, _reg8, _rmReg8 );

  // opcode
  if ( _regInOpc ) {
    outB( _opcode1 | regIdLowBits( _regInOpc->_nativeId ) );
  } else {
    outB( _opcode1 );
  }
  if ( _haveOpcode2 ) {
    outB( _opcode2 );
  }
  if ( _haveOpcode3 ) {
    outB( _opcode3 );
  }

  // For rip-relative instructions, we'll need the number of bytes between the
  // Mod/RM byte and the next instruction.  That's just the imm* argument, if any.
  int immBytes = 0;
  if ( _haveImm8 ) {
    immBytes = 1;
  } else if ( _haveImm32 ) {
    immBytes = 4;
  } else if ( _haveImm64 ) {
    immBytes = 8;
  }

  // ModR/M byte and offset
  if ( _reg || _base || _haveOpcodeExtension || _haveMemRipRelative ) {
    if ( _haveOpcodeExtension ) {
      if ( _haveMemRipRelative ) {
        emitModRM_OpcRMMemRipRelative( _opcodeExtension, _memRipRelative, immBytes );
      } else if ( _baseIsImmediate ) {
        emitModRM_OpcRMReg( _opcodeExtension, _base );
      } else {
        emitModRM_OpcRMMem( _opcodeExtension, _base, _offset );
      }
    } else {
      if ( _haveMemRipRelative ) {
        emitModRMMemRipRelative( _reg, _memRipRelative, immBytes );
      } else if ( _baseIsImmediate ) {
        emitModRM_RegReg( _reg, _base );
      } else {
        emitModRMMem( _reg, _base, _offset );
      }
    }
  }

  // Immediate value
  if ( _haveImm8 ) {
    outB( _imm8 );
  } else if ( _haveImm32 ) {
    outI( _imm32 );
  } else if ( _haveImm64 ) {
    outL( _imm64 );
  }
}


void
emit( const Instr& instr )
{
  instr.emit();
}


// Instruction templates are based on the Intel x86-64 instruction docs.
// I don't make templates for all possible variants, though.
//
// Size is the default data size that the instruction operates on.
// Instructions with default size 32 can also operate on 64 bits
// by issuing the REX.W prefix.  (If some instruction doesn't permit this,
// I'll add a way to disable it.)
//
// One of the emit methods can take my high-level operands,
// and choose the matching instruction template, and emit that.
//
class InstrTempl {
public:
  InstrTempl( int size, int opcode );
  InstrTempl( int size, int opcode1, int opcode2 );
  InstrTempl( int size, int opcode1, int opcode2, int opcode3 );

  InstrTempl& opc( int extension );  // opcode extension
  InstrTempl& prefix( int prefix );  // prefix (goes first, before any rex prefix)
  InstrTempl& RM();  // operands r, r/m
  InstrTempl& MR();  // operands r/m, r
  InstrTempl& MI();  // operands r/m, imm
  InstrTempl& M();   // single operand r/m
  InstrTempl& ySize( int ySize );  // override size of y operand to given #bits rather than _size

  void emit( const Operand& x, const Operand& y ) const;
  void emit( const Operand& x ) const;

  // this should go in a templates class instead.
  static const InstrTempl* findMatch( const std::vector<InstrTempl>& templates, const Operand& x, const Operand& y );
  static const InstrTempl* findMatch( const std::vector<InstrTempl>& templates, const Operand& x );

private:
  
  // helper for filling out a template.  translate an operand for a particular template field, generating instr data.
  void operandToI( Instr& instr, const Operand& operand, int size ) const;
  void operandToR( Instr& instr, const Operand& operand, int size ) const;
  void operandToM( Instr& instr, const Operand& operand, int size ) const;

  // DATA
  int _size;        // 8 or 32 (for 32/64)
  int _opcode;
  int _opcode2 = 0;
  int _opcode3 = 0;
  int _prefix = 0;
  bool _haveOpcode2 = false;
  bool _haveOpcode3 = false;
  bool _havePrefix = false;
  int _xType = 0;   // 1=I, 2=R, 3=M   0 = no x
  int _yType = 0;   // 1=I, 2=R, 3=M   0 = no y
  int _opc = -1;     // opcode extension, or -1
  int _ySize = 0;  // usually size, unless overridden
};

InstrTempl::InstrTempl( int size, int opcode )
  : _size( size ), _ySize( size ), _opcode( opcode )
{}

InstrTempl::InstrTempl( int size, int opcode1, int opcode2 )
  : _size( size ), _ySize( size ), _opcode( opcode1 ), _opcode2( opcode2 ), _haveOpcode2( true )
{}

InstrTempl::InstrTempl( int size, int opcode1, int opcode2, int opcode3 )
  : _size( size ), _ySize( size ),
    _opcode( opcode1 ), _opcode2( opcode2 ), _opcode3( opcode3 ),
    _haveOpcode2( true ), _haveOpcode3( true )
{}

InstrTempl&
InstrTempl::prefix( int prefix ) {
  _prefix = prefix;
  _havePrefix = true;
  return *this;
}

InstrTempl&
InstrTempl::opc( int opcodeExtension ) {
  _opc = opcodeExtension;
  return *this;
}

InstrTempl&
InstrTempl::RM() {
  _xType = 2;
  _yType = 3;
  return *this;
}

InstrTempl&
InstrTempl::MR() {
  _xType = 3;
  _yType = 2;
  return *this;
}

InstrTempl&
InstrTempl::MI() {
  _xType = 3;
  _yType = 1;
  return *this;
}

InstrTempl&
InstrTempl::M() {
  _xType = 3;
  _yType = 0;
  return *this;
}

InstrTempl&
InstrTempl::ySize( int ySize ) {
  _ySize = ySize;
  return *this;
}


// private helper: classify an operand for the template mechanism.
//
void
classify( const Operand& operand, int& size, int& templType )
{
  switch ( operand.size() ) {
    case 1:
      size = 8;
      break;
    case 4:
      size = 32;
      break;
    case 8:
      size = 64;
      break;
    default:
      fatal( "classify: unexpected operand size\n" );
  }

  templType = 0;
  if ( operand.isMem() ) {
    templType = 3;
  } else if ( operand.isReg() ) {
    templType = 2;
  } else if ( operand.isConst() ) {
    templType = 1;
  } else {
    toDo( "classify: unexpected operand type\n" );
  }
}


// For private use by InstrTempls, when I have a class for that.
const InstrTempl*
InstrTempl::findMatch( const std::vector<InstrTempl>& templates, const Operand& x, const Operand& y )
{
  // classify the operands
  int xSize, ySize;
  int xType, yType;
  classify( x, xSize, xType );
  classify( y, ySize, yType );

  // find the matching template
  for ( auto& templ : templates ) {

    if ( xSize == templ._size ||
         ( xSize == 64 && templ._size == 32 ) ) {
      // x size matches
    } else {
      continue;
    }

    if ( ySize == templ._ySize ||
         ( ySize == 64 && templ._ySize == 32 ) ||
         yType == 1 ) {
      // y size matches
      // ConstI can fit imm8 or imm32 templates,
      // because we don't have Const8 at the moment.
      // ConstI can also fit 64-bit template because such instructions can
      // sign extend imm32 to 64 bit.
    } else {
      continue;
    }

    if ( ( xType == templ._xType ) &&
         ( yType == templ._yType ) ) {
      return &templ;
    }

    // a R operand can fit in an M template too.
    if ( ( xType == 2 && templ._xType == 3 &&
           yType == templ._yType ) ||
         ( yType == 2 && templ._yType == 3 &&
           xType == templ._xType ) ) {
      return &templ;
    }
  }

  return nullptr;
}


// Find a matching template for a 1-operand instruction.
//
const InstrTempl*
InstrTempl::findMatch( const std::vector<InstrTempl>& templates, const Operand& x )
{
  // classify the operands
  int xSize;
  int xType;
  classify( x, xSize, xType );

  // find the matching template
  for ( auto& templ : templates ) {

    if ( xSize == templ._size ||
         ( xSize == 64 && templ._size == 32 ) ) {
      // x size matches
    } else {
      continue;
    }

    if ( ( xType == templ._xType ) &&
         ( templ._yType == 0 ) ) {
      return &templ;
    }

    // a R operand can fit in an M template too.
    if ( ( xType == 2 && templ._xType == 3 ) &&
         ( templ._yType == 0 ) ) {
        return &templ;
    }
  }

  return nullptr;
}


void
InstrTempl::operandToI( Instr& instr, const Operand& operand, int templSize ) const
{
  if ( operand._kind != jit_Operand_Kind_ConstI ) {
    fatal( "InstrTempl::operandToI unexpected operand\n" );
  }

  switch ( templSize ) {
    case 8:
      instr.imm8( operand._value );
      break;
    case 32:
      instr.imm32( operand._value );
      break;
    default:
      fatal( "InstrTempl::operandToI unexpected size\n" );
  }
}


void
InstrTempl::operandToR( Instr& instr, const Operand& operand, int templSize ) const
{
  switch ( operand._kind ) {
    case jit_Operand_Kind_RegB:
      assert( templSize == 8 );
      instr.reg8( operand._reg );
      break;
    case jit_Operand_Kind_RegI:
      assert( templSize == 32 );
      instr.reg( operand._reg );
      break;
    case jit_Operand_Kind_RegP:
    case jit_Operand_Kind_RegD:
      assert( templSize == 32 || templSize == 64 );
      instr.reg( operand._reg );
      break;
    default:
      fatal( "InstrTempl::operandToR unexpected operand\n" );
  }
}


void
InstrTempl::operandToM( Instr& instr, const Operand& operand, int templSize ) const
{
  switch ( operand._kind ) {
    case jit_Operand_Kind_GlobalB:
    case jit_Operand_Kind_GlobalI:
    case jit_Operand_Kind_GlobalP:
    case jit_Operand_Kind_GlobalD:
    case jit_Operand_Kind_Addr_Global:
      instr.memRipRelative( &data[ operand._value ] );
      break;

    case jit_Operand_Kind_LocalB:
    case jit_Operand_Kind_LocalI:
    case jit_Operand_Kind_LocalP:
    case jit_Operand_Kind_LocalD:
    case jit_Operand_Kind_Addr_Local:
      instr.mem( regRbp, operand._value );
      break;

    case jit_Operand_Kind_ParamB:
    case jit_Operand_Kind_ParamI:
    case jit_Operand_Kind_ParamP:
    case jit_Operand_Kind_ParamD:
    case jit_Operand_Kind_Addr_Param:
      instr.mem( regRbp, operand._value + FRAME_PARAMS_OFFSET );
      break;

    case jit_Operand_Kind_ActualB:
    case jit_Operand_Kind_ActualI:
    case jit_Operand_Kind_ActualP:
    case jit_Operand_Kind_ActualD:
    case jit_Operand_Kind_Addr_Actual:
      instr.mem( regRsp, operand._value );
      break;

    case jit_Operand_Kind_RegB:
      assert( templSize == 8 );
      instr.rmReg8( operand._reg );
      break;
    case jit_Operand_Kind_RegI:
      assert( templSize == 32 );
      instr.rmReg( operand._reg );
      break;
    case jit_Operand_Kind_RegP:
      assert( templSize == 32 || templSize == 64 );
      instr.rmReg( operand._reg );
      break;
    case jit_Operand_Kind_RegD:
      assert( templSize == 32 || templSize == 64 );
      instr.rmReg( operand._reg );
      break;

    case jit_Operand_Kind_RegP_DerefB:
    case jit_Operand_Kind_RegP_DerefI:
    case jit_Operand_Kind_RegP_DerefP:
    case jit_Operand_Kind_RegP_DerefD:
    case jit_Operand_Kind_Addr_Reg_Offset:
      instr.mem( operand._reg, operand._value );
      break;

    default:
      fatal( "InstrTempl::operandToM unexpected operand\n" );
   }
}


void
emit( const std::vector<InstrTempl>& templates, const Operand& x, const Operand& y )
{
  const InstrTempl* templ = InstrTempl::findMatch( templates, x, y );
  if ( !templ ) {
    toDo( "template lookup: didn't find a match\n" );
    return;
  }
  templ->emit( x, y );
}


void
emit( const std::vector<InstrTempl>& templates, const Operand& x )
{
  const InstrTempl* templ = InstrTempl::findMatch( templates, x );
  if ( !templ ) {
    toDo( "template lookup: didn't find a match\n" );
    return;
  }
  templ->emit( x );
}


void
InstrTempl::emit( const Operand& x, const Operand& y ) const
{
  // create an Instr using the template and operands
  Instr instr( _opcode );
  if ( _haveOpcode2 ) {
    instr.opcode2( _opcode2 );
  }
  if ( _haveOpcode3 ) {
    instr.opcode3( _opcode3 );
  }
  if ( _havePrefix ) {
    instr.prefix( _prefix );
  }
  // Are we extending a 32-bit template to a 64 bit operation?
  // (Note operand size is in bytes.)
  if ( ( _size == 32 ) && ( x.size() == 8 ) ) {
    instr.w();
  }
  if ( _opc >= 0 ) {
    instr.opc( _opc );
  }
  switch( _xType ) {
    case 2:
      operandToR( instr, x, _size );
      break;
    case 3:
      operandToM( instr, x, _size );
      break;
    default:
      fatal( "unexpected type in template\n" );
  }
  switch( _yType ) {
    case 1:
      operandToI( instr, y, _ySize );
      break;
    case 2:
      operandToR( instr, y, _ySize );
      break;
    case 3:
      operandToM( instr, y, _ySize );
      break;
    default:
      fatal( "unexpected type in template\n" );
  }

  ::emit( instr );
}


void
InstrTempl::emit( const Operand& x ) const
{
  // create an Instr using the template and operand
  Instr instr( _opcode );
  if ( _haveOpcode2 ) {
    instr.opcode2( _opcode2 );
  }

  // Is this a sixty-four bit operation?  (Operand size is in bytes.)
  if ( x.size() == 8 ) {
    instr.w();
  }
  if ( _opc >= 0 ) {
    instr.opc( _opc );
  }
  switch( _xType ) {
    case 2:
      operandToR( instr, x, _size );
      break;
    case 3:
      operandToM( instr, x, _size );
      break;
    default:
      fatal( "unexpected type in template\n" );
  }

  ::emit( instr );
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
int* tCodePc = nullptr;

void
generateCode()
{
  // Fold constants? May want to turn off during some unit testing.
  bool doConst = true;

  int jitSp = 0;

  tCodePc = tCode;
  int* tCodeEnd = tCode + tCodeLen;

  for ( tCodePc = tCode; tCodePc < tCodeEnd; ) {

    switch ( *tCodePc++ ) {

      case tPushGlobalI : {
          operandStack.emplace_back( jit_Operand_Kind_GlobalI, *tCodePc++ );
        }
        break;
      case tPushGlobalB : {
          operandStack.emplace_back( jit_Operand_Kind_GlobalB, *tCodePc++ );
        }
        break;
      case tPushGlobalP : {
          operandStack.emplace_back( jit_Operand_Kind_GlobalP, *tCodePc++ );
        }
        break;
      case tPushGlobalD : {
          operandStack.emplace_back( jit_Operand_Kind_GlobalD, *tCodePc++ );
        }
        break;
      case tPushLocalI : {
          operandStack.emplace_back( jit_Operand_Kind_LocalI, *tCodePc++ );
        }
        break;
      case tPushLocalB : {
          operandStack.emplace_back( jit_Operand_Kind_LocalB, *tCodePc++ );
        }
        break;
      case tPushLocalP : {
          operandStack.emplace_back( jit_Operand_Kind_LocalP, *tCodePc++ );
        }
        break;
      case tPushLocalD : {
          operandStack.emplace_back( jit_Operand_Kind_LocalD, *tCodePc++ );
        }
        break;
      case tPushParamI : {
          operandStack.emplace_back( jit_Operand_Kind_ParamI, *tCodePc++ );
        }
        break;
      case tPushParamB : {
          operandStack.emplace_back( jit_Operand_Kind_ParamB, *tCodePc++ );
        }
        break;
      case tPushParamP : {
          operandStack.emplace_back( jit_Operand_Kind_ParamP, *tCodePc++ );
        }
        break;
      case tPushParamD : {
          operandStack.emplace_back( jit_Operand_Kind_ParamD, *tCodePc++ );
        }
        break;
      case tPushUpLocalI : {
          int uplevels = *tCodePc++;
          int offset = *tCodePc++;
          Register* frameReg = getUpFrame( uplevels );
          Operand x( jit_Operand_Kind_RegP_DerefI, frameReg, offset );
          // For now, deref is only understood by emitMov, so need to get var into a register
          operandIntoReg( x );
          operandStack.push_back( x );
        }
        break;
      case tPushUpLocalB : {
          int uplevels = *tCodePc++;
          int offset = *tCodePc++;
          Register* frameReg = getUpFrame( uplevels );
          Operand x( jit_Operand_Kind_RegP_DerefB, frameReg, offset );
          // For now, deref is only understood by emitMov, so need to get var into a register
          operandIntoReg( x );
          operandStack.push_back( x );
        }
        break;
      case tPushUpLocalP : {
          int uplevels = *tCodePc++;
          int offset = *tCodePc++;
          Register* frameReg = getUpFrame( uplevels );
          Operand x( jit_Operand_Kind_RegP_DerefP, frameReg, offset );
          // For now, deref is only understood by emitMov, so need to get var into a register
          operandIntoReg( x );
          operandStack.push_back( x );
        }
        break;
      case tPushUpLocalD : {
          int uplevels = *tCodePc++;
          int offset = *tCodePc++;
          Register* frameReg = getUpFrame( uplevels );
          Operand x( jit_Operand_Kind_RegP_DerefD, frameReg, offset );
          // For now, deref is only understood by emitMov, so need to get var into a register
          operandIntoFloatReg( x );
          operandStack.push_back( x );
        }
        break;
      case tPushUpParamI : {
          int uplevels = *tCodePc++;
          int offset = *tCodePc++;
          Register* frameReg = getUpFrame( uplevels );
          Operand x( jit_Operand_Kind_RegP_DerefI, frameReg, offset + FPO );
          // For now, deref is only understood by emitMov, so need to get var into a register
          operandIntoReg( x );
          operandStack.push_back( x );
        }
        break;
      case tPushUpParamB : {
          int uplevels = *tCodePc++;
          int offset = *tCodePc++;
          Register* frameReg = getUpFrame( uplevels );
          Operand x( jit_Operand_Kind_RegP_DerefB, frameReg, offset + FPO );
          // For now, deref is only understood by emitMov, so need to get var into a register
          operandIntoReg( x );
          operandStack.push_back( x );
        }
        break;
      case tPushUpParamP : {
          int uplevels = *tCodePc++;
          int offset = *tCodePc++;
          Register* frameReg = getUpFrame( uplevels );
          Operand x( jit_Operand_Kind_RegP_DerefP, frameReg, offset + FPO );
          // For now, deref is only understood by emitMov, so need to get var into a register
          operandIntoReg( x );
          operandStack.push_back( x );
        }
        break;
      case tPushUpParamD : {
          int uplevels = *tCodePc++;
          int offset = *tCodePc++;
          Register* frameReg = getUpFrame( uplevels );
          Operand x( jit_Operand_Kind_RegP_DerefD, frameReg, offset + FPO );
          // For now, deref is only understood by emitMov, so need to get var into a register
          operandIntoFloatReg( x );
          operandStack.push_back( x );
        }
        break;
      case tPushConstI : {
          operandStack.emplace_back( jit_Operand_Kind_ConstI, *tCodePc++ );
        }
        break;
      case tPushConstD : {
          long longVal = *(uint32_t*) tCodePc++;
          longVal |= ( (long) *(uint32_t*) tCodePc++ ) << 32;
          double val = *(double*) &longVal;
          operandStack.emplace_back( jit_Operand_Kind_ConstD, val );
        }
        break;
      case tPushAddrGlobal : {
          operandStack.emplace_back( jit_Operand_Kind_Addr_Global, *tCodePc++ );
        }
        break;
      case tPushAddrLocal : {
          operandStack.emplace_back( jit_Operand_Kind_Addr_Local, *tCodePc++ );
        }
        break;
      case tPushAddrParam : {
          operandStack.emplace_back( jit_Operand_Kind_Addr_Param, *tCodePc++ );
        }
        break;
      case tPushAddrActual : {
          operandStack.emplace_back( jit_Operand_Kind_Addr_Actual, *tCodePc++ );
        }
        break;
      case tPushAddrUpLocal : {
          int uplevels = *tCodePc++;
          int offset = *tCodePc++;
          Register* frameReg = getUpFrame( uplevels );
          operandStack.emplace_back( jit_Operand_Kind_Addr_Reg_Offset, frameReg, offset );
        }
        break;
      case tPushAddrUpParam : {
          int uplevels = *tCodePc++;
          int offset = *tCodePc++;
          Register* frameReg = getUpFrame( uplevels );
          operandStack.emplace_back( jit_Operand_Kind_Addr_Reg_Offset, frameReg, offset + FPO );
        }
        break;
      case tSwap : {
          Operand y = operandStack.back();  operandStack.pop_back();
          Operand x = operandStack.back();  operandStack.pop_back();
          operandStack.push_back( y );
          operandStack.push_back( x );
        }
        break;
      case tFetchI : {
          Operand x = operandStack.back();   operandStack.pop_back();
          // x is a pointer to a value of size 4.
          // Make x refer to the pointed-to value
          operandDeref( x, 4 );
          // If I allow jit_Operand_Kind_RegP_Deref* on the operand stack,
          // then we could just push x and be done.
          // The actual retrieval of the value can be included in the next operation.
          // But, for now, we only handle that operand kind in emitMov.
          // So we have to retrieve it into a register now.
          operandIntoReg( x );
          operandStack.push_back( x );
        }
        break;
      case tFetchB : {
          Operand x = operandStack.back();   operandStack.pop_back();
          // x is a pointer to a value of size 1.
          // Make x refer to the pointed-to value
          operandDeref( x, 1 );
          // If I allow jit_Operand_Kind_RegP_Deref* on the operand stack,
          // then we could just push x and be done.
          // The actual retrieval of the value can be included in the next operation.
          // But, for now, we only handle that operand kind in emitMov.
          // So we have to retrieve it into a register now.
          operandIntoReg( x );
          operandStack.push_back( x );
        }
        break;
      case tFetchP : {
          Operand x = operandStack.back();   operandStack.pop_back();
          // x is a pointer to a value of size 8.
          // Make x refer to the pointed-to value
          operandDeref( x, 8 );
          // If I allow jit_Operand_Kind_RegP_Deref* on the operand stack,
          // then we could just push x and be done.
          // The actual retrieval of the value can be included in the next operation.
          // But, for now, we only handle that operand kind in emitMov.
          // So we have to retrieve it into a register now.
          operandIntoReg( x );
          operandStack.push_back( x );
        }
        break;
      case tFetchD : {
          Operand x = operandStack.back();   operandStack.pop_back();
          // x is a pointer to a value of size 8.
          // Make x refer to the pointed-to value
          operandDerefFloat( x, 8 );
          // If I allow jit_Operand_Kind_RegP_Deref* on the operand stack,
          // then we could just push x and be done.
          // The actual retrieval of the value can be included in the next operation.
          // But, for now, we only handle that operand kind in emitMov.
          // So we have to retrieve it into a register now.
          operandIntoFloatReg( x );
          operandStack.push_back( x );
        }
        break;
      case tAssignI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();

          // Watching for assignment into actuals space,
          // to help with cdecl calls.  This assign is not affected.
          cdeclCheckForAssignToActual( x, jit_Operand_Kind_ActualI );

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
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();

          // Watching for assignment into actuals space,
          // to help with cdecl calls.  This assign is not affected.
          cdeclCheckForAssignToActual( x, jit_Operand_Kind_ActualB );

          // allow for y being flags
          operandFlagsToValue( y, 1 );
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
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();

          // Watching for assignment into actuals space,
          // to help with cdecl calls.  This assign is not affected.
          cdeclCheckForAssignToActual( x, jit_Operand_Kind_ActualP );

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
      case tAssignD : {
          // This is similar to tAssignP, except for how we deal with cdecl params.
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();

          // Watching for assignment into actuals space,
          // to help with cdecl calls.  This assign is not affected.
          cdeclCheckForAssignToActual( x, jit_Operand_Kind_ActualD );

          // x is a pointer to a float value of size 8.
          // Make x refer to the pointed-to value (still usable as target of mov)
          operandDerefFloat( x, 8 );
          // x will be a memory reference, so y must not also be in memory.
          operandNotMem( y );
          emitMov( x, y );
          x.release();
          y.release();
        }
        break;
      case tCopy : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();

          // Watching for assignment into actuals space,
          // to help with cdecl calls.
          // At the moment, we don't support complex param types in cdecl calls
          // at all.  (see cdeclParamsIntoRegisters.) So error on that.
          if ( ( x._kind == jit_Operand_Kind_Addr_Actual ) &&
               ( callInfos.size() > 0 ) &&
               ( callInfos.back()._cdecl == true ) ) {
            toDo( "cdecl method with complex parameters is not supported yet\n" );
          }

          int bytes = *tCodePc++;
          // copy bytes to dest=x from src=y
          // rep movsb needs particular registers.  rdi = dest, rsi = src, ecx = bytes
          operandIntoSpecificReg( x, regRdi, 8 );
          operandIntoSpecificReg( y, regRsi, 8 );
          forceAllocateReg( regRcx );
          Operand countReg( jit_Operand_Kind_RegI, regRcx );
          Operand countConst( jit_Operand_Kind_ConstI, bytes );
          emitMov( countReg, countConst );
          outB( 0xf3 );  // rep
          outB( 0xa4 );  // movsb
          // note rep movsb advances forwards or backwards depending on direction flag
          // (controlled by cld or std).  But the convention on windows at least is
          // that direction flag is always cleared (forwards) by the system & around interrupts.
          // Assume linux is same.  So I should be able to count on that.
          countReg.release();
          x.release();
          y.release();
        }
        break;

      case tCastBtoI : {
          Operand x = operandStack.back();  operandStack.pop_back();
          if ( x.isConst() ) {
            // We only have ConstI so no change.
            operandStack.push_back( x );
          } else {
            // Expect the front end to use this instruction appropriately
            assert( x.size() == 1 );
            if ( x.isReg() ) {
              // zero-extend in the current register.  Mov RegI, RegB  will do so.
              Operand result( jit_Operand_Kind_RegI, x._reg );
              emitMov( result, x );
              operandStack.push_back( result );
            } else {
              Operand result( jit_Operand_Kind_RegI, allocateReg() );
              // zero-extend from x
              emitMov( result, x );
              operandStack.push_back( result );
              x.release();
            }
          }
        }
        break;
      case tCastItoB : {
          Operand x = operandStack.back();  operandStack.pop_back();
          if ( x.isConst() ) {
            // We only have ConstI but it can act as a ConstB too.
            // Just truncate the value as a precaution.
            x._value &= 0xff;
            operandStack.push_back( x );
          } else {
            // I might have been able to load just the low byte from wherever x is,
            // but for now I'll just load the entire x into a register, then truncate it.
            operandIntoReg( x );
            // Actually I probably don't really need to truncate the register (zero-extending low byte)
            // at this time.  Just update the operand kind to RegB.
            // Any subsequent use of the register should either only take the low byte, or should
            // zero-extend it first It think.
            x._kind = jit_Operand_Kind_RegB;
            operandStack.push_back( x );
          }
        }
        break;
      case tCastItoD : {
          Operand x = operandStack.back();  operandStack.pop_back();
          if ( x.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstD, (double) x._value );
          } else {
            Operand result( jit_Operand_Kind_RegD, allocateFloatReg() );
            emitCvtIntToDouble( result, x );
            operandStack.push_back( result );
            x.release();
          }

        }
        break;
      case tIncI : {
          Operand x = operandStack.back();  operandStack.pop_back();
          if ( x.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value + 1 );
          } else {
            // x must be a regiser.  This is because I defined tInc to operate on
            // the top value in the expression stack, so can't inc a variable directly.
            operandIntoReg( x );
            emitInc( x );
            operandStack.push_back( x );
          }
        }
        break;
      case tDecI : {
          Operand x = operandStack.back();  operandStack.pop_back();
          if ( x.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value - 1 );
          } else {
            // x must be a register (see tInc).
            operandIntoReg( x );
            emitDec( x );
            operandStack.push_back( x );
          }
        }
        break;
      case tMultI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          int bits;
          if ( x.isConst() && !y.isConst() ) {
            swap( x, y );
          }
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( x._kind, x._value * y._value );
          } else if ( doConst && y.isConst() && y._value == 0 ) {
            operandStack.emplace_back( x._kind, 0 );
          } else if ( doConst && y.isConst() && y._value == 1 ) {
            operandStack.push_back( x );
          } else if ( y.isConst() && ( bits = isPowerOf2( y._value ) ) ) {
            operandIntoReg( x );
            Operand shiftBy( jit_Operand_Kind_ConstI, bits );
            emitShl( x, shiftBy );
            operandStack.push_back( x );
          } else {
            operandIntoReg( x );
            emitMult( x, y );
            operandStack.push_back( x );
          }
          y.release();
        }
        break;
      case tDivI : {
          // TO DO: this is the sort of place where my flaw regarding forceAllocate
          //   would show up.  I need to get x into edx:eax, but forceAllocateReg() after popping
          //   x and y off the stack would leave x or y out-of-date if they had been in
          //   an affected register.
          // To avoid the problem for now, I'll try to get things in place prior to the pop.
          // Specifically, move x while leaving y still on the stack, so y gets updated if needed.
          // Still need a solution for other instructions too.
          Operand x = operandStack[ operandStack.size()-2 ];
          Operand y = operandStack[ operandStack.size()-1 ];
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.pop_back();
            operandStack.pop_back();
            operandStack.emplace_back( x._kind, x._value / y._value );
          } else if ( doConst && y.isConst() && y._value == 1 ) {
            operandStack.pop_back();
            // leave x on the stack as-is
          } else {
            operandIntoSpecificReg( x, regRax, 4 );
            forceAllocateReg( regRdx );
            y = operandStack.back();   operandStack.pop_back();
            operandStack.pop_back();
            if ( y.isConst() ) {
              // emitDiv does not accept const y
              operandIntoReg( y );
            }
            // cdq to sign extend from eax to edx::eax
            emitCdq();
            emitDiv( x, y );  // leaves quotient in x, remainder in edx
            operandStack.push_back( x );
            regRdx->release();
            y.release();
          }
        }
        break;
      case tAddPI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isAddrOfVar() && y.isConst() ) {
            operandStack.emplace_back( x._kind, x._value + y._value );
          } else {
            operandKindAddrIntoReg( x );
            operandKindAddrIntoReg( y );
            // Sign-extend y to 64-bits so we can add it directly to x.
            operandExtendToP( y );
            // A little optimization
            if ( ( y._kind == jit_Operand_Kind_RegP ) &&
                 ( x._kind != jit_Operand_Kind_RegP ) ) {
              swap( x, y );
            }
            operandIntoReg( x );
            emitAdd( x, y );
            operandStack.push_back( x );
          }
          y.release();
        }
        break;
      case tAddI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( x._kind, x._value + y._value );
          } else {
            operandKindAddrIntoReg( x );
            operandKindAddrIntoReg( y );
            operandIntoRegCommutative( x, y );
            emitAdd( x, y );
            operandStack.push_back( x );
          }
          y.release();
        }
        break;
      case tSubP : {
          // P - P produces I, at least in my current definition.
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isAddrOfVar() && y.isAddrOfVar() && x._kind == y._kind ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value - y._value );
          } else {
            operandKindAddrIntoReg( x );
            operandKindAddrIntoReg( y );
            operandIntoReg( x );
            emitSub( x, y );
            operandStack.emplace_back( jit_Operand_Kind_RegI, x._reg );
          }
          y.release();
        }
        break;
      case tSubPI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isAddrOfVar() && y.isConst() ) {
            operandStack.emplace_back( x._kind, x._value - y._value );
          } else {
            operandKindAddrIntoReg( x );
            operandKindAddrIntoReg( y );
            // Sign-extend y to 64-bits so we can subtract it directly from x.
            operandExtendToP( y );
            operandIntoReg( x );
            emitSub( x, y );
            operandStack.push_back( x );
          }
          y.release();
        }
        break;
      case tSubI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( x._kind, x._value - y._value );
          } else {
            operandKindAddrIntoReg( x );
            operandKindAddrIntoReg( y );
            operandIntoReg( x );
            emitSub( x, y );
            operandStack.push_back( x );
          }
          y.release();
        }
        break;
      case tNegI : {
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() ) {
            operandStack.emplace_back( x._kind, -x._value );
          } else {
            operandIntoReg( x );
            emitNeg( x );
            operandStack.push_back( x );
          }
        }
        break;

      case tMultD : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          int bits;
          if ( x.isConst() && !y.isConst() ) {
            swap( x, y );
          }
          if ( doConst && x.isConst() && y.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            assert( y._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( x._kind, x._double * y._double );
          } else if ( doConst && y.isConst() && y._double == 0.0 ) {
            operandStack.emplace_back( x._kind, 0.0 );
          } else if ( doConst && y.isConst() && y._double == 1.0 ) {
            operandStack.push_back( x );
          } else {
            operandIntoFloatReg( x );
            emitMultFloat( x, y );
            operandStack.push_back( x );
          }
          y.release();
        }
        break;
      case tDivD : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            assert( y._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( x._kind, x._double / y._double );
          } else if ( doConst && y.isConst() && y._double == 1.0 ) {
            operandStack.push_back( x );
          } else {
            operandIntoFloatReg( x );
            emitDivFloat( x, y );
            operandStack.push_back( x );
          }
          y.release();
        }
        break;
      case tAddD : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            assert( y._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( x._kind, x._double + y._double );
          } else {
            operandIntoFloatRegCommutative( x, y );
            emitAddFloat( x, y );
            operandStack.push_back( x );
          }
          y.release();
        }
        break;
      case tSubD : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            assert( y._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( x._kind, x._double - y._double );
          } else {
            operandIntoFloatReg( x );
            emitSubFloat( x, y );
            operandStack.push_back( x );
          }
          y.release();
        }
        break;
      case tNegD : {
          // There doesn't seem to be a floating point neg instruction.
          // All I really need to do is flip the sign bit.
          // But, for now I'll subtract from 0.
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( x._kind, -x._double );
          } else {
            Operand result( jit_Operand_Kind_RegD, allocateFloatReg() );
            // Get zero into result
            emitPxor( result, result );
            emitSubFloat( result, x );
            x.release();
            operandStack.push_back( result );
          }
        }
        break;

      case tNot : {
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() ) {
            operandStack.emplace_back( x._kind, x._value == 0 ? 1 : 0 );
          } else if ( x.isFlags() ) {
            ConditionFlags flags = x._flags;
            negateConditionFlag( flags );
            operandStack.emplace_back( jit_Operand_Kind_Flags, flags );
          } else {
            // x = 0  (leaving result in flags)
            Operand zero( jit_Operand_Kind_ConstI, 0 );
            Operand result = operandCompare( x, zero, FlagE );
            x.release();
            // Oh, but subsequent expr instructions might not be prepared to see Flags yet.
            // I might be obligated to produce 0/1 for now.
            // TO DO: try to leave as flags, because this is easily followed by conditional jmp
            operandFlagsToValue( result, 1 );
            operandStack.push_back( result );
          }
        }
        break;

      // Type "B" and "P" are unsigned, so tGreaterB and tGreaterP etc are unsigned comparisons.
      // Type "I" is signed, so tGreaterI etc are signed comparisons.
      // operandCompare() and emitCmp() don't care about that, they handle everything.
      // It just affects the Flag* that we say is a true result.

      case tEqualB : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value == y._value ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagE ) );
          }
          x.release();
          y.release();
        }
        break;
      case tNotEqualB : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value != y._value ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagNE ) );
          }
          x.release();
          y.release();
        }
        break;
      case tGreaterB : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, uint8_t(x._value) > uint8_t(y._value) ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagA ) );
          }
          x.release();
          y.release();
        }
        break;
      case tLessB : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, uint8_t(x._value) < uint8_t(y._value) ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagB ) );
          }
          x.release();
          y.release();
        }
        break;
      case tGreaterEqualB : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, uint8_t(x._value) >= uint8_t(y._value) ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagAE ) );
          }
          x.release();
          y.release();
        }
        break;
      case tLessEqualB : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, uint8_t(x._value) <= uint8_t(y._value) ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagBE ) );
          }
          x.release();
          y.release();
        }
        break;

      case tEqualI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value == y._value ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagE ) );
          }
          x.release();
          y.release();
        }
        break;
      case tNotEqualI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value != y._value ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagNE ) );
          }
          x.release();
          y.release();
        }
        break;
      case tGreaterI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value > y._value ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagG ) );
          }
          x.release();
          y.release();
        }
        break;
      case tLessI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value < y._value ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagL ) );
          }
          x.release();
          y.release();
        }
        break;
      case tGreaterEqualI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value >= y._value ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagGE ) );
          }
          x.release();
          y.release();
        }
        break;
      case tLessEqualI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._value <= y._value ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompare( x, y, FlagLE ) );
          }
          x.release();
          y.release();
        }
        break;

      case tEqualP : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          operandStack.push_back( operandCompare( x, y, FlagE ) );
          x.release();
          y.release();
        }
        break;
      case tNotEqualP : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          operandStack.push_back( operandCompare( x, y, FlagNE ) );
          x.release();
          y.release();
        }
        break;
      case tGreaterP : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          operandStack.push_back( operandCompare( x, y, FlagA ) );
          x.release();
          y.release();
        }
        break;
      case tLessP : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          operandStack.push_back( operandCompare( x, y, FlagB ) );
          x.release();
          y.release();
        }
        break;
      case tGreaterEqualP : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          operandStack.push_back( operandCompare( x, y, FlagAE ) );
          x.release();
          y.release();
        }
        break;
      case tLessEqualP : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          operandStack.push_back( operandCompare( x, y, FlagBE ) );
          x.release();
          y.release();
        }
        break;

      case tEqualD : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            assert( y._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._double == y._double ? 1 : 0 );
          } else {
            // TO DO: allow for NAN.  comisd docs suggest doing JP prior to JA etc.
            operandStack.push_back( operandCompareFloat( x, y, FlagE ) );
          }
          x.release();
          y.release();
        }
        break;
      case tNotEqualD : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            assert( y._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._double != y._double ? 1 : 0 );
          } else {
            // TO DO: allow for NAN.  comisd docs suggest doing JP prior to JA etc.
            operandStack.push_back( operandCompareFloat( x, y, FlagNE ) );
          }
          x.release();
          y.release();
        }
        break;
      case tGreaterD : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            assert( y._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._double > y._double ? 1 : 0 );
          } else {
            // Note: floating point values as always signed, but its comparision operator
            // produces flags that correspond with unsigned integer comparisions (A, B)
            operandStack.push_back( operandCompareFloat( x, y, FlagA ) );
          }
          x.release();
          y.release();
        }
        break;
      case tLessD : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            assert( y._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._double < y._double ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompareFloat( x, y, FlagB ) );
          }
          x.release();
          y.release();
        }
        break;
      case tGreaterEqualD : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            assert( y._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._double >= y._double ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompareFloat( x, y, FlagAE ) );
          }
          x.release();
          y.release();
        }
        break;
      case tLessEqualD : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            assert( x._kind == jit_Operand_Kind_ConstD );
            assert( y._kind == jit_Operand_Kind_ConstD );
            operandStack.emplace_back( jit_Operand_Kind_ConstI, x._double <= y._double ? 1 : 0 );
          } else {
            operandStack.push_back( operandCompareFloat( x, y, FlagBE ) );
          }
          x.release();
          y.release();
        }
        break;

      case tAllocActuals : {
          int size = *tCodePc++;
          // As noted in finishMethod(), we will bump only by a multiple of 16
          // to preserve stack alignment
          size = ( size + 15 ) & 0xfffffff0;
          if ( size ) {
            emitSub( Operand( jit_Operand_Kind_RegP, regRsp ),
                    Operand( jit_Operand_Kind_ConstI, size ) );
          }
          callInfos.emplace_back( false );
        }
        break;
      case tAllocActualsCdecl : {
          int size = *tCodePc++;
          // As noted in finishMethod(), we will bump only by a multiple of 16
          // to preserve stack alignment
          size = ( size + 15 ) & 0xfffffff0;
          if ( size ) {
            emitSub( Operand( jit_Operand_Kind_RegP, regRsp ),
                    Operand( jit_Operand_Kind_ConstI, size ) );
          }
          callInfos.emplace_back( true );
        }
        break;
      case tFreeActuals : {
          int size = *tCodePc++;
          // As noted in finishMethod(), we will bump only by a multiple of 16
          // to preserve stack alignment
          size = ( size + 15 ) & 0xfffffff0;
          if ( size ) {
            emitAdd( Operand( jit_Operand_Kind_RegP, regRsp ),
                    Operand( jit_Operand_Kind_ConstI, size ) );
          }
        }
        break;
      case tCall : {
          // TO DO: move in-use registers that are not callee-save, into temporaries,
          //        and update operand stack to refer to temp instead.
          preserveRegsAcrossCall();
          int label = *tCodePc++;
          char* addr = findLabel( label );
          emitCall( addr );
          if ( !addr ) {
            requestPatch( (int*) ( nativePc - 4 ), label );
          }

          // Remove info we tracked starting at alloc actuals.
          // (The initial call to main doesn't have an alloc actuals,
          // so doesn't have a callInfos entry.)
          if ( callInfos.size() > 0 ) {
            callInfos.pop_back();
          }
        }
        break;
      case tCallCdecl : {
          // TO DO
          int label = *tCodePc++;
          // TO DO TEMP: assume cdecl is declared as extern,
          //    and might not be resolved:
          auto it = unresolvedExternLabels.find( label );
          if ( it != unresolvedExternLabels.end() ) {
            toDo( "tCallCdecl: %s is unresolved\n", it->second );
          }
          else {
            // TO DO: preserve in-use registers that are not callee-save.
            //  But, how does this interact with params?
            preserveRegsAcrossCall();
            // Detect whether we are calling a function, rather than a procedure.
            // A function call's tcode will have a push of the result value 
            // immediately after the call, prior to the tFreeActuals.
            cdeclCheckForFunction( tCodePc );
            // Get parameters into registers as necessary for cdecl calling convention.
            cdeclParamsIntoRegisters();
            char* addr = findLabel( label );
            // Assume cdecl calls are always far
            emitCallExtern( addr );
            // Deal with function return value, if any
            cdeclSaveReturnValue();
            if ( !addr ) {
              // unexpected, this should be known by now.  would need to patch a far call
              toDo( "patching of a cdecl call\n" );
            }
          }
          callInfos.pop_back();
        }
        break;
      case tReturn : {
          if ( !currLocalSpaceAddr ) {
            // We have not done tEnter.
            // That means we are in the dummy mini-method at the start of the tCode,
            // and there is no stack frame to unwind.   Simply return.
            emitReturnFromInitialStub();
          } else {
            emitReturn();
          }
        }
        break;
      case tEnter : {
          // We're starting a new method.  Take this opportunity to wrap up
          // the previous method, if necessary.
          finishMethod();
          emitEnter( *tCodePc++ );
        }
        break;
      case tJump : {
          int label = *tCodePc++;
          emitJmpToLabel( label );
        }
        break;
      case tJumpTrue : {
          Operand x = operandStack.back();   operandStack.pop_back();
          int label = *tCodePc++;
          if ( x.isConst() ) {
            if ( x._value ) {
              // unconditional jump
              emitJmpToLabel( label );
            } else {
              // unconditional no-op
            }
          } else if ( x.isFlags() ) {
            ConditionFlags flags = x._flags;
            emitJccToLabel( label, flags );
          } else {
            // x <> 0
            Operand c( jit_Operand_Kind_ConstI, 0 );
            Operand cmpOp = operandCompare( x, c, FlagNE );
            // conditional jump
            emitJccToLabel( label, cmpOp._flags );
          }
          x.release();
        }
        break;
      case tJumpFalse : {
          Operand x = operandStack.back();   operandStack.pop_back();
          int label = *tCodePc++;
          if ( x.isConst() ) {
            if ( x._value ) {
              // unconditional no-op
            } else {
              // unconditional jump
              emitJmpToLabel( label );
            }
          } else if ( x.isFlags() ) {
            ConditionFlags flags = x._flags;
            negateConditionFlag( flags );
            emitJccToLabel( label, flags );
          } else {
            // x == 0
            Operand c( jit_Operand_Kind_ConstI, 0 );
            Operand cmpOp = operandCompare( x, c, FlagE );
            // conditional jump
            emitJccToLabel( label, cmpOp._flags );
          }
          x.release();
        }
        break;
      case tJumpCaseB : {
          // Same as tJumpCaseI, except we use unsigned comparison flags (e.g. FlagBE rather than FlagLE)
          Operand x = operandStack.back();   operandStack.pop_back();
          int table = findLabelTCode( *tCodePc++ );
          operandIntoReg( x );
          while ( true ) {
            if ( tCode[table] == tCase ) {
              Operand y( jit_Operand_Kind_ConstI, tCode[table+1] );
              Operand c = operandCompare( x, y, FlagE );
              int label = tCode[table+2];
              emitJccToLabel( label, c._flags );
              table += 3;
            } else if ( tCode[table] == tCaseRange ) {
              Operand low( jit_Operand_Kind_ConstI, tCode[table+1] );
              Operand cLow = operandCompare( x, low, FlagB );
              int labelSkip = labelNew();
              emitJccToLabel( labelSkip, cLow._flags );
              Operand high( jit_Operand_Kind_ConstI, tCode[table+2] );
              Operand cHigh = operandCompare( x, high, FlagBE );
              int label = tCode[table+3];
              emitJccToLabel( label, cHigh._flags );
              defineLabel( labelSkip, nativePc );
              table += 4;
            } else if ( tCode[table] == tCaseEnd ) {
              int label = tCode[table+1];
              emitJmpToLabel( label );
              break;
            } else {
              fatal( "unexpected instruction in case table\n" );
            }
          }
          x.release();
        }
        break;
      case tJumpCaseI : {
          // TO DO: we could choose to create a jump table
          //        if the set of choices looks suitable.s
          Operand x = operandStack.back();   operandStack.pop_back();
          int table = findLabelTCode( *tCodePc++ );
          operandIntoReg( x );
          while ( true ) {
            if ( tCode[table] == tCase ) {
              Operand y( jit_Operand_Kind_ConstI, tCode[table+1] );
              Operand c = operandCompare( x, y, FlagE );
              int label = tCode[table+2];
              emitJccToLabel( label, c._flags );
              table += 3;
            } else if ( tCode[table] == tCaseRange ) {
              Operand low( jit_Operand_Kind_ConstI, tCode[table+1] );
              Operand cLow = operandCompare( x, low, FlagL );
              int labelSkip = labelNew();
              emitJccToLabel( labelSkip, cLow._flags );
              Operand high( jit_Operand_Kind_ConstI, tCode[table+2] );
              Operand cHigh = operandCompare( x, high, FlagLE );
              int label = tCode[table+3];
              emitJccToLabel( label, cHigh._flags );
              defineLabel( labelSkip, nativePc );
              table += 4;
            } else if ( tCode[table] == tCaseEnd ) {
              int label = tCode[table+1];
              emitJmpToLabel( label );
              break;
            } else {
              fatal( "unexpected instruction in case table\n" );
            }
          }
          x.release();
        }
        break;
      case tJumpCaseS : {
          Operand x = operandStack.back();   operandStack.pop_back();
          int table = findLabelTCode( *tCodePc++ );
          // x is a pointer to ShortString.
          // I don't want x itself in register since that might be trashed by strcmp.
          // Essentially I want to preserve x around that method call.
          // The simple way I'll do it here is to force x out of any register.
          operandNotRegOrDeref( x );
          while ( true ) {
            if ( tCode[table] == tCase ) {
              Operand y( jit_Operand_Kind_Addr_Global, tCode[table+1] );
              // runlibShortStrCmp( x, y ) == 0
              Operand xTemp = x;   // must leave x unchanged
              operandIntoSpecificReg( xTemp, paramRegs[0], 8 );
              operandIntoSpecificReg( y, paramRegs[1], 8 );
              emitCallExtern( (char*) runlibShortStrCmp );
              xTemp.release();
              y.release();
              Operand result( jit_Operand_Kind_RegI, regRax );
              Operand zero( jit_Operand_Kind_ConstI, 0 );
              Operand c = operandCompare( result, zero, FlagE );
              int label = tCode[table+2];
              emitJccToLabel( label, c._flags );
              table += 3;
            } else if ( tCode[table] == tCaseRange ) {
              int labelSkip = labelNew();
              {
                Operand low( jit_Operand_Kind_Addr_Global, tCode[table+1] );
                // runlibShortStrCmp( x, low ) < 0
                Operand xTemp = x;   // must leave x unchanged
                operandIntoSpecificReg( xTemp, paramRegs[0], 8 );
                operandIntoSpecificReg( low, paramRegs[1], 8 );
                emitCallExtern( (char*) runlibShortStrCmp );
                xTemp.release();
                low.release();
                Operand result( jit_Operand_Kind_RegI, regRax );
                Operand zero( jit_Operand_Kind_ConstI, 0 );
                Operand cLow = operandCompare( result, zero, FlagL );
                emitJccToLabel( labelSkip, cLow._flags );
              }
              {
                Operand high( jit_Operand_Kind_Addr_Global, tCode[table+2] );
                // runlibShortStrCmp( x, high ) <= 0
                Operand xTemp = x;   // must leave x unchanged
                operandIntoSpecificReg( xTemp, paramRegs[0], 8 );
                operandIntoSpecificReg( high, paramRegs[1], 8 );
                emitCallExtern( (char*) runlibShortStrCmp );
                xTemp.release();
                high.release();
                Operand result( jit_Operand_Kind_RegI, regRax );
                Operand zero( jit_Operand_Kind_ConstI, 0 );
                Operand cHigh = operandCompare( result, zero, FlagLE );
                int label = tCode[table+3];
                emitJccToLabel( label, cHigh._flags );
              }
              defineLabel( labelSkip, nativePc );
              table += 4;
            } else if ( tCode[table] == tCaseEnd ) {
              int label = tCode[table+1];
              emitJmpToLabel( label );
              break;
            } else {
              fatal( "unexpected instruction in case table\n" );
            }
          }
          x.release();
        }
        break;
      case tCase : {
          // A no-op if we fall through to this case table
          tCodePc += 2;
        }
        break;
      case tCaseRange : {
          // A no-op if we fall through to this case table
          tCodePc += 3;
        }
        break;
      case tCaseEnd : {
          // A no-op if we fall through to this case table
          tCodePc++;
        }
        break;
      case tLabel : {
          // We already defined the tCode address of this label,
          // but now we can fill in the native address.
          int label = *tCodePc++;
          defineLabel( label, nativePc );
        }
        break;
      case tLabelAlias :
        // Already handled in defineLabels().
        tCodePc += 2;
        break;
      case tLabelExtern :
        // Already handled in defineLabels().
        tCodePc += 2;
        break;
      case tWriteI : {
          Operand x = operandStack.back();   operandStack.pop_back();
          // We need to form a call with the native calling convention.
          // At the moment it's simple because I only support these simple 1-parameter methods.
          Register* paramReg1 = paramRegs[0];
          operandIntoSpecificReg( x, paramReg1, 4 );
          emitCallExtern( (char*) runlibWriteI );
          x.release();
        }
        break;
      case tWriteBool : {
          Operand x = operandStack.back();   operandStack.pop_back();
          Register* paramReg1 = paramRegs[0];
          operandIntoSpecificReg( x, paramReg1, 1 );
          emitCallExtern( (char*) runlibWriteBool );
          x.release();
        }
        break;
      case tWriteChar : {
          Operand x = operandStack.back();   operandStack.pop_back();
          Register* paramReg1 = paramRegs[0];
          operandIntoSpecificReg( x, paramReg1, 1 );
          emitCallExtern( (char*) runlibWriteChar );
          x.release();
        }
        break;
      case tWriteShortStr : {
          Operand x = operandStack.back();   operandStack.pop_back();
          Register* paramReg1 = paramRegs[0];
          operandIntoSpecificReg( x, paramReg1, 8 );
          emitCallExtern( (char*) runlibWriteShortStr );
          x.release();
        }
        break;
      case tWritePChar : {
          Operand x = operandStack.back();   operandStack.pop_back();
          Register* paramReg1 = paramRegs[0];
          operandIntoSpecificReg( x, paramReg1, 8 );
          emitCallExtern( (char*) runlibWritePChar );
          x.release();
        }
        break;
      case tWriteP : {
          Operand x = operandStack.back();   operandStack.pop_back();
          Register* paramReg1 = paramRegs[0];
          operandIntoSpecificReg( x, paramReg1, 8 );
          emitCallExtern( (char*) runlibWriteP );
          x.release();
        }
        break;
      case tWriteEnum : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          operandIntoSpecificReg( x, paramRegs[0], 4 );
          operandIntoSpecificReg( y, paramRegs[1], 8 );
          emitCallExtern( (char*) runlibWriteEnum );
          x.release();
          y.release();
        }
        break;
      case tWriteD : {
          Operand x = operandStack.back();   operandStack.pop_back();
          Register* paramReg1 = paramFloatRegs[0];
          operandIntoSpecificReg( x, paramReg1, 8 );
          emitCallExtern( (char*) runlibWriteD );
          x.release();
        }
        break;
      case tWriteCR : {
          emitCallExtern( (char*) runlibWriteCR );
        }
        break;
      case tReadI : {
          Operand x = operandStack.back();   operandStack.pop_back();
          operandIntoSpecificReg( x, paramRegs[0], 8 );
          emitCallExtern( (char*) runlibReadI );
          x.release();
        }
        break;
      case tReadChar : {
          Operand x = operandStack.back();   operandStack.pop_back();
          operandIntoSpecificReg( x, paramRegs[0], 8 );
          emitCallExtern( (char*) runlibReadChar );
          x.release();
        }
        break;
      case tReadShortStr : {
          int capacity = *tCodePc++;
          Operand x = operandStack.back();   operandStack.pop_back();
          Operand y( jit_Operand_Kind_ConstI, capacity );
          operandIntoSpecificReg( x, paramRegs[0], 8 );
          operandIntoSpecificReg( y, paramRegs[1], 4 );
          emitCallExtern( (char*) runlibReadShortStr );
          x.release();
        }
        break;
      case tReadCR : {
          emitCallExtern( (char*) runlibReadCR );
        }
        break;
      case tFile :
        // no-op, for debugging
        continue;
      case tLine :
        // no-op, for debugging
        continue;

      default:
        tCodeNotImplemented();
        continue;
    }
  }

  finishMethod();
  makePatches();
  if ( optionDumpCode ) {
    dumpNativeCode();
  }
}


// Perform some fix-up work on the method we've just completed.
// No harm if this is called more than once at the end of a method.
//
void
finishMethod()
{
  if ( currLocalSpaceAddr ) {

    // Adjust local space to meet the stack alignment rule of the x64-64 ABI.
    // This rule is common to linux64 and windows64:
    // The stack must be aligned on 16 bytes immediately prior to a call.
    // Therefore, a method may assume that on entry, rsp is 8 bytes off of 16-byte
    // alignment (due to the return address pushed on the stack).
    //
    // Methods may rely on that alignment e.g. when dumping vector registers
    // to the stack.  So it must be honored or weird crashes may occur
    // (and I've seen this when calling the SDL library).
    //
    // In our case, we are patching the local space allocated by tEnter (emitEnter),
    // which has -also- pushed rbp.  So we must ensure that the local space is
    // a multiple of 16.  We'll also need to ensure that tAllocActuals adjusts by
    // a multiple of 16 too.
    currLocalSpace = ( currLocalSpace + 15 ) & 0xfffffff0;

    *currLocalSpaceAddr = currLocalSpace;
    currLocalSpaceAddr = nullptr;
  }
}


// ----------------------------------------------------------------------------

// Return the operand kind for a register with the given size.
//
jitOperandKind
operandKindReg( int size, bool isFloat )
{
  if ( isFloat ) {
    switch ( size ) {
      case 8: return jit_Operand_Kind_RegD;
      default:
        fatal( "operandKindReg: unexpected size %d for float\n", size );
        return jit_Operand_Kind_Illegal;
    }
  } else {
    switch ( size ) {
      case 1: return jit_Operand_Kind_RegB;
      case 4: return jit_Operand_Kind_RegI;
      case 8: return jit_Operand_Kind_RegP;
      default:
        fatal( "operandKindReg: unexpected size %d\n", size );
        return jit_Operand_Kind_Illegal;
    }
  }
}


// Provide a general purpose register.
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

  fatal( "TO DO: allocateReg dump a reg into temporary\n" );
  return nullptr;
}


// Provide a floating point register.
//
Register*
allocateFloatReg()
{
  for ( Register* reg : floatRegisters ) {
    if ( !reg->_inUse ) {
      reg->_inUse = true;
      return reg;
    }
  }  

  fatal( "TO DO: allocateFloatReg dump a reg into temporary\n" );
  return nullptr;

}


// Force reg to be available (possibly by dumping to a temporary),
// and allocate it.
// TO DO: I could also dump into a different register.
// TO DO: Can this work as-is for floating point register? If so, document this.
//
void
forceAllocateReg( Register* reg )
{
  if ( reg->_inUse ) {
    // Dump to temporary.
    // TO DO: move into another register if available.

    // We'll need to scan through the operand stack, since those operands will
    // need to change to refer to the temporary.
    // And, this gives us an opportunity to see how big the temp needs to be.

    for ( Operand& x : operandStack ) {
      if ( x.isReg() && x._reg == reg ) {
        Operand newX = allocateTemp( x.size(), reg->isFloat() );
        emitMov( newX, x );
        x.release();
        // replace entry in operand stack
        x = newX;
      } else if ( x.isDeref() && x._reg == reg ) {
        // would need work if this can happen.
        // e.g. move the dereferenced value into another reg, or maybe the same reg,
        //   and from there into memory if there wasn't another reg available.
        fatal( "forceAllocateReg: unexpected Deref in operand stack\n" );
      }
    }
  }
  reg->_inUse = true;
}


// Return a register pointing to the stack frame of an enclosing static scope
// (uplevels > 0) or the current scope (uplevels = 0).  This is used for working with
// nested methods, which may access variables in enclosing scopes.
//
// The caller is free to further modify the register (e.g. for array/record indexing).
// (As a future optimization, I may cache the immediate parent scope in r10, whcih is a
// convention for languages with static scopes.  In that case, I'll need to prevent
// modifying the register, copying to a different one if a further modification is needed.
//
// The caller will need to release the register at some point.
// For now, the caller uses the register to create a Deref operand, so the register will
// become released when that operand is released. 
//
//
Register*
getUpFrame( int uplevels )
{
  Register* reg = allocateReg();

  if ( uplevels == 0 ) {
    // Pointer to the current scope.  This is needed when calling a nested method
    // that's an immediate child of the current scope (which need not be nested itself).
    // For now, always copying from rbp into another register that the user can modify further.
    Operand opReg( jit_Operand_Kind_RegP, reg );
    Operand opRbp( jit_Operand_Kind_RegP, regRbp );
    emitMov( opReg, opRbp );
    return reg;
  }

  // Move up the static link chain, consisting of the first parameter into nested methods.
  // First jump up starts near rbp.
  Operand opReg( jit_Operand_Kind_RegP, reg );
  Operand opStaticLinkFromRbp( jit_Operand_Kind_RegP_DerefP, regRbp, FRAME_STATIC_LINK_OFFSET );
  emitMov( opReg, opStaticLinkFromRbp );

  // Subsequent jumps, if any, are starting from the register we are advancing through the chain.
  for ( int level = 1; level < uplevels; ++level ) {
    Operand opStaticLinkFromReg( jit_Operand_Kind_RegP_DerefP, reg, FRAME_STATIC_LINK_OFFSET );
    emitMov( opReg, opStaticLinkFromReg );
  }

  return reg;
}


// Move in-use registers that are not callee-save, into temporaries,
// and update operand stack to refer to temp instead.
//
void
preserveRegsAcrossCall()
{
  for ( Operand& x : operandStack ) {
    if ( x.isReg() && !x._reg->_calleeSave ) {
      // TO DO: if same reg is referenced by multiple opconds in the stack,
      //        share the same temporary too.  But only if they have the same size.
      Operand newX = allocateTemp( x.size(), x._reg->isFloat() );
      emitMov( newX, x );
      x.release();
      // replace entry in operand stack
      x = newX;
    } else if ( x.isDeref() && !x._reg->_calleeSave ) {
      // would need work if this can happen
      fatal( "unexpected Deref in operand stack at call\n" );
    }
  }
}


// Check if a tAssign* is assigning to an actual.
// If so, and we're evaluating params of a cdecl call,
// build up an ordered list of actuals that correspond with parameters.
//
// At the moment, the assignment into the stack always proceeds,
// and we'll copy into registers, as needed by the native calling convention,
// later via cdeclParamsIntoRegisters().
//
// Note this will pick up a function result's dummy VAR parameter too.
// We will manage to avoid assigning that to a register for the call.
//
void
cdeclCheckForAssignToActual( Operand& x, jitOperandKind actualKind )
{
  if ( ( x._kind == jit_Operand_Kind_Addr_Actual ) &&
        ( callInfos.size() > 0 ) &&
        ( callInfos.back()._cdecl == true ) ) {
    CallInfo& ci = callInfos.back();
    Operand actualOnStack( actualKind, x._value );
    // Don't assume the assignments to actuals occur in left param to right param order.
    // Build up a sorted list, and assign registers later.
    // Not expecting many params, so a simple insertion sorting.
    auto it = ci._actualsToRegs.begin();
    while ( ( it != ci._actualsToRegs.end() ) &&
            ( it->first._value < x._value ) ) {
      ++it;
    }
    ci._actualsToRegs.insert( it, { { actualOnStack, nullptr } } );
  }
}


// During processing of tCallCdecl.
// See if we are making a function call, rather than a procedure call.
// tCodePc is pointing to the instruction after tCallDecl.
//
// If it is a function, update the current CallInfo with the location of
// where the tcode expects the return value to be found.
// Later, cdeclSaveReturnValue() will store the native return value there
// so the tcode will work as it expects.
//
void
cdeclCheckForFunction( int* tCodePc )
{
  if ( callInfos.empty() || !callInfos.back()._cdecl ) {
    // not a cdecl call
    return;
  }
  if ( *tCodePc == tFreeActuals ) {
    // this is a procedure call
    return;
  }
  CallInfo& ci = callInfos.back();
  
  // The tcode instruction should be pushing the local space temporary
  // that it expects the called function to store the return value in.
  int offset;
  switch ( *tCodePc ) {
    case tPushLocalI:
      ci._isFunc = true;
      ci._returnValue = Operand( jit_Operand_Kind_LocalI, tCodePc[1] );
      break;
    case tPushLocalB:
      ci._isFunc = true;
      ci._returnValue = Operand( jit_Operand_Kind_LocalB, tCodePc[1] );
      break;
    case tPushLocalP:
      ci._isFunc = true;
      ci._returnValue = Operand( jit_Operand_Kind_LocalP, tCodePc[1] );
      break;
    case tPushLocalD:
      ci._isFunc = true;
      ci._returnValue = Operand( jit_Operand_Kind_LocalD, tCodePc[1] );
      break;
      case tPushAddrLocal:
      // complex return value
      // not supporting in cdec yet.
      toDo( "cdeclCheckForFunction: complex function return type not supported yet\n" );
      break;
    default:
      fatal( "cdeclCheckForFunction: unexpected instruction after tCallCdecl\n" );
  }

  if ( ci._isFunc ) {
    // The last parameter that we thought we saw isn't real
    // and doesn't need to be provided.
    // TO DO: when complex return types are supported, this needs to change.
    assert( !ci._actualsToRegs.empty() );
    ci._actualsToRegs.pop_back();
  }
}


// Set up a cdecl call, by putting params into registers as necessary.
//
// The current approach for cdecl calls is, the tcode contains
// my normal Pascal calling convention, assigning actuals to stack space
// in the actuals area.  JIT will notice these assignments, and record the
// location and type of the parameters that should go in registers,
// within a CallInfo object.
// Now, at the time of the call, we will use that recorded information
// to generate moves from the stack space into the registers.
//
// This is obviously not ideal, since it forces everything onto the stack.
// But, it's simple.  Later I could replace the assign-into-stack
// with an assign-into-reg directly.  But then I need to watch out for that
// register getting dumped into a temporary during another actual expression.
// (Right now I'd have a problem there, because only operands on the operand stack
// get updated when a register is dumped to a temporary.)
//
// One other current limitation is that I can't handle non-register parameters.
// Complex types are not supported because JIT doesn't see enough type info
// to decide what registers should be used.   And params beyond the number of
// calling convention registers are not supported because I'd have to
// shift the value's location on the stack, and haven't tackled that yet.
//
void
cdeclParamsIntoRegisters()
{
  CallInfo& ci = callInfos.back();
  assert( ci._cdecl == true );

  for ( auto actualToReg : ci._actualsToRegs ) {
    int paramSize = actualToReg.first.size();
    if ( actualToReg.first._kind == jit_Operand_Kind_ActualD ) {
      if ( ci._numFloatRegs >= paramFloatRegs.size() ) {
        toDo( "cdecl call with more float parameters than will fit in float regs\n" );
      }
      actualToReg.second = paramFloatRegs[ ci._numFloatRegs++ ];
    } else {
      if ( ci._numGeneralRegs >= paramRegs.size() ) {
        toDo( "cdecl call with more parameters than will fit in general regs\n" );
      }
      actualToReg.second = paramRegs[ ci._numGeneralRegs++ ];
    }
    operandIntoSpecificReg( actualToReg.first, actualToReg.second, paramSize );
  }
}

// Following a cdecl function call,
// store the native return value into the local temporary that
// tcode expects to find it in.
// This isn't efficient but it is simple.  Following the approach of params,
// we are converting between what the native calling convention does
// and my own tcode calling convetion.
//
void
cdeclSaveReturnValue()
{
  CallInfo& ci = callInfos.back();
  assert( ci._cdecl == true );

  if ( ci._isFunc ) {
    // cdecl return value is in regRax for integral types,
    // and xmm0 for floating types.  I don't support struct return types yet.
    bool isFloat = ci._returnValue.isFloat();
    Register* cdeclReg = isFloat ? regXmm0 : regRax;
    Operand cdeclResult( operandKindReg( ci._returnValue.size(), isFloat ), cdeclReg );
    emitMov( ci._returnValue, cdeclResult );
  }  
}



// Allocate a temporary variable of the given byte size (1, 4, 8).
//
Operand
allocateTemp( int size, bool isFloat )
{
  currLocalSpace += size;
  jitOperandKind newKind = jit_Operand_Kind_Illegal;
  if ( isFloat ) {
    switch ( size ) {
      case 8:
        newKind = jit_Operand_Kind_LocalD;
        break;
      default:
        fatal( "allocateTemp: unexpected size %d for float\n", size );
    }
  } else {
    switch ( size ) {
      case 1:
        newKind = jit_Operand_Kind_LocalB;
        break;
      case 4:
        newKind = jit_Operand_Kind_LocalI;
        break;
      case 8:
        newKind = jit_Operand_Kind_LocalP;
        break;
      default:
        fatal( "allocateTemp: unexpected size %d\n", size );
    }
  }
  return Operand( newKind, -currLocalSpace );
}


// Define the given label to mean the given native address.
//
void
defineLabel( int label, char* nativeAddr )
{
  labels[label] = nativeAddr;

  if ( listingFile ) {
    fprintf( listingFile, "%p\tLabel %d\n", nativeAddr, label );
  }
}


// Define the given label to mean the given tCode address.
// (THe address after the label instruction.)
//
void
defineLabelTCode( int label, int tCodeAddr )
{
  labelTCodeAddrs[label] = tCodeAddr;
}


// Define the given label to be an alias of another label aliasToLabel
// (which might itself be defined as an alias).
//
void
defineLabelAlias( int label, int aliasToLabel )
{
  labelAliases[label] = aliasToLabel;
}

void
defineLabelExtern( int label, char* name )
{
  void* addr = runlibLookupMethod( name );
  if ( addr != nullptr ) {
    labels[label] = (char*) addr;
  } else {
    unresolvedExternLabels[label] = name;
  }
}


// Find the native address of the label.
// Returns NULL if not defined.
//
char*
findLabel( int label )
{
  auto it = labels.find( label );
  if ( it != labels.end() ) {
    return it->second;
  }
  auto it2 = labelAliases.find( label );
  if ( it2 != labelAliases.end() ) {
    return findLabel( it2->second );
  }
  // This label has not yet been defined
  return nullptr;
}


// Find the tCode address of the label.
// (This is the address of the instruction after the label.)
//
int
findLabelTCode( int label )
{
  auto it = labelTCodeAddrs.find( label );
  if ( it != labelTCodeAddrs.end() ) {
    return it->second;
  }
  auto it2 = labelAliases.find( label );
  if ( it2 != labelAliases.end() ) {
    return findLabelTCode( it2->second );
  }
  // This label has not yet been defined
  return 0;
}


// Request future patching a label's address
// to be installed at the given location.
// When the address is installed, it will be written as a
// rip-relative value, assuming the integer being patched
// are the final bytes of its instruction.
//
void
requestPatch( int* patchAt, int label )
{
  patches.emplace_back( patchAt, label );  
}

void
makePatches()
{
  for ( auto patch : patches ) {
    char* targetAddr = findLabel( patch.second );
    if ( !targetAddr ) {
      fatal( "makePatches: label %d was never defined\n", patch.second );
    }
    int* patchAt = patch.first;
    char* nextInstr = ( (char*)patchAt ) + 4;
    *patchAt = targetAddr - nextInstr;
  }
}


// If x is a power of 2 (not including 2^0),
// return the power, i.e. how many bits to shift 1 left to get it.
// Returns 0 if not a power of 2 (not including 2^0).
//
int
isPowerOf2( int x )
{
  for ( int bits = 0; bits < 32 ; ++bits ) {
    if ( x == ( 1 << bits ) ) {
      return bits;
    }
  }
  return 0;
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
    case jit_Operand_Kind_ActualB:
    case jit_Operand_Kind_RegB:
    case jit_Operand_Kind_RegP_DerefB:
    case jit_Operand_Kind_Flags:
      return 1;

    case jit_Operand_Kind_GlobalI:
    case jit_Operand_Kind_LocalI:
    case jit_Operand_Kind_ParamI:
    case jit_Operand_Kind_ActualI:
    case jit_Operand_Kind_ConstI:
    case jit_Operand_Kind_RegI:
    case jit_Operand_Kind_RegP_DerefI:
      return 4;

    case jit_Operand_Kind_GlobalP:
    case jit_Operand_Kind_GlobalD:
    case jit_Operand_Kind_LocalP:
    case jit_Operand_Kind_LocalD:
    case jit_Operand_Kind_ParamP:
    case jit_Operand_Kind_ParamD:
    case jit_Operand_Kind_ActualP:
    case jit_Operand_Kind_ActualD:
    case jit_Operand_Kind_Addr_Global:
    case jit_Operand_Kind_Addr_Local:
    case jit_Operand_Kind_Addr_Param:
    case jit_Operand_Kind_Addr_Actual:
    case jit_Operand_Kind_Addr_Reg_Offset:
    case jit_Operand_Kind_ConstD:
    case jit_Operand_Kind_RegP:
    case jit_Operand_Kind_RegD:
    case jit_Operand_Kind_RegP_DerefP:
    case jit_Operand_Kind_RegP_DerefD:
      return 8;

    default:
      fatal( "Operand::size() - unexpected operand kind %d\n", (int) _kind );
  }
  return 1;  // won't reach here
}


// Does the operand represent a floating point value?
//
bool
Operand::isFloat() const
{
  switch ( _kind ) {
    case jit_Operand_Kind_GlobalD:
    case jit_Operand_Kind_LocalD:
    case jit_Operand_Kind_ParamD:
    case jit_Operand_Kind_ActualD:
    case jit_Operand_Kind_ConstD:
    case jit_Operand_Kind_RegD:
    case jit_Operand_Kind_RegP_DerefD:
      return true;

    default:
      return false;
  }
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


// Force (at least) one of the operands into a floating point register.
// On return, x will be in a floating point register.
// This is for a commutative operation, so x and y may be swapped.
//
void
operandIntoFloatRegCommutative( Operand& x, Operand& y )
{
  if ( x.isFloatReg() ) {
    return;
  }
  if ( y.isFloatReg() ) {
    swap( x, y );
    return;
  }
  // we'll prefer to put a variable into a register
  if ( !x.isVarOrAddrOfVar() && y.isVarOrAddrOfVar() ) {
    swap( x, y );
  }
  operandIntoFloatReg( x );
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
  Register* reg = nullptr;
  if ( x.isFloat() ) {
    reg = allocateFloatReg();
    switch ( x.size() ) {
      case 8:
        regKind = jit_Operand_Kind_RegD;
        break;
      default:
        fatal( "operandIntoReg: unexpected size %d for float operand\n", x.size() );
    }
  } else {
    reg = allocateReg();
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
      default:
        fatal( "operandIntoReg: unexpected size %d\n", x.size() );
    }
  }
  Operand newX( regKind, reg );
  emitMov( newX, x );
  x.release();
  x = newX;
}


// Force the operand into a floating point register, if it isn't already.
//
void
operandIntoFloatReg( Operand& x )
{
  if ( x.isFloatReg() ) {
    return;
  }

  // The size of the register will depend on the value we're moving into it
  jitOperandKind regKind;
  switch ( x.size() ) {
    case 8:
      regKind = jit_Operand_Kind_RegD;
      break;
  }
  Operand newX( regKind, allocateFloatReg() );
  emitMov( newX, x );
  x.release();
  x = newX;
}


// Ensure the operand is in the given register, with the given data size
//
void
operandIntoSpecificReg( Operand& x, Register* reg, int size )
{
  jitOperandKind desiredKind = jit_Operand_Kind_Illegal;

  switch ( size ) {
    case 1:
      desiredKind = jit_Operand_Kind_RegB;
      break;
    case 4:
      desiredKind = jit_Operand_Kind_RegI;
      break;
    case 8:
      desiredKind = reg->isFloat() ? jit_Operand_Kind_RegD : jit_Operand_Kind_RegP;
      break;
    default:
      fatal( "operandIntoSpecificReg: unexpected size\n" );
  }
  Operand desiredOp( desiredKind, reg );
  // If it's already there, with the right size, we don't need to do anything.
  // If it's in the right register, but with a smaller size,
  // move to itself to sign-extend.
  if ( x.isReg() && x._reg == reg && x.size() >= size ) {
    // The operand is already as desired
    // (or bigger size, but that should be ok)
    // TO DO: make sure mov from larger to smaller is always accepted.
  } else if ( x.isReg() && x._reg == reg && x.size() < size ) {
    // In the correct reg, but need to extend the value.
    // We can do that by moving to itself.
    emitMov( desiredOp, x );
    x = desiredOp;
  } else {
    // Not in the right place yet.

    // TO DO: I think I have a logical flaw with inUse / release.
    // e.g. if x is RegDeref using the desired reg.
    //    forceAllocateReg would dump the operand to a temporary,
    // and update the operand on the expr stack.
    // BUT the x variable the caller has passed down to here is
    // no longer on the expr stack.  So it wouldn't get updated.
    // And maybe the dump-to-temporary wouldn't actually dump anything
    // since we don't see th operand on the stack.
    // Probably I should leave x and y operands on the stack
    // while the instruction is being processed, and only remove them
    // at the end?  Need to think about it.

    forceAllocateReg( desiredOp._reg );
    emitMov( desiredOp, x );
    x.release();
    x = desiredOp;
  }
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


// Ensure operand is not in a register or dereference of a register.
//
void
operandNotRegOrDeref( Operand& x )
{
  if ( x.isReg() || x.isDeref() ) {
    Operand newX = allocateTemp( x.size(), x.isFloat() );
    emitMov( newX, x );
    x.release();
    x = newX;
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
    case jit_Operand_Kind_Addr_Reg_Offset:
      switch ( size ) {
        case 1:
          x = Operand( jit_Operand_Kind_RegP_DerefB, x._reg, x._value );
          break;
        case 4:
          x = Operand( jit_Operand_Kind_RegP_DerefI, x._reg, x._value );
          break;
        case 8:
          x = Operand( jit_Operand_Kind_RegP_DerefP, x._reg, x._value );
          break;
        default:
          break;
      }
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


// x is a pointer to a floating point value of the given byte size.
// Make x refer to the pointed-to value.
// The resulting operand can be used as the left or right argument of emitMov().
//
void
operandDerefFloat( Operand& x, int size )
{
  if ( x.isVar() ) {
    // the pointer is stored in a variable, so we need to fetch the pointer first.
    operandIntoReg( x );
  }

  // Create an opcond that represents the pointed-to value.
  switch ( x._kind ) {
    case jit_Operand_Kind_Addr_Global:
      switch ( size ) {
        case 8:
          x = Operand( jit_Operand_Kind_GlobalD, x._value );
          break;
        default:
          break;
      }
      break;
    case jit_Operand_Kind_Addr_Local:
      switch ( size ) {
        case 8:
          x = Operand( jit_Operand_Kind_LocalD, x._value );
          break;
        default:
          break;
      }
      break;
    case jit_Operand_Kind_Addr_Param:
      switch ( size ) {
        case 8:
          x = Operand( jit_Operand_Kind_ParamD, x._value );
          break;
        default:
          break;
      }
      break;
    case jit_Operand_Kind_Addr_Actual:
      switch ( size ) {
        case 8:
          x = Operand( jit_Operand_Kind_ActualD, x._value );
          break;
        default:
          break;
      }
      break;
    case jit_Operand_Kind_Addr_Reg_Offset:
      switch ( size ) {
        case 8:
          x = Operand( jit_Operand_Kind_RegP_DerefD, x._reg, x._value );
          break;
        default:
          break;
      }
    case jit_Operand_Kind_RegP:
      switch ( size ) {
        case 8:
          x = Operand( jit_Operand_Kind_RegP_DerefD, x._reg, 0 );
          break;
        default:
          break;
      }
      break;
    default:
      fatal( "operandDerefFloat: unexpected operands\n" );
      break;
  }
}


// Sign-extend the operand to a 64-bit value.
// Exception: we don't bother to extend a ConstI
// because many instructions can sign-extend an immediate32 to 64 bits.
//
void
operandExtendToP( Operand& x )
{
  if ( x.size() == 8 ) {
    return;
  }

  if ( x.isConst() ) {
    // won't bother extending const, as noted above
  } else if ( x.isVar() || x.isDeref() ) {
    Operand newX( jit_Operand_Kind_RegP, allocateReg() );
    emitMov( newX, x );   // knows how to sign-extend
    x = newX;
  } else if ( x.isReg() ) {
    Operand newX( jit_Operand_Kind_RegP, x._reg );
    emitMov( newX, x );   // mov register to itself, with sign extension
    x = newX;
  }
}


void
swap( Operand& x, Operand& y )
{
  Operand temp = x;
  x = y;
  y = temp;
}


// Generate a comparison of two operands.
// The condition flags indicates what should be considered a true result.
// Returns the result operand (which will usually be a Flags Operand).
//
Operand
operandCompare( Operand& x, Operand& y, ConditionFlags flags )
{
  // emitCmp doesn't support addr in x or y.
  operandKindAddrIntoReg( x );
  operandKindAddrIntoReg( y );

  // emitCmp doesn't expect flags as input
  // TO DO: Concern: Could it be too late to do this here.
  //        We have to convert flags to value prior to any other operation that might touch the flags.
  //        If I let the operands linger on the stack, that flag status can become obsolete.
  //        How can I catch such an error, and it is is possible where else should I do the
  //        lazy conversion to 0/1.
  operandFlagsToValue( x, 1 );
  operandFlagsToValue( y, 1 );

  // emitCmp doesn't support const in x.
  if ( x.isConst() ) {
    swap( x, y );
    swapConditionFlag( flags );
  }

  // emitCmp doesn't allow both x and y in memory.
  if ( x.isMem() && y.isMem() ) {
    operandIntoReg( x );
  }

  emitCmp( x, y );

  return Operand( jit_Operand_Kind_Flags, flags );
}


// Generate a comparison of two floating point operands.
// The condition flags indicates what should be considered a true result.
// Returns the result operand (which will usually be a Flags Operand).
//
Operand
operandCompareFloat( Operand& x, Operand& y, ConditionFlags flags )
{
  // emitComisd only supports (reg, reg) or (reg, mem)
  if ( x.isConst() ) {
    operandIntoFloatReg( x );
  }
  if ( y.isConst() ) {
    operandIntoFloatReg( y );
  }
  if ( x.isMem() && y.isFloatReg() ) {
    swap( x, y );
    swapConditionFlag( flags );
  }
  operandIntoFloatReg( x );
  // Don't expect I could have y in a general purpose reg, but just in case:
  if ( y.isReg() && !y.isFloatReg() ) {
    operandIntoFloatReg( y );
  }

  emitComisd( x, y );

  return Operand( jit_Operand_Kind_Flags, flags );
}


// If x is a Flags operand, covert it to a 0/1 value.
// Given the byte size of the value we want.
//
void
operandFlagsToValue( Operand& x, int size )
{
  if ( x.isFlags() ) {
    jitOperandKind kind = jit_Operand_Kind_Illegal;
    switch ( size ) {
      case 1:
        kind = jit_Operand_Kind_RegB;
        break;
      case 4:
        kind = jit_Operand_Kind_RegI;
        break;
      case 8:
        kind = jit_Operand_Kind_RegP;
        break;
      default:
        fatal( "operandFlagsToValue: unexpected size\n" );
    }

    Operand result( kind, allocateReg() );
    emitSet( result, x._flags );
    x = result;
  }
}


// Return an operand that represents the low 32 bits of the given operand.
//
Operand
operandLowWord( const Operand& x )
{
  switch ( x._kind ) {
    case jit_Operand_Kind_GlobalP:
    case jit_Operand_Kind_GlobalD:
      return Operand( jit_Operand_Kind_GlobalI, x._value );
    case jit_Operand_Kind_LocalP:
    case jit_Operand_Kind_LocalD:
      return Operand( jit_Operand_Kind_LocalI, x._value );
    case jit_Operand_Kind_ParamP:
    case jit_Operand_Kind_ParamD:
      return Operand( jit_Operand_Kind_ParamI, x._value );
    case jit_Operand_Kind_ActualP:
    case jit_Operand_Kind_ActualD:
      return Operand( jit_Operand_Kind_ActualI, x._value );
    case jit_Operand_Kind_RegP_DerefP:
    case jit_Operand_Kind_RegP_DerefD:
      return Operand( jit_Operand_Kind_RegP_DerefI, x._reg, x._value );
    default:
      fatal( "operandLowWord unexpected operand kind %d\n", x._kind );
      return Operand();
  }
}


// Return an operand that represents the high 32 bits of the given operand.
//
Operand
operandHighWord( const Operand& x )
{
  switch ( x._kind ) {
    case jit_Operand_Kind_GlobalP:
    case jit_Operand_Kind_GlobalD:
      return Operand( jit_Operand_Kind_GlobalI, x._value + 4 );
    case jit_Operand_Kind_LocalP:
    case jit_Operand_Kind_LocalD:
      return Operand( jit_Operand_Kind_LocalI, x._value + 4 );
    case jit_Operand_Kind_ParamP:
    case jit_Operand_Kind_ParamD:
      return Operand( jit_Operand_Kind_ParamI, x._value + 4 );
    case jit_Operand_Kind_ActualP:
    case jit_Operand_Kind_ActualD:
      return Operand( jit_Operand_Kind_ActualI, x._value + 4 );
    case jit_Operand_Kind_RegP_DerefP:
    case jit_Operand_Kind_RegP_DerefD:
      return Operand( jit_Operand_Kind_RegP_DerefI, x._reg, x._value + 4 );
    default:
      fatal( "operandLowWord unexpected operand kind %d\n", x._kind );
      return Operand();
  }
}



// ----------------------------------------------------------------------------
//
// x86 Operations
//
// All my comments use Intel syntax:     instruction  dest, src
// in order to match Intel's documentation.
// Note that linux gdb shows the opposite AT&T syntax:   instruction  src, dest
//
// I have a few layers for generating machine code.
// outB() etc - write bytes to the code stream.
// emitRex(), emitModRM* - emit REX prefix and ModR/M byte, in various configurations.
//                         The caller needs to know what configuration it wants.
// Instr - Slightly higher level, assembles all pieces for one instruction.
//         Currently only supports instructions involving a ModR/M byte.
// InstrTempl - Highest level.  Instruction templates based closely on Intel docs.
//         Finds a matching template based on Operands, and knows how to translate Operands
//         into underlying Instr api.
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


// Emit entry code at the start of a method.
// Given the number of local variable space requested by the tcode.
// This value can be increased later by jit for our own temporaries.
//
// Remember the address of the local-space value,
// so jit can bump it for tempraries too.
// We'll need to keep it a 32-bit offset regardless of current value.
//
void
emitEnter( int localSpace )
{
  // push rbp
  outB( 0x50 + regRbp->_nativeId );

  // mov rbp, rsp
  emitMov( Operand( jit_Operand_Kind_RegP, regRbp ),
           Operand( jit_Operand_Kind_RegP, regRsp ) );

  // sub rsp, localSpace
  // NOTE: use 32-bit regardless of value, so we can increase it later.
  emitSub( Operand( jit_Operand_Kind_RegP, regRsp ),
           Operand( jit_Operand_Kind_ConstI, localSpace ) );
    
  currLocalSpace = localSpace;
  currLocalSpaceAddr = (int*) ( nativePc - 4 );   // we know it is the 4 bytes just written


  // TO DO: preserve callee-save registers that this method might use.
  // Previously I pushed esi edi.  But rules different now.
  // I can preserve by push/pop or move into temp / move out of temp.
  // But note! push/pop is not supported for r8 - r15.
}


// Emit the code to unwind the stack frame created by emitEnter, and return.
//
void
emitReturn()
{
  // TO DO: unpreserve the callee-save registers saved by emitEnter.

  // mov rsp, rbp
  emitMov( Operand( jit_Operand_Kind_RegP, regRsp ),
           Operand( jit_Operand_Kind_RegP, regRbp ) );

  // pop rbp
  outB( 0x58 + regRbp->_nativeId );

  // ret
  outB( 0xc3 );
}


// The tCode currently starts with a dummy mini-method that just does call + return,
// with no enter thus no stack frame to unwind.
// I could add tEnter to that method too, to avoid this special case.
void
emitReturnFromInitialStub()
{
  // ret
  outB( 0xc3 );
}


// Emit a call to the given addr.
// Addr may be nullptr if it isn't known yet.
// We use rip-relative addressing, so the addr must be within 2 GB
// of the current code.
//
void
emitCall( char* addr )
{
  // call rip-relative address
  outB( 0xe8 );
  char* nextInstr = nativePc + 4;
  outI( (long) addr - (long) nextInstr );
}


// Emit a call to the given addr,
// which might not be within 2GB of rip.
// e.g. this is used to call runtime library methods.
//
// This is only emitting the call instruction,
// it's not dealing with any calling convention regarding
// parameters, stack frame, return value, caller-save registers.
//
void
emitCallExtern( char* addr )
{
  Operand x( jit_Operand_Kind_RegP, allocateReg() );
  // mov reg, 64-bit address
  emit( Instr( 0xb8 ).regInOpc( x._reg ).w().imm64( (long) addr ) );

  // call [reg]
  emit( Instr( 0xff ).opc( 2 ).rmReg( x._reg ) );
  x.release();
}


// Emit an unconditional jump to the given label.
// Will request patching of this instruction if the label address isn't known yet.
//
void
emitJmpToLabel( int label )
{
  char* addr = findLabel( label );
  emitJmp( addr );
  if ( !addr) {
    requestPatch( (int*) ( nativePc - 4 ), label );
  }
}


// Emit a conditional jump to the given label.
// Will request patching of this instruction if the label address isn't known yet.
//
void
emitJccToLabel( int label, ConditionFlags flags )
{
  char* addr = findLabel( label );
  emitJcc( addr, flags );
  if ( !addr) {
    requestPatch( (int*) ( nativePc - 4 ), label );
  }
}


// Emit an unconditional jump to the given addr.
// Addr may be nullptr if it isn't known yet.
// Using rip-relative addressing, so addr must be within 2 GB
// of the current code.
//
void
emitJmp( char* addr )
{
  // jmp rip-relative address
  outB( 0xe9 );
  char* nextInstr = nativePc + 4;
  outI( (long) addr - (long) nextInstr );
}


// Emit a conditional jump to the given addr.
// Addr may be a nullptr if it isn't known yet.
// Using rip-relative addressing, so addr must be within 2 GB
// of the current code.
//
// Flags is the condition on which we want to jump.
// 
void
emitJcc( char* addr, ConditionFlags flags )
{
  // j<cc> rip-relative address
  // The ConditionFlags were defined to match the second byte of this opcond.
  outB( 0x0f );
  outB( (int) flags );
  char* nextInstr = nativePc + 4;
  outI( (long) addr - (long) nextInstr );
}



// x += y
// x is a register
//
void
emitAdd( const Operand& x, const Operand& y )
{
  // Instruction info from Intel docs
  static std::vector<InstrTempl> templates = {
    InstrTempl( 8,  0x80 ).opc( 0 ).MI(),   // I with size8 will implicitly mean ib i.e. imm8
    InstrTempl( 32, 0x81 ).opc( 0 ).MI(),   // I with size8 will implicitly mean id i.e. imm32
                                            // size32 implicitly allows rex.w to become sixtyfour,
                                            // though I remains imm32 and sign-extends to sixtyfour
    InstrTempl( 32, 0x83 ).opc( 0 ).MI().ySize( 8 ),  // override the default of I being 32 due to size.  imm8 will be sign extended to 32 (or sixtyfour)
    InstrTempl( 8,  0x00 ).MR(),
    InstrTempl( 32, 0x01 ).MR(),
    InstrTempl( 8,  0x02 ).RM(),
    InstrTempl( 32, 0x03 ).RM()
  };

  emit( templates, x, y );
}


// x += y
// x is a floating point register
//
void
emitAddFloat( const Operand& x, const Operand& y )
{
  // addsd does not allow an immediate y
  // Note, the size is implicitly 64 bit and does not need rex.w
  static std::vector<InstrTempl> addsdTemplates = {
    InstrTempl( 64, 0xf2, 0x0f, 0x58 ).RM()
  };

  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegP_DerefP ):
      // TO DO: Do I still need the above?
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegP_DerefD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegD ):
      emit( addsdTemplates, x, y );
      break;

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ConstD ):
      {
        Operand yReg( jit_Operand_Kind_RegD, allocateFloatReg() );
        emitMov( yReg, y );
        emitAddFloat( x, yReg );
        yReg.release();
      }
      break;

    default:
      toDo( "emitAddFloat: unsupported operands\n" );
  }
}


// x -= y
// x is a register
//
void
emitSub( const Operand& x, const Operand& y )
{
  // Instruction info from Intel docs
  static std::vector<InstrTempl> templates = {
    InstrTempl( 8,  0x80 ).opc( 5 ).MI(),
    InstrTempl( 32, 0x81 ).opc( 5 ).MI(),
    InstrTempl( 32, 0x83 ).opc( 5 ).MI().ySize( 8 ),
    InstrTempl( 8,  0x28 ).MR(),
    InstrTempl( 32, 0x29 ).MR(),
    InstrTempl( 8,  0x2a ).RM(),
    InstrTempl( 32, 0x2b ).RM()
  };

  emit( templates, x, y );
}


// x -= y
// x is a floating point register
//
void
emitSubFloat( const Operand& x, const Operand& y )
{
  // subsd doesn't allow immediate y
  static std::vector<InstrTempl> subsdTemplates = {
    InstrTempl( 64, 0xf2, 0x0f, 0x5c ).RM()
  };

  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegP_DerefP ):
      // TO DO: may not need above anymore?

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegP_DerefD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegD ):
      emit( subsdTemplates, x, y );
      break;

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ConstD ):
      {
        Operand yReg( jit_Operand_Kind_RegD, allocateFloatReg() );
        emitMov( yReg, y );
        emitSubFloat( x, yReg );
        yReg.release();
      }
      break;

    default:
      toDo( "emitSubFloat: unsupported operands\n" );
  }
}


// x = -x
// x is a register
//
void
emitNeg( const Operand& x )
{
  static std::vector<InstrTempl> templates = {
    InstrTempl( 8,  0xf6 ).opc( 3 ).M(),
    InstrTempl( 32, 0xf7 ).opc( 3 ).M()
  };

  emit( templates, x );
}


// x = logical exclusive or of x, y
// x is a floating point register (xmm)
//
// Callers use this to load 0.0 into a floating point register.
//
// y is a floating point register or memory.
// But in the memory case, it reads 128 bits.
// If I ever need this, look at the no-prefix version
// which is 64-bit, but uses legacy mm registers rather than xmm.
// So I'm not sure the full set of xmm registers is accessible in that case.
//
void
emitPxor( const Operand& x, const Operand& y )
{
  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegD ):
      // This is actually a 128-bit operation
      emit( Instr( 0x0f, 0xef ).prefix( 0x66 ).reg( x._reg ).rmReg( y._reg ) );
      break;

    default:
      toDo( "emitPxor: unsupported operands\n" );
  }
}


// x * y
// x is a register
//
void
emitMult( const Operand& x, const Operand& y )
{
  // imul - signed multiply
  static std::vector<InstrTempl> templates = {
    InstrTempl( 32, 0x0f, 0xaf ).RM()
    // There's a 3-arg variant that my template system doesn't allow for yet: RMI.  R = M * I
    // Have to check for that in the switch statement below.
  };


  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      emit( templates, x, y );
      break;
 
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
      // imul x_reg32, x_reg32, immediate32   -- i.e. x = x * i
      emit( Instr( 0x69 ).reg( x._reg ).rmReg( x._reg ).imm32( y._value ) );
      break;

    default:
      toDo( "emitMult\n" );
      break;
  }
}


// x * y
// x is a floating point register
//
void
emitMultFloat( const Operand& x, const Operand& y )
{
  // mulsd - multiply scalar double precision floating point
  static std::vector<InstrTempl> mulsdTemplates = {
    InstrTempl( 64, 0xf2, 0x0f, 0x59 ).RM()
  };

  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamP ):
      // TO DO: may not need above anymore?

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegD ):
      emit( mulsdTemplates, x, y );
      break;
 
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ConstD ):
      {
        Operand yReg( jit_Operand_Kind_RegD, allocateFloatReg() );
        emitMov( yReg, y );
        emitMultFloat( x, yReg );
        yReg.release();
      }
      break;

    default:
      toDo( "emitMultFloat unexpected operands\n" );
      break;
  }
}


// sign-extends eax into edx:eax
// This is useful prior to idiv.
// Note this is in the Intel docs on the page for "CWD/CDQ/CQO".
// On linux, gdb reports this instruction as "cltd".
//
void
emitCdq()
{
  outB( 0x99 );
}


// x = x / y
// x is in eax, and already sign-extended into edx
// y is a register or memory, not const.
//
void
emitDiv( const Operand& x, const Operand& y )
{
  // Signed divide.
  // The first argument is implicitly *ax or *dx:*ax.
  // So, the template only takes one operand.

  static std::vector<InstrTempl> templates = {
    InstrTempl( 8,  0xf6 ).opc( 7 ).M(),   // first argument implicitly AX, result in AL (remainder in AH)
    InstrTempl( 32, 0xf7 ).opc( 7 ).M()    // first argument implicitly EDX:EAX, result in EAX (remainder in EDX)
                                           // or 64-bit: first arg in RDX:RAX, result in RAX (remainder in RDX)
  };

  assert( x._reg == regRax );

  emit( templates, y );
}


// x = x / y
// x is a floating point register
//
void
emitDivFloat( const Operand& x, const Operand& y )
{
  static std::vector<InstrTempl> divsdTemplates = {
    InstrTempl( 64, 0xf2, 0x0f, 0x5e ).RM()
  };

  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamP ):
      // Do I still need the above?
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegD ):
      emit( divsdTemplates, x, y );
      break;
 
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ConstD ):
      {
        Operand yReg( jit_Operand_Kind_RegD, allocateFloatReg() );
        emitMov( yReg, y );
        emitDivFloat( x, yReg );
        yReg.release();
      }
      break;

    default:
      toDo( "emitDivFloat unexpected operands\n" );
      break;
  }
}


// x = (double) y
// x is a floating point register
// y is an int32 in a general purpose register or memory.
// 
void
emitCvtIntToDouble( const Operand& x, const Operand& y )
{
  static std::vector<InstrTempl> cvtsi2sdTemplates = {
    // x is 64 bit, y is 32 bit
    InstrTempl( 64, 0xf2, 0x0f, 0x2a ).ySize( 32 ).RM()
  };

  switch ( KindPair( x._kind, y._kind ) ) {
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalI ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalI ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamI ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegI ):
      // cvtsi2sd x_xmm, y_int32
      // emit( Instr( 0xf2, 0x0f, 0x2a ).reg( x._reg ).rm( y._reg ) );
      emit( cvtsi2sdTemplates, x, y );
      break;

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ConstI ):
      // cvtsi2sd doesn't have an immediate option.
      // Caller would have filtered out this case already.
      // Alternatively we can return a ConstD directly here.
      toDo( "emitCvtIntToDouble: unexpected const operand\n" );
      break;

    default:
      toDo( "emitCvtIntToDouble: unexpected operands\n" );
      break;
  }
}


// x << y
// x is a register
//
void
emitShl( const Operand& x, const Operand& y )
{
  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
      toDo( "emitShl\n" );
      // NOTE: SHL only supports y in CL.  Or consider VEX instruction SHLX.
      break;
    
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
      // shl reg32, immediate8
      emit( Instr( 0xc1 ).opc( 4 ).rmReg( x._reg ).imm8( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      toDo( "emitShl\n" );
      // NOTE: SHL only supports y in CL.  Or consider VEX instruction SHLX.
      break;

    default:
      fatal( "emitShl: unexpected operands\n" );
  }
}


// ++x
// x is a register.
//
void
emitInc( const Operand& x )
{
  static std::vector<InstrTempl> templates = {
    InstrTempl( 8,  0xfe ).opc( 0 ).M(),
    InstrTempl( 32, 0xff ).opc( 0 ).M()
  };

  emit( templates, x );
}


// --x
// x is a register.
//
void
emitDec( const Operand& x )
{
  static std::vector<InstrTempl> templates = {
    InstrTempl( 8,  0xfe ).opc( 1 ).M(),
    InstrTempl( 32, 0xff ).opc( 1 ).M()
  };

  emit( templates, x );
}


// Compare x and y
// This produces condition flags.  The caller will create the appropriate result Operand.
// x may not be a constant.
// x and y may not both be memory references.
// Neither x nor y may be Addr_<Var> operands.
//
void
emitCmp( const Operand& x, const Operand& y )
{
  static std::vector<InstrTempl> templates = {
    InstrTempl( 8,  0x80 ).opc( 7 ).MI(),
    InstrTempl( 32, 0x81 ).opc( 7 ).MI(),
    InstrTempl( 32, 0x83 ).opc( 7 ).MI().ySize( 8 ),
    InstrTempl(  8, 0x38 ).MR(),
    InstrTempl( 32, 0x39 ).MR(),
    InstrTempl(  8, 0x3a ).RM(),
    InstrTempl( 32, 0x3b ).RM()
  };

  emit( templates, x, y );
}


// Compare floating point values x and y
// x is a floating point register.
// y is a floating point register or memory reference.
// This produces condition flags.  The caller will create the appropriate result Operand.
//
// The caller should interpret the flags similar to an unsigned integer comparison
// (even though float values are always signed).
//
// TO DO: the caller should do an extra check for the P (unordered?) flag
//   to allow for NAN in the comparison.  I still need to figure out how to handle this.
//   ACTUALLY, the comisd doc says comisd will signal an invalid operation exception
//   for either a quiet nan or signalling nan, while ucomisd will only signal for a
//   signalling nan.  So probably it's not enough to check P after the compare,
//   maybe I also need to use ucomisd.
//
void
emitComisd( const Operand& x, const Operand& y )
{
  static std::vector<InstrTempl> comisdTemplates = {
    InstrTempl( 64, 0x66, 0x0f, 0x2f ).RM()
  };

  emit( comisdTemplates, x, y );
}


// set x to 1 or 0, according to the current condition flags.
// flags indicates which condition should produce 1.
// x is a register.
//
void
emitSet( const Operand& x, ConditionFlags flags )
{
  switch ( x._kind ) {
    case jit_Operand_Kind_RegB:
      // set<cc> x_reg8
      // Note the set opcode is aligned with the condition codes.
      // Note this instruction is unusual in that the reg field of ModR/M is unused and ignored.
      emit( Instr( 0x0f, 0x10 + (int)flags ).rmReg8( x._reg ) );
      break;
    case jit_Operand_Kind_RegI:
      // set<cc> x_reg8  -- this sets only a byte value
      emit( Instr( 0x0f, 0x10 + (int)flags ).rmReg8( x._reg ) );
      // movsx x_reg32, x_reg8  -- sign extend to 32 bits
      emit( Instr( 0x0f, 0xbe ).reg( x._reg ).rmReg8( x._reg ) );
      break;   
    case jit_Operand_Kind_RegP:
      // set<cc> x_reg8  -- this sets only a byte value
      emit( Instr( 0x0f, 0x10 + (int)flags ).rmReg8( x._reg ) );
      // movsx x_reg64, x_reg8  -- sign extend to 64 bits
      emit( Instr( 0x0f, 0xbe ).w().reg( x._reg ).rmReg8( x._reg ) );
      break;
    default:
      fatal( "emitSet: unexpected operands\n" );
  }
}


// x = y
// x and y are not both memory references.
//
// We support move from actual to reg, to help prepare cdecl calls.
//
void
emitMov( const Operand& x, const Operand& y )
{
  static std::vector<InstrTempl> movTemplates = {
    InstrTempl( 8,  0x88 ).MR(),
    InstrTempl( 32, 0x89 ).MR(),
    InstrTempl( 8,  0x8a ).RM(),
    InstrTempl( 32, 0x8b ).RM(),
    // There's an instruction template using operands "OI" which isn't supported by
    // the template mechanism yet.  But, it is equivalent to the MI instructions next.
    // (The only advantage is it avoids a ModR/M byte).
    InstrTempl( 8,  0xc6 ).opc( 0 ).MI(),
    InstrTempl( 32, 0xc7 ).opc( 0 ).MI()
  };

  static std::vector<InstrTempl> movzxTemplates = {
    InstrTempl( 32, 0x0f, 0xb6 ).RM().ySize( 8 )
  };

  // movsd: move scalar double-precision floating point.
  // This instructions operates only on floating point regs and memory.  No general purpose regs.
  // This instruction is implicitly 64-bit and does not need rex.w to indicate that.
  static std::vector<InstrTempl> movsdTemplates = {
    InstrTempl( 64, 0xf2, 0x0f, 0x10 ).RM(),
    InstrTempl( 64, 0xf2, 0x0f, 0x11 ).MR()
  };

  // movd: move double-word (or quadword with rex.w) between
  // floating point registers and general purpose registers or memory.
  //
  // Because my template system doesn't distinguish between float and general
  // registers (yet), I need to have the two directions in two different templates,
  // and the caller must decide which template list to query.
  static std::vector<InstrTempl> movd_x_general = {
    InstrTempl( 32, 0x0f, 0x6e ).prefix( 0x66 ).RM()
  };
  static std::vector<InstrTempl> movd_general_x = {
    InstrTempl( 32, 0x0f, 0x7e ).prefix( 0x66 ).MR()
  };

  static std::vector<InstrTempl> leaTemplates = {
    InstrTempl( 32, 0x8d ).RM()
  };

  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_GlobalB, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_GlobalB, jit_Operand_Kind_RegB ):
    case KindPair( jit_Operand_Kind_GlobalI, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_GlobalI, jit_Operand_Kind_RegI ):
    case KindPair( jit_Operand_Kind_GlobalP, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_GlobalP, jit_Operand_Kind_RegP ):
    case KindPair( jit_Operand_Kind_LocalB, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_LocalB, jit_Operand_Kind_RegB ):
    case KindPair( jit_Operand_Kind_LocalI, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_LocalI, jit_Operand_Kind_RegI ):
    case KindPair( jit_Operand_Kind_LocalP, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_LocalP, jit_Operand_Kind_RegP ):
    case KindPair( jit_Operand_Kind_ParamB, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_ParamB, jit_Operand_Kind_RegB ):
    case KindPair( jit_Operand_Kind_ParamI, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_ParamI, jit_Operand_Kind_RegI ):
    case KindPair( jit_Operand_Kind_ParamP, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_ParamP, jit_Operand_Kind_RegP ):
    case KindPair( jit_Operand_Kind_ActualB, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_ActualB, jit_Operand_Kind_RegB ):
    case KindPair( jit_Operand_Kind_ActualI, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_ActualI, jit_Operand_Kind_RegI ):
    case KindPair( jit_Operand_Kind_ActualP, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_ActualP, jit_Operand_Kind_RegP ):
    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_GlobalB ):
    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_LocalB ):
    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ParamB ):
    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ActualB ):
    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_RegB ):
    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_RegP_DerefB ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ActualI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegP_DerefI ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalP ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalP ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamP ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ActualP ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP_DerefP ):
    case KindPair( jit_Operand_Kind_RegP_DerefB, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_RegP_DerefB, jit_Operand_Kind_RegB ):
    case KindPair( jit_Operand_Kind_RegP_DerefI, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_RegP_DerefI, jit_Operand_Kind_RegI ):
    case KindPair( jit_Operand_Kind_RegP_DerefP, jit_Operand_Kind_ConstI ):
    case KindPair( jit_Operand_Kind_RegP_DerefP, jit_Operand_Kind_RegP ):
      emit( movTemplates, x, y );
      break;


    // MOVZX - zero extend from uint8 to int32

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalB ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalB ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamB ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ActualB ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegB ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegP_DerefB ):
      emit( movzxTemplates, x, y );
      break;


    // MOVSXD - sign extend from int32 to int64

    // TO DO: the template mechanism doesn't handle movsxd yet
    //  because it needs a mix of 64-bit and 32-bit arguments
    //  (while templates treat those as independent variants).

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalI ):
      // movsxd x_reg64, [rip + offsetToGlobal] 32 bits sign-extended to 64 bits
      emit( Instr( 0x63 ).w().reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalI ):
      // movsxd x_reg64, [rbp + y._value] 32 bits sign-extended to 64 bits
      emit( Instr( 0x63 ).w().reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamI ):
      // movsxd x_reg64, [rbp + y._value + FPO] 32 bits sign-extended to 64 bits
      emit( Instr( 0x63 ).w().reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegI ):
      // movsxd x_reg64, y_reg32 sign-extended to 64 bits
      emit( Instr( 0x63 ).w().reg( x._reg ).rmReg( y._reg ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP_DerefI ):
      // movsxd x_reg64, [y_reg64 + offset] 32 bits sign-extended to 64 bits
      emit( Instr( 0x63 ).w().reg( x._reg ).mem( y._reg, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefP, jit_Operand_Kind_RegI ):
      // I can't mov into memory with sign extension in one instruction,
      // because movsxd only moves into register.
      // But I can do it in two steps, moving the register to itself with extension first.
      // On the other hand, maybe I don't need this (same goes for the VarP, RegI cases).
      toDo( "emitMov: moving memoryP, regI with sign extension\n" );
      break;


    // LEA

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Global ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Local ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Param ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Actual ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Reg_Offset ):
      emit( leaTemplates, x, y );
      break;


    // Moving double floating point values.
    //
    // Movsd - between memory and floating point registers

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ActualP ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegP_DerefP ):
    case KindPair( jit_Operand_Kind_GlobalP, jit_Operand_Kind_RegD ):
    case KindPair( jit_Operand_Kind_LocalP, jit_Operand_Kind_RegD ):
    case KindPair( jit_Operand_Kind_ParamP, jit_Operand_Kind_RegD ):
    case KindPair( jit_Operand_Kind_ActualP, jit_Operand_Kind_RegD ):
    case KindPair( jit_Operand_Kind_RegP_DerefP, jit_Operand_Kind_RegD ):
      // I used the above prior to introducing "D" variables.  Probably can remove now.

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_GlobalD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_LocalD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ParamD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ActualD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegD ):
    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_RegP_DerefD ):
    case KindPair( jit_Operand_Kind_GlobalD, jit_Operand_Kind_RegD ):
    case KindPair( jit_Operand_Kind_LocalD, jit_Operand_Kind_RegD ):
    case KindPair( jit_Operand_Kind_ParamD, jit_Operand_Kind_RegD ):
    case KindPair( jit_Operand_Kind_ActualD, jit_Operand_Kind_RegD ):
    case KindPair( jit_Operand_Kind_RegP_DerefD, jit_Operand_Kind_RegD ):
      emit( movsdTemplates, x, y );
      break;

    // ConstD is an imm64 value.  But not many instructions support imm64.
    // Instead, we often need a sequence of instructions.

    case KindPair( jit_Operand_Kind_GlobalP, jit_Operand_Kind_ConstD ):
    case KindPair( jit_Operand_Kind_LocalP, jit_Operand_Kind_ConstD ):
    case KindPair( jit_Operand_Kind_ParamP, jit_Operand_Kind_ConstD ):
    case KindPair( jit_Operand_Kind_ActualP, jit_Operand_Kind_ConstD ):
      // Do we still need the above?
    case KindPair( jit_Operand_Kind_GlobalD, jit_Operand_Kind_ConstD ):
    case KindPair( jit_Operand_Kind_LocalD, jit_Operand_Kind_ConstD ):
    case KindPair( jit_Operand_Kind_ParamD, jit_Operand_Kind_ConstD ):
    case KindPair( jit_Operand_Kind_ActualD, jit_Operand_Kind_ConstD ):
      {
        int* yWords = (int*) &y._double;
        Operand yLow( jit_Operand_Kind_ConstI, yWords[0] );
        Operand yHigh( jit_Operand_Kind_ConstI, yWords[1] );
        emitMov( operandLowWord( x ), yLow );
        emitMov( operandHighWord( x ), yHigh );
      }
      break;

    case KindPair( jit_Operand_Kind_RegD, jit_Operand_Kind_ConstD ):
      {
        // I'll put the const in a general purpose reg using imm64,
        // then move from there to the floating point reg.
        Operand yReg( jit_Operand_Kind_RegP, allocateReg() );
        int64_t yLong = *(int64_t*) &y._double;
        // mov reg, imm64
        emit( Instr( 0xb8 ).regInOpc( yReg._reg ).w().imm64( yLong ) );
        // movq X_xmm64, Y_reg64
        emit( movd_x_general, x, yReg );
        yReg.release();
      }
      break;

    // mov reg, imm64

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ConstI ):
      // mov reg64, immediate64 (which I sign-extend from the given 32-bit constant).
      // NOTE: this instruction really does take immediate64 rather than simply
      // sign-extending immediate32.
      emit( Instr( 0xb8 ).regInOpc( x._reg ).w().imm64( (long) y._value ) );
      break;


    // set reg, flags

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_Flags ):
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_Flags ):
    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Flags ):
      emitSet( x, y._flags );
      break;

    default:
      fatal( "emitMov: unexpected operands %s, %s at %d\n",
             x.describe().c_str(), y.describe().c_str(), (int)( tCodePc - tCode ) );
  }
}


// Emit a REX prefix if necessary.
// overrideWidth is true if we need to change a 32-bit operation to 64-bit.
// reg is the register that will be specified in the instruction's ModRM.reg field (if any).
//     The REX.R bit gives access to the extended set of registers there.
// rmReg is the register that will be specified in the instruction's ModRM.rm field
//     or the SIB.base field (if any).
//     Or for instuctions without any ModR/M byte, the register specified in the opcode's own reg field.
//     The REX.B field gives access to extended registers there.
//     Note that some instructions (e.g. ADD REG, immediate) use the Mod/RM rm field for the register,
//     because they use the reg field as an opcode extension.   So be sure to pass the register
//     to this method's "baseReg" in that case.
// r8 is true if reg is an 8-bit register.
// rmReg8 is true if rmReg is an 8-bit register.
//     We need to know the above in order to encode a reference to 8-bit register in the
//     low byte of rsi/rdi/rsp/rbp.   This is done via the REX prefix, potentially a blank one.
//     When a REX prefix is used, the encoding refers to the low byte of the 16 registers,
//     rather than the legacy ah/bh/ch/dh.
//
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
emitRex( bool overrideWidth, const Register* reg, const Register* rmReg,
         bool reg8, bool rmReg8 )
{
  int rex = 0x40;
  if ( overrideWidth ) {
    rex |= 8;  // REX.W
  }
  if ( reg && reg->_nativeId >= 8 ) {
    rex |= 4;  // REX.R
  }
  if ( rmReg && rmReg->_nativeId >= 8 ) {
    rex |= 1;  // REX.B
  }
  if ( rex != 0x40 ) {
    outB( rex );
  } else if ( reg8 && ( reg == regRsi ||
                        reg == regRdi ||
                        reg == regRsp ||
                        reg == regRbp ) ) {
    // still issue a blank rex
    outB( rex );
  } else if ( rmReg8 && ( rmReg == regRsi ||
                          rmReg == regRdi ||
                          rmReg == regRsp ||
                          rmReg == regRbp ) ) {
    // still issue a blank rex
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


// This form of ModR/M is used for an immediate register, not a memory reference.
// opcodeExtension is a bit pattern specified for the instruction,
// and is stored in the "Reg" field of ModR/M.    rm is a register stored in the "RM" field.
// So, this form is only used for instructions that use one register.
//
// It is necessary to check the Intel documentation for a specific instruction
// to know which of these ModR/M encodings should be used.
//
void
emitModRM_OpcRMReg( int opcodeExtension, const Register* rm )
{
  // mod 11
  outB( 0xc0 | (opcodeExtension << 3) | regIdLowBits( rm->_nativeId ) );
}


void
emitModRM_RegReg( const Register* reg, const Register* rmReg )
{
  // mod 11
  // There are some rare instructions where the reg field is unused and ignored (e.g. set<cc>).
  // So allow for reg == nullptr.
  outB( 0xc0 | ( regIdLowBits( reg ? reg->_nativeId : 0 ) << 3) | regIdLowBits( rmReg->_nativeId ) );
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
  printf( "Jit - program loaded (%d words)\n", tCodeLen );
}


// Called with tCodePc pointing just after the opcode.
// On exit, tCodePc points to the next instruction.
//
void
tCodeNotImplemented()
{
  --tCodePc;
  int opcode = *tCodePc;
  const char* name = tcodeInstrName( opcode );
  if ( name == nullptr ) {
    fatal( "bad instruction %d\n", *tCodePc );
  }
  printf( "Warning: tCode operation %s not supported yet\n", name );
  tCodePc += tcodeInstrSize( opcode );
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


// Dump machine code to a text file on disk, to help with debugging.
//
void
dumpNativeCode()
{
  std::string dumpCodeFilename( filename );
  dumpCodeFilename.append( ".jit_out.txt" );
  dumpCodeFile = fopen( dumpCodeFilename.c_str(), "w" );
  if ( !dumpCodeFile ) {
    printf(" jit can't open code dump file %s\n", dumpCodeFilename.c_str() );
    return;
  }

  // For now, this assumes that native code is all allocated in one contiguous block
  for ( char* addr = nativeCode; addr < nativePc; ++addr ) {
    fprintf( dumpCodeFile, "%02x\n", (unsigned char) *addr );
  }

  fclose( dumpCodeFile );
}



void
executeCode()
{
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


