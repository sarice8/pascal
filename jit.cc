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
char and byte types not implemented.
   - note both of these are unsigned, so I need careful handling of extension to int.
     Currently all my 1-byte operations would sign-extend to int.
   - note char and byte are not assignment compatible, but can convert between them with
     chr(byte) and ord(char).   Literal one-character string is type char.
file type not implemented.
enum type not implemented.
set type not implemented.
unions ( "case" inside record definition )
case statement

goto / label
exit[(value)] - an extension to leave a procedure / function (with optional return value) / program.

boolean variables may not be fully fleshed out, depending on what operators are allowed on them
(aside from and or not equal not-equal).

Support other flavors of integer - unsigned int, int64, int8.  (Look up the complete set).
These need their own tcode operators and jit operand types, e.g. to produce different comparision flags.

Dynamic memory allocation functions.  Mainly new, dispose (also ReAllocMem).  fpc has a bunch of others
but not sure how standard: https://www.tutorialspoint.com/pascal/pascal_memory.htm

Support type casts.

Proper type checking for method forward declaration vs body declaration.  Type matching rule
that folks apparently use is that must have the same type declaration, not just the same content of
the type declaration.

Add missing standard functions like sin().

Maybe add in some of the graphics functions that I think Turbo Pascal had.
Or, at least some simple modern graphics library, so I can make some mini game/toy programs.

Support units, for multiple source files in project, and a way to package extern library methods
e.g. whatever graphical utility I want to make available.
Note, I should have a way to mark the method with cdecl or similar, to know that I need to use
the native calling convention, in addition to absolute code address.

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
void finishMethod();
void tCodeNotImplemented( int opcode );
void executeCode();
void fatal( const char* msg, ... );
void toDo( const char* msg, ... );


char* filename = 0;
bool optionListing = true;
FILE* listingFile = nullptr;

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
  jit_Operand_Kind_Addr_Reg_Offset,

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
  jit_Operand_Kind_RegP_DerefP,

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
  Register( const std::string& name, int nativeId, bool calleeSaveLinux64, bool calleeSaveWin64 )
    : _name( name ), _nativeId( nativeId ), _calleeSave( calleeSaveLinux64 ),
      _calleeSaveWin64( calleeSaveWin64 )
    {}

  void release() { _inUse = false; }

  std::string _name = "";
  int _nativeId = 0;
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


Register* regRax = new Register( "rax", 0, false, false );
Register* regRcx = new Register( "rcx", 1, false, false );
Register* regRdx = new Register( "rdx", 2, false, false ); 
Register* regRbx = new Register( "rbx", 3, true, true );
Register* regRsp = new Register( "rsp", 4, false, false );
Register* regRbp = new Register( "rbp", 5, true, true );
Register* regRsi = new Register( "rsi", 6, false, true );
Register* regRdi = new Register( "rdi", 7, false, true );
Register* regR8  = new Register( "r8", 8, false, false );
Register* regR9  = new Register( "r9", 9, false, false );
Register* regR10 = new Register( "r10", 10, false, false );
Register* regR11 = new Register( "r11", 11, false, false );
Register* regR12 = new Register( "r12", 12, true, true );
Register* regR13 = new Register( "r13", 13, true, true );
Register* regR14 = new Register( "r14", 14, true, true );
Register* regR15 = new Register( "r15", 15, true, true );


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

std::vector<Register*> paramRegsLinux64 = { regRdi, regRsi, regRdx, regRcx, regR8, regR9 };
std::vector<Register*> paramRegsWin64 = { regRcx, regRdx, regR8, regR9 };
// TO DO: there are also differences in use of XMM/YMM/ZMM registers as params.  Floating point. (Also vector? or no?)
// TO DO: there are also differences for return value.

// For now, hardcoded for linux64
std::vector<Register*>& paramRegs = paramRegsLinux64;


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
                              _kind <= jit_Operand_Kind_RegP; }
  bool isVar() const { return _kind >= jit_Operand_Kind_GlobalB &&
                              _kind <= jit_Operand_Kind_ActualP; }
  bool isAddrOfVar() const { return _kind >= jit_Operand_Kind_Addr_Global &&
                                    _kind <= jit_Operand_Kind_Addr_Reg_Offset; }
  bool isVarOrAddrOfVar() const { return isVar() || isAddrOfVar(); }
  bool isDeref() const { return _kind >= jit_Operand_Kind_RegP_DerefB &&
                                _kind <= jit_Operand_Kind_RegP_DerefP; }
  bool isMem() const { return isVar() || isAddrOfVar() || isDeref(); }
  bool isConst() const { return _kind == jit_Operand_Kind_ConstI; }
  bool isFlags() const { return _kind == jit_Operand_Kind_Flags; }
  int  size() const;  // valid size of the operand value in bytes (1, 4, or 8)
  void release();     // don't need this register anymore (if any)

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
  Register* _reg = nullptr;    // for jit_Operand_Kind_Reg*
  ConditionFlags _flags = FlagInvalid;  // for jit_Operand_Kind_Flags
};


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
void operandIntoSpecificReg( Operand& x, Register* reg, int size );
void operandKindAddrIntoReg( Operand& x );
void operandNotMem( Operand& x );
void operandDeref( Operand& x, int valueSize );
void operandExtendToP( Operand& x );
Operand operandCompare( Operand& x, Operand& y, ConditionFlags flags );
void operandFlagsToValue( Operand& x, int size );

Register* allocateReg();
void forceAllocateReg( Register* reg );
Register* getUpFrame( int uplevels );
Operand allocateTemp( int size );
void preserveRegsAcrossCall();
jitOperandKind operandKindReg( int size );


