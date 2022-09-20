/*

  jit.c

  This converts the intermediate t-code produced by my Pascal compiler
  into x86 machine code, and executes it.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
//  -- for mmap, mprotect
#include <errno.h>

// This is where we get the definitions for tCode,
// which originate in SSL.
#include "pascal.h"

typedef void (*funcptr)();


void parseArgs( int argc, char* argv[] );
void usage( int status );
void loadTCode();
void allocNativeCode();
void protectNativeCode();
void outB( char c );
void outI( int i );
void generateCode();
void tCodeNotImplemented( int opcode );
void executeCode();


char* filename = 0;

int* tCode = 0;
int  tCodeLen = 0;


// Runtime data memory, for globals.
// Right now we don't actually know how much we need!
//
// The call stack will not live in here.  It will be part of the
// same call stack used by this calling program.
//
#define dataSize 100000
char data[dataSize];



char* nativeCode = 0;
size_t nativeCodeLen = 10000;
char* nativePc = 0;


// Operand stack
//
typedef enum {
  jit_Operand_Kind_Illegal = 0,

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

  jit_Operand_Kind_ConstI,

  // operand is the address of a variable
  jit_Operand_Kind_Addr_Global,
  jit_Operand_Kind_Addr_Local,
  jit_Operand_Kind_Addr_Param,
  jit_Operand_Kind_Addr_Actual,

  // operand is a value in a register.
  // We either have one byte valid, 4 bytes valid, or 8 bytes valid.
  // In the case of 4 bytes, we are guaranteed that the higher 4 bytes
  // of the register are 0 (note: not sign extended), thanks to x86-64 rules.
  // This is not the case if the valid size is one byte.
  jit_Operand_Kind_RegB,
  jit_Operand_Kind_RegI,
  jit_Operand_Kind_RegP

} jitOperandKind;


typedef struct {
  jitOperandKind kind;
  int value; // for a var: the var offset
             // for a const: the const value
  // ... reg ptr ... or should I use value for a reg id too?
} operandT;

#define operandStackSize 100
operandT operandStack[operandStackSize];


int
main( int argc, char* argv[] )
{
  parseArgs( argc, argv );
  loadTCode();
  allocNativeCode();
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

void
generateCode()
{
  int jitSp = -1;  // points to top entry

  int* tCodePc = tCode;
  int* tCodeEnd = tCode + tCodeLen;

  for ( tCodePc = tCode; tCodePc < tCodeEnd; ) {
    switch ( *tCodePc++ ) {

      case tPushGlobalI :
        tCodeNotImplemented( tCodePc[-1] );
        tCodePc++;
        break;
      case tPushGlobalB :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushGlobalP :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushLocalI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushLocalB :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushLocalP :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushParamI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushParamB :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushParamP :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushConstI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushAddrGlobal :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushAddrLocal :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushAddrParam :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tPushAddrActual :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tFetchI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tFetchB :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tFetchP :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tAssignI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tAssignB :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tAssignP :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tCopy :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tIncI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tDecI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tMultI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tDivI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tAddI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tSubI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tNegI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tNot :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tAnd :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tOr :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tEqualI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tNotEqualI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tGreaterI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tLessI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tGreaterEqualI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tLessEqualI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tEqualP :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tNotEqualP :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tAllocActuals :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tFreeActuals :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tCall :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tReturn :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tEnter :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tJump :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tJumpTrue :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tJumpFalse :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        tCodePc++;
        break;
      case tWriteI :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tWriteBool :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tWriteStr :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tWriteP :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;
      case tWriteCR :
        tCodeNotImplemented( tCodePc[-1] );
        // TO DO
        break;

      default:
        --tCodePc;
        printf( "Error: bad instruction %d\n", *tCodePc );
        exit( 1 );
    }
  }
}



// ----------------------------------------------------------------------------

void
loadTCode()
{
  FILE* src = fopen( filename, "r" );
  if ( !src ) {
    printf( "Error: can't open intermediate file %s\n", filename );
    exit( 1 );
  }
  int read = fscanf( src, "%d", &tCodeLen );
  assert( read == 1 );
  tCode = (int*) malloc( tCodeLen * sizeof( int ) );
  assert( tCode != 0 );
  for ( int i = 0; i < tCodeLen; ++i ) {
    read = fscanf( src, "%d", &tCode[i] );
    assert( read == 1 );
  }

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
  nativeCode = (char*) mmap( NULL, nativeCodeLen, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
  nativePc = nativeCode;
  if ( nativeCode == NULL ) {
    printf( "error: mmap failed to allocate %ld bytes for native code\n", nativeCodeLen );
    exit( 1 );
  }
}


void
protectNativeCode()
{
  // Switch memory from writable to executable.
  // This also ensures the instruction cache doesn't have stale data.
  if ( mprotect( nativeCode, nativeCodeLen, PROT_READ | PROT_EXEC ) != 0 ) {
    printf( "error: mprotect failed with status %d\n", errno );
    exit( 1 );
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


void
outI( int i )
{
  int* p = (int*) nativePc;
  *(p++) = i;
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
   outB( 0x8b );  // mov ebp, esp   -- Need to switch to rsp, rbp.  Getting SEGV on this.
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