std::unordered_map<int, char*> labels;
std::unordered_map<int, int> labelAliases;
std::unordered_map<int, char*> unresolvedExternLabels;
std::vector< std::pair< int*, int> > patches;

void defineLabel( int label, char* addr );
void defineLabelAlias( int label, int aliasToLabel );
void defineLabelExtern( int label, char* name );
char* findLabel( int label );
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
void emitSub( const Operand& x, const Operand& y );
void emitNeg( const Operand& x );
void emitMult( const Operand& x, const Operand& y );
void emitCdq();
void emitDiv( const Operand& x, const Operand& y );
void emitShl( const Operand& x, const Operand& y );
void emitInc( const Operand& x );
void emitDec( const Operand& x );
void emitCmp( const Operand& x, const Operand& y );
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

// runtime library methods
extern void runlibWriteI( int val );
extern void runlibWriteBool( bool val );
extern void runlibWriteStr( char* ptr );
extern void runlibWriteP( char* ptr );
extern void runlibWriteCR();

extern void grInit();
extern void grTerm();
extern void runlibClearScreen();
extern void runlibUpdateScreen();
extern void runlibSetPixel( int x, int y, int color );
extern int  runlibGetPixel( int x, int y );
extern void runlibDelay( int milliseconds );
extern int  runlibWaitKey();


// For now, I have a hardcoded list of available external methods.
//
std::unordered_map<std::string, void*> runlibMethods = {
  { "runlibClearScreen", (void*) runlibClearScreen },
  { "runlibUpdateScreen", (void*) runlibUpdateScreen },
  { "runlibSetPixel", (void*) runlibSetPixel },
  { "runlibGetPixel", (void*) runlibGetPixel },
  { "runlibDelay", (void*) runlibDelay },
  { "runlibWaitKey", (void*) runlibWaitKey },
};


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
  printf( "Usage:   jit [-l] <file>\n" );
  printf( "  -l : create listing\n" );
  exit( status );
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

  // requests the REX.W flag, converting a 32-bit instruction to 64-bit.
  Instr& w();

  // Indicate that the instruction is using an 8-bit register.
  // This may necessitate use of a blank REX prefix (to refer to the low byte of rsi/rdi
  // or rsp/rbp if I want that in the future).
  Instr& r8();

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

  // Specifies a memory reference, involving a base register and an offset.
  // The register goes in the r/m field of ModR/M.
  Instr& mem( Register* base, int offset );

  // specifies a memory reference relative to the address of the instruction following this one.
  Instr& memRipRelative( char* targetAddr );

  // specifies an immediate 8-bit value.
  Instr& imm8( char value );

  // specifies an immediate 32-bit value.   For 64-bit instructions, the cpu will sign-extend it.
  Instr& imm32( int value );

  void emit() const;

private:
  char _opcode1 = 0;
  char _opcode2 = 0;

  bool _wide = false;
  char _opcodeExtension = 0;
  char* _memRipRelative = nullptr;
  char _imm8 = 0;
  int _imm32 = 0;
  Register* _reg = nullptr;
  Register* _base = nullptr;
  bool _reg8 = false;
  bool _rmReg8 = false;
  int _offset = 0;

  bool _haveOpcode2 = false;
  bool _haveOpcodeExtension = false;
  bool _haveMemRipRelative = false;
  bool _haveImm8 = false;
  bool _haveImm32 = false;
  bool _baseIsImmediate = false;
};


Instr::Instr( char opcode )
  : _opcode1( opcode )
{}

Instr::Instr( char opcode1, char opcode2 )
  : _opcode1( opcode1 ), _opcode2( opcode2 ), _haveOpcode2( true )
{}


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


void
Instr::emit() const
{
  // optional REX prefix.
  // This gives access to registers r8-r15, and/or reequests 64-bit data width.
  emitRex( _wide, _reg, _base, _reg8, _rmReg8 );

  // opcode
  outB( _opcode1 );
  if ( _haveOpcode2 ) {
    outB( _opcode2 );
  }

  // For rip-relative instructions, we'll need the number of bytes between the
  // Mod/RM byte and the next instruction.  That's just the imm* argument, if any.
  int immBytes = 0;
  if ( _haveImm8 ) {
    immBytes = 1;
  } else if ( _haveImm32 ) {
    immBytes = 4;
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
  }
}


void
emit( const Instr& instr )
{
  instr.emit();
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
      case tPushConstI : {
          operandStack.emplace_back( jit_Operand_Kind_ConstI, *tCodePc++ );
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
          operandIntoSpecificReg( x, regRax, 4 );
          forceAllocateReg( regRdx );
          Operand y = operandStack.back();   operandStack.pop_back();
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
      case tEqualI : {
          Operand y = operandStack.back();   operandStack.pop_back();
          Operand x = operandStack.back();   operandStack.pop_back();
          if ( doConst && x.isConst() && y.isConst() ) {
            operandStack.emplace_back( x._kind, x._value == y._value ? 1 : 0 );
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
            operandStack.emplace_back( x._kind, x._value != y._value ? 1 : 0 );
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
            operandStack.emplace_back( x._kind, x._value > y._value ? 1 : 0 );
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
            operandStack.emplace_back( x._kind, x._value < y._value ? 1 : 0 );
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
            operandStack.emplace_back( x._kind, x._value >= y._value ? 1 : 0 );
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
            operandStack.emplace_back( x._kind, x._value <= y._value ? 1 : 0 );
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
      case tLabel : {
          defineLabel( *tCodePc++, nativePc );
        }
        break;
      case tLabelAlias : {
          int label = *tCodePc++;
          int aliasToLabel = *tCodePc++;
          defineLabelAlias( label, aliasToLabel );
        }
        break;
      case tLabelExtern : {
          int label = *tCodePc++;
          int nameOffset = *tCodePc++;
          // Note: I need to see this prior to any calls,
          // but that should happen since this is issued just prior to first call to the method.
          defineLabelExtern( label, &data[nameOffset] );
        }
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
      case tWriteStr : {
          Operand x = operandStack.back();   operandStack.pop_back();
          Register* paramReg1 = paramRegs[0];
          operandIntoSpecificReg( x, paramReg1, 8 );
          emitCallExtern( (char*) runlibWriteStr );
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
      case tWriteCR : {
          emitCallExtern( (char*) runlibWriteCR );
        }
        break;

      default:
        --tCodePc;
        fatal( "bad instruction %d\n", *tCodePc );
    }
  }

  finishMethod();
  makePatches();
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
operandKindReg( int size )
{
  switch ( size ) {
    case 1: return jit_Operand_Kind_RegB;
    case 4: return jit_Operand_Kind_RegI;
    case 8: return jit_Operand_Kind_RegP;
    default:
      fatal( "operandKindReg: unexpected size %d\n", size );
      return jit_Operand_Kind_Illegal;
  }
}


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


// Force reg to be available (possibly by dumping to a temporary),
// and allocate it.
// TO DO: I could also dump into a different register.
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
        Operand newX = allocateTemp( x.size() );
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
      Operand newX = allocateTemp( x.size() );
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


// Check in a tAssign* is assigning to an actual.
// If so, and we're evaluating params of a cdecl call,
// note if this should get assigned to a register.
//
// At the moment, the assignment into the stack always proceeds,
// and we'll copy into register later via cdeclParamsIntoRegisters().
//
// Note this will pick up a function result's dummy VAR parameter too.
//
void
cdeclCheckForAssignToActual( Operand& x, jitOperandKind actualKind )
{
  if ( ( x._kind == jit_Operand_Kind_Addr_Actual ) &&
        ( callInfos.size() > 0 ) &&
        ( callInfos.back()._cdecl == true ) ) {
    CallInfo& ci = callInfos.back();
    // only supporting cdecl methods that can fit all params in regs
    assert( ci._actualsToRegs.size() < paramRegs.size() );
    Operand actualOnStack( actualKind, x._value );
    Register* reg = paramRegs[ ci._actualsToRegs.size() ];
    ci._actualsToRegs.push_back( { actualOnStack, reg } );
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
    // For all my currently supported cases,
    // the cdecl return value is in rax
    Operand cdeclResult( operandKindReg( ci._returnValue.size() ), regRax );
    emitMov( ci._returnValue, cdeclResult );
  }  
}



// Allocate a temporary variable of the given byte size (1, 4, 8).
//
Operand
allocateTemp( int size )
{
  currLocalSpace += size;
  jitOperandKind newKind = jit_Operand_Kind_Illegal;
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
  return Operand( newKind, -currLocalSpace );
}


// Define the given label to mean the given native address.
//
void
defineLabel( int label, char* addr )
{
  labels[label] = addr;

  if ( listingFile ) {
    fprintf( listingFile, "%p\tLabel %d\n", addr, label );
  }
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
  auto it = runlibMethods.find( std::string(name) );
  if ( it != runlibMethods.end() ) {
    labels[label] = (char*) it->second;
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
    case jit_Operand_Kind_LocalP:
    case jit_Operand_Kind_ParamP:
    case jit_Operand_Kind_ActualP:
    case jit_Operand_Kind_Addr_Global:
    case jit_Operand_Kind_Addr_Local:
    case jit_Operand_Kind_Addr_Param:
    case jit_Operand_Kind_Addr_Actual:
    case jit_Operand_Kind_Addr_Reg_Offset:
    case jit_Operand_Kind_RegP:
    case jit_Operand_Kind_RegP_DerefP:
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
      desiredKind = jit_Operand_Kind_RegP;
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
      break;      break;    
    default:
      fatal( "operandDeref: unexpected operands\n" );
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

    Operand result = Operand( kind, allocateReg() );
    emitSet( result, x._flags );
    x = result;
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
  emitRex( true, nullptr, x._reg );
  outB( 0xb8 | regIdLowBits( x._reg->_nativeId ) );
  outL( (long) addr );

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
      // add reg_x, [rip + offsetToGlobal]
      emit( Instr( 0x03 ).reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
      // add reg_x, [rbp + y.value];
      emit( Instr( 0x03 ).reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
      // add reg_x, [rbp + y.value + FPO]
      emit( Instr( 0x03 ).reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
      // add reg32, immediate32
      emit( Instr( 0x81 ).opc( 0 ).rmReg( x._reg ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      // add reg_x32, reg_y32
      emit( Instr( 0x03 ).reg( x._reg ).rmReg( y._reg ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalP ):
      // add reg_x64, [rip + offsetToGlobal]
      emit( Instr( 0x03 ).w().reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalP ):
      // add reg_x64, [rbp + y.value];
      emit( Instr( 0x03 ).w().reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamP ):
      // add reg_x64, [rbp + y.value + FPO]
      emit( Instr( 0x03 ).w().reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ConstI ):
      // add reg64, immediate32 sign-extended to 64 bits
      emit( Instr( 0x81 ).w().opc( 0 ).rmReg( x._reg ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP ):
      // add x_reg64, y_reg64
      emit( Instr( 0x01 ).w().reg( y._reg ).rmReg( x._reg ) );
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
      // sub x_reg32, [rip + offsetToGlobal]
      emit( Instr( 0x2b ).reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
      // sub x_reg32, [rbp+offset]
      emit( Instr( 0x2b ).reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
      // sub x_reg32, [rbp + offset + FPO]
      emit( Instr( 0x2b ).reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
      // sub reg32, immediate32
      emit( Instr( 0x81 ).opc( 5 ).rmReg( x._reg ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      // sub x_reg32, y_reg32
      emit( Instr( 0x2b ).reg( x._reg ).rmReg( y._reg ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalP ):
      // sub x_reg64, [rip + offsetToGlobal]
      emit( Instr( 0x2b ).w().reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalP ):
      // sub x_reg32, [rbp+offset]
      emit( Instr( 0x2b ).reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamP ):
      // sub x_reg32, [rbp + offset + FPO]
      emit( Instr( 0x2b ).w().reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ConstI ):
      // sub reg64, immediate32 sign-extended to 64 bits
      emit( Instr( 0x81 ).w().opc( 5 ).rmReg( x._reg ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP ):
      // sub x_reg64, y_reg64
      emit( Instr( 0x2b ).w().reg( x._reg ).rmReg( y._reg ) );
      break;

    default:
      fatal( "emitSub: unexpected operands\n" );
  }
}


// x = -x
// x is a register
//
void
emitNeg( const Operand& x )
{
  switch ( x._kind ) {

    case jit_Operand_Kind_RegB:
      emit( Instr( 0xf6 ).opc( 3 ).rmReg8( x._reg ) );
      break;

    case jit_Operand_Kind_RegI:
      emit( Instr( 0xf7 ).opc( 3 ).rmReg( x._reg ) );
      break;

    case jit_Operand_Kind_RegP:
      emit( Instr( 0xf7 ).w().opc( 3 ).rmReg( x._reg ) );
      break;

    default:
      fatal( "emitNeg: unexpected operands\n" );
  }
}


// x * y
// x is a register
//
void
emitMult( const Operand& x, const Operand& y )
{
  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalI ):
      // imul reg32, [rip + offsetToGlobal]
      emit( Instr( 0x0f, 0xaf ).reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
      // imul reg32, [rbp + y._value]
      emit( Instr( 0x0f, 0xaf ).reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
      // imul reg32, [rbp + y._value + FPO]
      emit( Instr( 0x0f, 0xaf ).reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;
 
    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
      // imul x_reg32, x_reg32, immediate32   -- i.e. x = x * i
      emit( Instr( 0x69 ).reg( x._reg ).rmReg( x._reg ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      // imul x_reg32, y_reg32
      emit( Instr( 0x0f, 0xaf ).reg( x._reg ).rmReg( y._reg ) );
      break;

    default:
      toDo( "emitMult\n" );
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
  assert( x._reg == regRax );

  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalI ):
      // idiv [rip + offsetToGlobal]
      emit( Instr( 0xf7 ).opc( 7 ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
      // idiv [rbp + y._value]
      emit( Instr( 0xf7 ).opc( 7 ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
      // idiv [rbp + y._value + FPO]
      emit( Instr( 0xf7 ).opc( 7 ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      // idiv y_reg32
      emit( Instr( 0xf7 ).opc( 7 ).rmReg( y._reg ) );
      break;

    default:
      fatal( "emitDiv: unexpected operands\n" );
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
      fatal( "emitMov: unexpected operands\n" );
  }
}


// ++x
// x is a register.
//
void
emitInc( const Operand& x )
{
  switch ( x._kind ) {

    case jit_Operand_Kind_RegB:
      // inc reg8
      emit( Instr( 0xfe ).opc( 0 ).rmReg8( x._reg ) );
      break;

    case jit_Operand_Kind_RegI:
      // inc reg32
      emit( Instr( 0xff ).opc( 0 ).rmReg( x._reg ) );
      break;

    case jit_Operand_Kind_RegP:
      // inc reg64
      emit( Instr( 0xff ).w().opc( 0 ).rmReg( x._reg ) );
      break;

    default:
      fatal( "emitInc: unexpected operands\n" );
  }
}


// --x
// x is a register.
//
void
emitDec( const Operand& x )
{
  switch ( x._kind ) {

    case jit_Operand_Kind_RegB:
      // dec reg8
      emit( Instr( 0xfe ).opc( 1 ).rmReg8( x._reg ) );
      break;

    case jit_Operand_Kind_RegI:
      // dec reg32
      emit( Instr( 0xff ).opc( 1 ).rmReg( x._reg ) );
      break;

    case jit_Operand_Kind_RegP:
      // dec reg64
      emit( Instr( 0xff ).w().opc( 1 ).rmReg( x._reg ) );
      break;

    default:
      fatal( "emitDec: unexpected operands\n" );
  }
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
  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_GlobalB, jit_Operand_Kind_ConstI ):
      // cmp [rip + offsetToGlobal], immediate8 (truncated from the given int32)
      emit( Instr( 0x80 ).opc( 7 ).memRipRelative( &data[x._value] ).imm8( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_GlobalB, jit_Operand_Kind_RegB ):
      // cmp [rip + offsetToGlobal], reg8
      emit( Instr( 0x38 ).reg( y._reg ).memRipRelative( &data[x._value] ) );
      break;

    case KindPair( jit_Operand_Kind_GlobalI, jit_Operand_Kind_ConstI ):
      // cmp [rip + offsetToGlobal], immediate32
      emit( Instr( 0x81 ).opc( 7 ).memRipRelative( &data[x._value] ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_GlobalI, jit_Operand_Kind_RegI ):
      // cmp [rip + offsetToGlobal], reg32
      emit( Instr( 0x39 ).reg( y._reg ).memRipRelative( &data[x._value] ) );
      break;

    case KindPair( jit_Operand_Kind_GlobalP, jit_Operand_Kind_ConstI ):
      // cmp [rip + offsetToGlobal], immediate32 sign-extended to 64 bits
      emit( Instr( 0x81 ).w().opc( 7 ).memRipRelative( &data[x._value] ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_GlobalP, jit_Operand_Kind_RegP ):
      // cmp [rip + offsetToGlobal], reg64
      emit( Instr( 0x39 ).w().reg( y._reg ).memRipRelative( &data[x._value] ) );
      break;

    case KindPair( jit_Operand_Kind_LocalB, jit_Operand_Kind_ConstI ):
      // cmp [rbp + x._value], immediate8 (truncated from given int32)
      emit( Instr( 0x80 ).opc( 7 ).mem( regRbp, x._value ).imm8( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_LocalB, jit_Operand_Kind_RegB ):
      // cmp [rbp + x._value], reg8
      emit( Instr( 0x38 ).reg( y._reg ).mem( regRbp, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_LocalI, jit_Operand_Kind_ConstI ):
      // cmp [rbp + x._value], immediate32
      emit( Instr( 0x81 ).opc( 7 ).mem( regRbp, x._value ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_LocalI, jit_Operand_Kind_RegI ):
      // cmp [rbp + x._value], reg32
      emit( Instr( 0x39 ).reg( y._reg ).mem( regRbp, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_LocalP, jit_Operand_Kind_ConstI ):
      // cmp [rbp + x._value], immediate32 sign-extended to 64 bits
      emit( Instr( 0x81 ).w().opc( 7 ).mem( regRbp, x._value ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_LocalP, jit_Operand_Kind_RegP ):
      // cmp [rbp + x._value], reg64
      emit( Instr( 0x39 ).w().reg( y._reg ).mem( regRbp, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_ParamB, jit_Operand_Kind_ConstI ):
      // cmp [rbp + x._value + FPO], immediate8 (truncated from given int32)
      emit( Instr( 0x80 ).opc( 7 ).mem( regRbp, x._value + FPO ).imm8( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_ParamB, jit_Operand_Kind_RegB ):
      // cmp [rbp + x._value + FPO], reg8
      emit( Instr( 0x38 ).reg( y._reg ).mem( regRbp, x._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_ParamI, jit_Operand_Kind_ConstI ):
      // cmp [rbp + x._value + FPO], immediate32
      emit( Instr( 0x81 ).opc( 7 ).mem( regRbp, x._value + FPO ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_ParamI, jit_Operand_Kind_RegI ):
      // cmp [rbp + x._value + FPO], reg32
      emit( Instr( 0x39 ).reg( y._reg ).mem( regRbp, x._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_ParamP, jit_Operand_Kind_ConstI ):
      // cmp [rbp + x._value + FPO], immediate32 sign-extended to 64 bits
      emit( Instr( 0x81 ).w().opc( 7 ).mem( regRbp, x._value + FPO ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_ParamP, jit_Operand_Kind_RegP ):
      // cmp [rbp + x._value + FPO], reg64
      emit( Instr( 0x39 ).w().reg( y._reg ).mem( regRbp, x._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_GlobalB ):
      // cmp reg8, [rip + offsetToGlobal]
      emit( Instr( 0x3a ).reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_LocalB ):
      // cmp reg8, [rbp + y.value]
      emit( Instr( 0x3a ).reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ParamB ):
      // cmp reg8, [rbp + y.value + FPO]
      emit( Instr( 0x3a ).reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ConstI ):
      // cmp reg8, immediate8 (truncated from given int32 )
      emit( Instr( 0x80 ).opc( 7 ).rmReg( x._reg ).imm8( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_RegB ):
      // cmp x_reg8, y_reg8
      emit( Instr( 0x3a ).reg( x._reg ).rmReg( y._reg ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalI ):
      // cmp reg32, [rip + offsetToGlobal]
      emit( Instr( 0x3b ).reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
      // cmp reg32, [rbp + y.value]
      emit( Instr( 0x3b ).reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
      // cmp reg32, [rbp + y.value + FPO]
      emit( Instr( 0x3b ).reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
      // cmp reg32, immediate32
      emit( Instr( 0x81 ).opc( 7 ).rmReg( x._reg ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      // cmp x_reg32, y_reg32
      emit( Instr( 0x3b ).reg( x._reg ).rmReg( y._reg ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalP ):
      // cmp reg64, [rip + offsetToGlobal]
      emit( Instr( 0x3b ).w().reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalP ):
      // cmp reg64, [rbp + y.value]
      emit( Instr( 0x3b ).w().reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamP ):
      // cmp reg64, [rbp + y.value + FPO]
      emit( Instr( 0x3b ).w().reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ConstI ):
      // cmp reg64, immediate32 sign extended to 64 bits
      emit( Instr( 0x81 ).w().opc( 7 ).rmReg( x._reg ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP ):
      // cmp x_reg64, y_reg64
      emit( Instr( 0x3b ).w().reg( x._reg ).rmReg( y._reg ) );
      break;

    default:
      fatal( "emitCmp: unexpected operands\n" );
  }
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
  switch ( KindPair( x._kind, y._kind ) ) {

    case KindPair( jit_Operand_Kind_GlobalB, jit_Operand_Kind_ConstI ):
      // mov [rip + offsetToGlobal], immmediate8
      emit( Instr( 0xc6 ).opc( 0 ).memRipRelative( &data[x._value] ).imm8( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_GlobalB, jit_Operand_Kind_RegB ):
      // mov [rip + offsetToGlobal], reg8
      emit( Instr( 0x88 ).reg8( y._reg ).memRipRelative( &data[x._value] ) );
      break;

    case KindPair( jit_Operand_Kind_GlobalI, jit_Operand_Kind_ConstI ):
      // mov [rip + offsetToGlobal], immmediate32
      emit( Instr( 0xc7 ).opc( 0 ).memRipRelative( &data[x._value] ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_GlobalI, jit_Operand_Kind_RegI ):
      // mov [rip + offsetToGlobal], reg32
      emit( Instr( 0x89 ).reg( y._reg ).memRipRelative( &data[x._value] ) );
      break;

    case KindPair( jit_Operand_Kind_GlobalP, jit_Operand_Kind_ConstI ):
      // mov [rip + offsetToGlobal], immmediate32 sign-extended to 64 bits
      emit( Instr( 0xc7 ).w().opc( 0 ).memRipRelative( &data[x._value] ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_GlobalP, jit_Operand_Kind_RegP ):
      // mov [rip + offsetToGlobal], reg64
      emit( Instr( 0x89 ).w().reg( y._reg ).memRipRelative( &data[x._value] ) );
      break;

    case KindPair( jit_Operand_Kind_LocalB, jit_Operand_Kind_ConstI ):
      // mov [rbp+offset], immediate8
      emit( Instr( 0xc6 ).opc( 0 ).mem( regRbp, x._value ).imm8( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_LocalB, jit_Operand_Kind_RegB ):
      // mov [rbp+offset], reg8
      emit( Instr( 0x88 ).reg8( y._reg ).mem( regRbp, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_LocalI, jit_Operand_Kind_ConstI ):
      // mov [rbp+offset], immediate32
      emit( Instr( 0xc7 ).opc( 0 ).mem( regRbp, x._value ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_LocalI, jit_Operand_Kind_RegI ):
      // mov [rbp+offset], reg32
      emit( Instr( 0x89 ).reg( y._reg ).mem( regRbp, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_LocalP, jit_Operand_Kind_ConstI ):
      // mov [rbp+offset], immediate32 sign-extended to 64 bits
      emit( Instr( 0xc7 ).w().opc( 0 ).mem( regRbp, x._value ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_LocalP, jit_Operand_Kind_RegP ):
      // mov [rbp+offset], reg64
      emit( Instr( 0x89 ).w().reg( y._reg ).mem( regRbp, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_ParamB, jit_Operand_Kind_ConstI ):
      // mov [rbp + offset + FPO], immediate8
      emit( Instr( 0xc6 ).opc( 0 ).mem( regRbp, x._value + FPO ).imm8( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_ParamB, jit_Operand_Kind_RegB ):
      // mov [rbp + offset + FPO], reg8
      emit( Instr( 0x88 ).reg8( y._reg ).mem( regRbp, x._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_ParamI, jit_Operand_Kind_ConstI ):
      // mov [rbp + offset + FPO], immediate32
      emit( Instr( 0xc7 ).opc( 0 ).mem( regRbp, x._value + FPO ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_ParamI, jit_Operand_Kind_RegI ):
      // mov [rbp + offset + FPO], reg32
      emit( Instr( 0x89 ).reg( y._reg ).mem( regRbp, x._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_ParamP, jit_Operand_Kind_ConstI ):
      // mov [rbp + offset + FPO], immediate32 sign-extended to 64 bits
      emit( Instr( 0xc7 ).w().opc( 0 ).mem( regRbp, x._value + FPO ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_ParamP, jit_Operand_Kind_RegP ):
      emit( Instr( 0x89 ).w().reg( y._reg ).mem( regRbp, x._value + FPO ) );
      break;

    // Note, Kind_Actual* only appear on the left side of emitMov,
    // and not in any other operations.
    // They are used to assign to actual space, when setting up a call.

    case KindPair( jit_Operand_Kind_ActualB, jit_Operand_Kind_ConstI ):
      // mov [rsp + offset], immediate8
      emit( Instr( 0xc6 ).opc( 0 ).mem( regRsp, x._value ).imm8( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_ActualB, jit_Operand_Kind_RegB ):
      // mov [rsp + offset], reg8
      emit( Instr( 0x88 ).reg8( y._reg ).mem( regRsp, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_ActualI, jit_Operand_Kind_ConstI ):
      // mov [rsp + offset], immediate32
      emit( Instr( 0xc7 ).opc( 0 ).mem( regRsp, x._value ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_ActualI, jit_Operand_Kind_RegI ):
      // mov [rsp + offset], reg32
      emit( Instr( 0x89 ).reg( y._reg ).mem( regRsp, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_ActualP, jit_Operand_Kind_ConstI ):
      // mov [rsp + offset], immediate32 sign-extended to 64 bits
      emit( Instr( 0xc7 ).w().opc( 0 ).mem( regRsp, x._value ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_ActualP, jit_Operand_Kind_RegP ):
      // mov [rsp + offset], reg64
      emit( Instr( 0x89 ).w().reg( y._reg ).mem( regRsp, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_GlobalB ):
      // mov reg8, [rip + offsetToGlobal]
      emit( Instr( 0x8a ).reg8( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_LocalB ):
      // mov x_reg8, [rbp + y._value]
      emit( Instr( 0x8a ).reg8( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ParamB ):
      // mov x_reg8, [rbp + y._value + FPO]
      emit( Instr( 0x8a ).reg8( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ActualB ):
      // mov x_reg8, [rsp + y._value]
      emit( Instr( 0x8a ).reg8( x._reg ).mem( regRsp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_ConstI ):
      // mov reg8, immediate8
      // TO DO: confirm that this works correctly for all registers to r15,
      //        and doesn't get confused by encodings for ah bh ch dh etc.
      // Note, I can't use Instr() here because this may need to set REX.B
      // and Instr() doesn't know how to do that without a base reg.
      // I can't create an Instr() with a base reg, because that would
      // lead to a ModR/M byte.
      emitRex( false, nullptr, x._reg, false, true );
      outB( 0xb0 | regIdLowBits( x._reg->_nativeId ) );
      outB( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_RegB ):
      // mov x_reg8, y_reg8
      emit( Instr( 0x8a ).reg8( x._reg ).rmReg8( y._reg ) );
      break;

    case KindPair( jit_Operand_Kind_RegB, jit_Operand_Kind_RegP_DerefB ):
      // mov x_reg8, [y_reg64 + offset]
      emit( Instr( 0x8a ).reg8( x._reg ).mem( y._reg, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_GlobalI ):
      // mov x_reg32, [rip + offsetToGlobal]
      emit( Instr( 0x8b ).reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_LocalI ):
      // mov x_reg32, [rbp + y._value]
      emit( Instr( 0x8b ).reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ParamI ):
      // mov x_reg32, [rbp + y._value + FPO]
      emit( Instr( 0x8b ).reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ActualI ):
      // mov x_reg32, [rsp + y._value]
      emit( Instr( 0x8b ).reg( x._reg ).mem( regRsp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_ConstI ):
      // mov x_reg32, immediate32
      // As noted above, can't use Instr() for this instruction yet,
      // due to need for REG.B without ModR/M.
      emitRex( false, nullptr, x._reg );
      outB( 0xb8 | regIdLowBits( x._reg->_nativeId ) );
      outI( y._value );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegI ):
      // mov x_reg32, y_reg32
      emit( Instr( 0x8b ).reg( x._reg ).rmReg( y._reg ) );
      break;

    case KindPair( jit_Operand_Kind_RegI, jit_Operand_Kind_RegP_DerefI ):
      // mov x_reg32, [y_reg64 + offset]
      emit( Instr( 0x8b ).reg( x._reg ).mem( y._reg, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalI ):
      // movsxd x_reg64, [rip + offsetToGlobal] 32 bits sign-extended to 64 bits
      emit( Instr( 0x63 ).w().reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_GlobalP ):
      // mov x_reg64, [rip + offsetToGlobal]
      emit( Instr( 0x8b ).w().reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalI ):
      // movsxd x_reg64, [rbp + y._value] 32 bits sign-extended to 64 bits
      emit( Instr( 0x63 ).w().reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_LocalP ):
      // mov x_reg, [rbp + y._value]
      emit( Instr( 0x8b ).w().reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamI ):
      // movsxd x_reg64, [rbp + y._value + FPO] 32 bits sign-extended to 64 bits
      emit( Instr( 0x63 ).w().reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ParamP ):
      // mov reg_x, [rbp + y._value + FPO]
      emit( Instr( 0x8b ).w().reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ActualP ):
      // mov x_reg, [rsp + y._value]
      emit( Instr( 0x8b ).w().reg( x._reg ).mem( regRsp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Global ):
      // lea reg, [rip + offsetToGlobal]
      emit( Instr( 0x8d ).w().reg( x._reg ).memRipRelative( &data[y._value] ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Local ):
      // lea reg64, [rbp+offset]
      emit( Instr( 0x8d ).w().reg( x._reg ).mem( regRbp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Param ):
      // lea reg64, [rbp + y._value + FPO]
      emit( Instr( 0x8d ).w().reg( x._reg ).mem( regRbp, y._value + FPO ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Actual ):
      // lea reg64, [rsp+offset]
      emit( Instr( 0x8d ).w().reg( x._reg ).mem( regRsp, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_Addr_Reg_Offset ):
      // lea x_reg64, [y_reg+offset]
      emit( Instr( 0x8d ).w().reg( x._reg ).mem( y._reg, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_ConstI ):
      // mov reg64, immediate64 (which I sign-extend from the given 32-bit constant).
      // NOTE: this instruction really does take immediate64 rather than simply
      // sign-extending immediate32.
      // Note: Instr() api doesn't support this yet.  Needs REX.B and outL.
      emitRex( true, nullptr, x._reg );
      outB( 0xb8 | regIdLowBits( x._reg->_nativeId ) );
      outL( (long) y._value );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegI ):
      // movsxd x_reg64, y_reg32 sign-extended to 64 bits
      emit( Instr( 0x63 ).w().reg( x._reg ).rmReg( y._reg ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP ):
      // mov x_reg64, y_reg64
      emit( Instr( 0x8b ).w().reg( x._reg ).rmReg( y._reg ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP_DerefI ):
      // movsxd x_reg64, [y_reg64 + offset] 32 bits sign-extended to 64 bits
      emit( Instr( 0x63 ).w().reg( x._reg ).mem( y._reg, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP, jit_Operand_Kind_RegP_DerefP ):
      // mov x_reg64, [y_reg64 + offset]
      emit( Instr( 0x8b ).w().reg( x._reg ).mem( y._reg, y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefB, jit_Operand_Kind_ConstI ):
      // mov [reg64+offset], immediate8
      emit( Instr( 0xc6 ).opc( 0 ).mem( x._reg, x._value ).imm8( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefB, jit_Operand_Kind_RegB ):
      // mov [x_reg64 + offset], y_reg8
      emit( Instr( 0x88 ).reg8( y._reg ).mem( x._reg, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefI, jit_Operand_Kind_ConstI ):
      // mov [reg64+offset], immediate32
      emit( Instr( 0xc7 ).opc( 0 ).mem( x._reg, x._value ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefI, jit_Operand_Kind_RegI ):
      // mov [x_reg64 + offset], y_reg32
      emit( Instr( 0x89 ).reg( y._reg ).mem( x._reg, x._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefP, jit_Operand_Kind_ConstI ):
      // mov [reg64+offset], immediate32 sign-extended to 64 bits
      emit( Instr( 0xc7 ).w().opc( 0 ).mem( x._reg, x._value ).imm32( y._value ) );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefP, jit_Operand_Kind_RegI ):
      // I can't mov into memory with sign extension in one instruction,
      // because movsxd only moves into register.
      // But I can do it in two steps, moving the register to itself with extension first.
      // On the other hand, maybe I don't need this (same goes for the VarP, RegI cases).
      toDo( "emitMov: moving memoryP, regI with sign extension\n" );
      break;

    case KindPair( jit_Operand_Kind_RegP_DerefP, jit_Operand_Kind_RegP ):
      // mov [x_reg64 + offset], y_reg64
      emit( Instr( 0x89 ).w().reg( y._reg ).mem( x._reg, x._value ) );
      break;

    default:
      fatal( "emitMov: unexpected operands\n" );
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



// ----------------------------------------------------------------------------
// Runtime library methods.
// These can be called by the generated code.
// ----------------------------------------------------------------------------

void
runlibWriteI( int val )
{
  printf( "%d", val );
}

// TO DO: argument type byte or int?
void
runlibWriteBool( bool val )
{
  printf( val ? "TRUE" : "FALSE" );
}

void
runlibWriteStr( char* ptr )
{
  printf( "%s", ptr );
}

void
runlibWriteP( char* ptr )
{
  printf( " <%p>", ptr );
}

void
runlibWriteCR()
{
  printf( "\n" );
}


// ----------------------------------------------
// My mini graphics api, with SDL2 under the hood
// ----------------------------------------------

bool grInitialized = false;
SDL_Window* grWindow = nullptr;
SDL_Renderer* grRenderer = nullptr;
SDL_Texture* grBuffer = nullptr;  // this is what program will draw into
uint32_t* grPixels = nullptr;   // or actually this is
bool grPixelsPending = false;

int grBufferX = 320;
int grBufferY = 240;
int grScale = 4;  // scaling up the pixels, from buffer to screen
int grWindowX = grBufferX * grScale;
int grWindowY = grBufferY * grScale;


void
grInit()
{
  if ( grInitialized ) {
    // maybe should clear screen
    return;
  }

  SDL_Init( SDL_INIT_VIDEO );
  grWindow = SDL_CreateWindow( "Pascal",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      grWindowX, grWindowY, 0 );
  grRenderer = SDL_CreateRenderer( grWindow, -1, 0 );
  grBuffer = SDL_CreateTexture( grRenderer,
      SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
      grBufferX, grBufferY );
  grPixels = new uint32_t[ grBufferX * grBufferY ];
  memset( grPixels, 0, grBufferX * grBufferY * sizeof( uint32_t ) );
  grPixelsPending = true;
  grInitialized = true;
}

void
grLazyInit()
{
  if ( !grInitialized ) {
    grInit();
  }
}

void
grTerm()
{
  if ( grInitialized ) {
    // wait until user closes window

    bool quit = false;
    SDL_Event event;

    while ( !quit ) {
      runlibUpdateScreen();

      SDL_WaitEvent( &event );
      switch( event.type ) {
        case SDL_QUIT:
          quit = true;
          break;
        default:
          break;
      }
    }
  }
  delete[] grPixels;
  SDL_DestroyTexture( grBuffer );
  SDL_DestroyRenderer( grRenderer );
  SDL_DestroyWindow( grWindow );
  SDL_Quit();
}

void
runlibClearScreen()
{
  grLazyInit();
  memset( grPixels, 0, grBufferX * grBufferY * sizeof( uint32_t ) );
  grPixelsPending = true;
}

void
runlibUpdateScreen()
{
  if ( grPixelsPending ) {
    SDL_UpdateTexture( grBuffer, nullptr, grPixels, grBufferX * sizeof( uint32_t ) );
    // fill the renderer with the current drawing color.  probably I don't need.
    // SDL_RenderClear( grRenderer );

    // copy entire grBuffer to grRenderer, scaling to fit.
    // srcRect == null indicates entire texture.
    // destRect == null indicates entire rendering target. 
    SDL_Rect* srcRect = nullptr;
    SDL_Rect* destRect = nullptr;
    SDL_RenderCopy( grRenderer, grBuffer, srcRect, destRect );
    // show the renderer's updates on the screen
    SDL_RenderPresent( grRenderer );

    grPixelsPending = false;
  }
}

void
runlibSetPixel( int x, int y, int color )
{
  grLazyInit();
  grPixels[ x + y * grBufferX ] = color;
  grPixelsPending = true;
}

int
runlibGetPixel( int x, int y )
{
  grLazyInit();
  return grPixels[ x + y * grBufferX ];
}

void
runlibDelay( int milliseconds )
{
  // Do this under the hood, in case there's anything new to show
  runlibUpdateScreen();
  SDL_Delay( milliseconds );
}

// Waits for a key to be pressed.
// Returns the SDL_Keycode of the key that was hit.
// (That's a slightly more generic form of scancode.)
//
// Note, the SDL graphical window needs to be in focus,
// not the terminal window.  I'm not sure if SDL keyboard events
// can be used without a graphical window.
//
int
runlibWaitKey()
{
  // uses SDL, so need at least SDL_Init
  grLazyInit();
  // Also before we wait, bring the screen up-to-date, similar to runlibDelay().
  runlibUpdateScreen();

  SDL_Event event;
  while ( true ) {
    SDL_WaitEvent( &event );

    if ( event.type == SDL_KEYDOWN ) {
      // Maybe: ignore if event.key.repeat != 0 because that's a repeat
      // "Note: if you are looking for translated character input,
      //   see the SDL_TEXTINPUT event."
      // SDL_Scancode = event.key.keysym.scancode;
      SDL_Keycode sym = event.key.keysym.sym; // SDL virtual key code
      return (int) sym;
    }
  }
}


// ----------------------------------------------------------------------------


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


