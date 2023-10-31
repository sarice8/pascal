/*

  StackMachine.c

               This executes the intermediate t-code generated by
               my Pascal compiler.

  HISTORY
    21Sep89    1st version, runs test.pas
    24Sep89    Pointer instructions
    28Sep89    -dump will dump a readable t-code file
    01Jan90    read string literals into data segment

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef AMIGA
#include <dos.h>
#endif // AMIGA


#include <unordered_map>
#include <string>
#include <vector>
#include <cstring>


#include "pascal.h"
#include "tcode.h"
#include "runlib.h"

FILE *src;

#define PTR_SIZE (sizeof(void*))

#define codeMax 10000
int code[codeMax],pc;      /* Program memory */
int codeWords;             /* Actual #words used */

// expression stack - used for expression evaluation.
//  In a JIT, this might be implemented with a mix of registers and temp vars instead.
//  Each element is large enough to hold a pointer value.
#define stackMax 2000
long stack[stackMax],sp;


// Hardcoded offset from fp to start of params.
// This is needed to record old fp and method return address.
// TO DO: maybe the offset should be built into code instead.
#define FRAME_PARAMS_OFFSET  (2*PTR_SIZE)

// Hardcoded offset from fp to static link pointer,
// in a nested method.  This points to the frame of the enclosing scope.
// In my calling convention, it is actually the hidden first parameter
// to the method.  So it sits at the start of the params.
#define FRAME_STATIC_LINK_OFFSET  FRAME_PARAMS_OFFSET


// good grief, can't hardcode these
#define dataSize 5000000
char data[dataSize];         /* Data memory */


// call stack - used for function params, local vars, temp vars
// (and TO DO: return addresses)
//
// The call stack resides within the data[] array (at the end, growing down).
// This is because in my runtime model, an 'address' is always an index into data.
// e.g. so tAssignI can work equally on global var and local var.
// 
#define callStackSize 200000
#define callStackLowerBound (&data[dataSize - callStackSize])
char* call_sp;
char* call_fp;

// Helper: read a method parameter from the tcode call stack.
template<typename T>
T
getParam( int offset )
{
  return *(T*)( call_sp + offset );       
}



short temp;
char trace, underflow, dump;


// Forward declarations
void walkTable();
void dumpTable();
void hitBreak();
void fatal( const char* msg );
void cleanup();

void defineLabels();
void defineLabel( int label, int addr );
void defineLabelAlias( int label, int aliasToLabel );
void defineLabelExtern( int label, char* name );

void callCdecl( const std::string& name );

std::unordered_map<int, int> labels;   // label# -> tcode addr
std::unordered_map<int, int> labelAliases;  // label# -> aliasToLabel#
std::unordered_map<int, std::string> externLabels;  // label# -> name of extern method


int
main( int argc, char* argv[] )
{
  trace = 0;

  /* Prepare Files */

  int arg = 1;
  if (arg>=argc) {
    printf("Usage:  stackmachine -trace -underflow -dump <file>\n");
    exit(-1);
  }
  while (argv[arg][0]=='-') {
    if (argv[arg][1]=='t' || argv[arg][1]=='T')
      trace = 1;
    else if (argv[arg][1]=='u' || argv[arg][1]=='U')
      underflow = 1;
    else if (argv[arg][1]=='d' || argv[arg][1]=='D')
      dump = 1;
    arg++;
  }
  if((src=fopen(argv[arg],"r"))==NULL) {
    printf("Can't open program file %s\n",argv[arg]);
    exit(-1);
  }
  int read = fscanf(src, "%d", &codeWords);
  assert( read == 1 );
  if (codeWords >= codeMax) {
    printf("Program code too big for buffer\n");
    exit(-1);
  }
  printf("Stack Machine ");

  int temp;
  for (pc=0; pc<codeWords; pc++) {
    read = fscanf(src, "%d", &temp);
    assert( read == 1 );
    code[pc] = temp;
  } 

  /* Also get any string literals, and store in data segemnt */
  /* File format: <addr> <#ints> <data ints>               */

  int address;
  int count;
  while (fscanf(src, "%d", &address) != EOF) {
    read = fscanf(src, "%d", &count);
    int* intPtr = (int*) &(data[address]);
    while (count--) {
      int word;
      read = fscanf(src, "%d", &word);
      assert( read == 1 );
      *intPtr++ = word;
    }
  }

  fclose(src);
  printf("- program loaded (%d words)\n",codeWords);

#ifdef AMIGA
  onbreak(&hitBreak);
#endif // AMIGA

  if (dump) {
    dumpTable();
    exit(0);
  }

  /* Scan through the code to find the labels */
  defineLabels();

  /* Execute code */

  pc = 0;                        /* Initialize walker */
  sp = 0;
  call_sp = &data[dataSize] - PTR_SIZE;

  // Emulate an initial tCall, with return address 0.
  // When we execute the tReturn to 0, walkTable will exit.
  // NOTE tCall currently pushes return addr on stack rather than callStack
  stack[sp] = 0;

  walkTable();

  printf(".Done\n");
  cleanup();
}


void
defineLabels()
{
  for ( pc = 0; pc < codeWords; ) {
    switch ( code[pc] ) {
      case tLabel:
        defineLabel( code[pc+1], pc+2 );
        break;
      case tLabelAlias:
        defineLabelAlias( code[pc+1], code[pc+2] );
        break;
      case tLabelExtern:
        defineLabelExtern( code[pc+1], &data[code[pc+2]] );
        break;
      default:
        break;
    }
    pc += tcodeInstrSize( code[pc] );
  }
}


void
defineLabel( int label, int addr )
{
  labels[ label ] = addr;
}

void
defineLabelAlias( int label, int aliasToLabel )
{
  labelAliases[ label ] = aliasToLabel;
}

void
defineLabelExtern( int label, char* name )
{
  externLabels[label] = std::string( name );
}


int
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
  fatal( "undefined label\n" );
  return 0;
}


// Returns pointer to the static scope <uplevels> higher (0 is the current scope).
// This only works in a nested method, unless uplevels is 0.
//
char*
upStaticFrame( int uplevels )
{
  char* up_fp = call_fp;
  for ( int i = 0; i < uplevels; ++i ) {
    up_fp = *(char**)(up_fp + FRAME_STATIC_LINK_OFFSET);
  }
  return up_fp;
}


// Cdecl calls have to translate between the tcode calling convention
// and the cdecl calling convention.
// For example, tcode implements function return values by passing an
// extra parameter that's a pointer to a temporary, in which the called method
// is expected to place the return value.  We have to do that storing here.
//
void
callCdecl( const std::string& name )
{
  // For now, need to hardcode calls to the available cdecl methods.

  if ( name == "runlibShortStrCmp" ) {
    int* resultPtr = getParam<int*>( 16 );
    *resultPtr = runlibShortStrCmp( *(const char**)call_sp, *(const char**)(call_sp+8) );
  } else if ( name == "runlibMalloc" ) {
    void** resultPtr = getParam<void**>( 4 );
    *resultPtr = runlibMalloc( getParam<int>( 0 ) );
  } else if ( name == "runlibRealloc" ) {
    void** resultPtr = getParam<void**>( 12 );  // Note params are not aligned today
    *resultPtr = runlibRealloc( getParam<void*>( 0 ), getParam<int>( 8 ) );
  } else if ( name == "runlibFree" ) {
    runlibFree( getParam<void*>( 0 ) );
  } else if ( name == "runlibClearScreen" ) {
    runlibClearScreen();
  } else if ( name == "runlibUpdateScreen" ) {
    runlibUpdateScreen();
  } else if ( name == "runlibSetPixel" ) {
    runlibSetPixel( getParam<int>( 0 ), getParam<int>( 4 ), getParam<int>( 8 ) );
  } else if ( name == "runlibGetPixel" ) {
    int* resultPtr = getParam<int*>( 8 );
    *resultPtr = runlibGetPixel( getParam<int>( 0 ), getParam<int>( 4 ) );
  } else if ( name == "runlibDelay" ) {
    runlibDelay( getParam<int>( 0 ) );
  } else if ( name == "runlibWaitKey" ) {
    int* resultPtr = getParam<int*>( 0 );
    *resultPtr = runlibWaitKey();
  } else if ( name == "runlibArctan" ) {
    double* resultPtr = getParam<double*>( 8 );
    *resultPtr = runlibArctan( getParam<double>( 0 ) );
  } else if ( name == "runlibCos" ) {
    double* resultPtr = getParam<double*>( 8 );
    *resultPtr = runlibCos( getParam<double>( 0 ) );
  } else if ( name == "runlibExp" ) {
    double* resultPtr = getParam<double*>( 8 );
    *resultPtr = runlibExp( getParam<double>( 0 ) );
  } else if ( name == "runlibLn" ) {
    double* resultPtr = getParam<double*>( 8 );
    *resultPtr = runlibLn( getParam<double>( 0 ) );
  } else if ( name == "runlibRound" ) {
    int* resultPtr = getParam<int*>( 8 );
    *resultPtr = runlibRound( getParam<double>( 0 ) );
  } else if ( name == "runlibSin" ) {
    double* resultPtr = getParam<double*>( 8 );
    *resultPtr = runlibSin( getParam<double>( 0 ) );
  } else if ( name == "runlibSqrt" ) {
    double* resultPtr = getParam<double*>( 8 );
    *resultPtr = runlibSqrt( getParam<double>( 0 ) );
  } else if ( name == "runlibTrunc" ) {
    int* resultPtr = getParam<int*>( 8 );
    *resultPtr = runlibTrunc( getParam<double>( 0 ) );
  } else if ( runlibLookupMethod( name.c_str() ) != nullptr ) {
    printf( "SM callCdecl() needs to be taught how to call %s\n", name.c_str() );
  } else {
    printf( "Unknown external method %s\n", name.c_str() );
  }
}


void
walkTable()
{

   while (1) {

     if (trace) printf("[%d]",pc);
     if (underflow)
       if (sp<0) fatal("+++Stack Underflow+++");

     switch (code[pc++]) {

       case tPushGlobalI :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = *(int*) (data + code[pc++]);
              continue;
       case tPushGlobalB :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = *(char*) (data + code[pc++]);
              continue;
       case tPushGlobalP :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = (long) *(void**) (data + code[pc++]);
              continue;
       case tPushGlobalD :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = (long) *(void**) (data + code[pc++]);
              continue;
       case tPushLocalI :
              // code provides negative offset from call_fp
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = *(int*) (call_fp + code[pc++]);
              continue;
       case tPushLocalB :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = *(char*) (call_fp + code[pc++]);
              continue;
       case tPushLocalP :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = (long) *(void**) (call_fp + code[pc++]);
              continue;
       case tPushLocalD :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = (long) *(void**) (call_fp + code[pc++]);
              continue;
       case tPushParamI :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = *(int*) (call_fp + code[pc++] + FRAME_PARAMS_OFFSET);
              continue;
       case tPushParamB :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = *(char*) (call_fp + code[pc++] + FRAME_PARAMS_OFFSET);
              continue;
       case tPushParamP :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = (long) *(void**) (call_fp + code[pc++] + FRAME_PARAMS_OFFSET);
              continue;
       case tPushParamD :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = (long) *(void**) (call_fp + code[pc++] + FRAME_PARAMS_OFFSET);
              continue;
       case tPushUpLocalI : {
              if (++sp>=stackMax) fatal("stack overflow");
              int uplevels = code[pc++];
              int offset = code[pc++];
              stack[sp] = *(int*) (upStaticFrame( uplevels ) + offset);
              continue;
              }
       case tPushUpLocalB : {
              if (++sp>=stackMax) fatal("stack overflow");
              int uplevels = code[pc++];
              int offset = code[pc++];
              stack[sp] = *(char*) (upStaticFrame( uplevels ) + offset);
              continue;
              }
       case tPushUpLocalP : {
              if (++sp>=stackMax) fatal("stack overflow");
              int uplevels = code[pc++];
              int offset = code[pc++];
              stack[sp] = (long) *(void**) (upStaticFrame( uplevels ) + offset);
              continue;
              }
       case tPushUpLocalD : {
              if (++sp>=stackMax) fatal("stack overflow");
              int uplevels = code[pc++];
              int offset = code[pc++];
              stack[sp] = (long) *(void**) (upStaticFrame( uplevels ) + offset);
              continue;
              }
       case tPushConstI :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = code[pc++];
              continue;
       case tPushConstD : {
              if (++sp>=stackMax) fatal("stack overflow");
              // The value is actually a double, but we can push it as long since also 8 bytes
              long val = *(uint32_t*) &code[pc];
              val |= ( (long) *(uint32_t*) &code[pc+1] ) << 32;
              stack[sp] = val;
              pc += 2;
              continue;
              }
       case tPushAddrGlobal :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = (long) (data + code[pc++]);
              continue;
       case tPushAddrLocal :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = (long) (call_fp + code[pc++]);
              continue;
       case tPushAddrParam :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = (long) (call_fp + code[pc++] + FRAME_PARAMS_OFFSET);
              continue;
       case tPushAddrActual :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = (long) (call_sp + code[pc++]);
              continue;
       case tPushAddrUpLocal : {
              if (++sp>=stackMax) fatal("stack overflow");
              int uplevels = code[pc++];
              int offset = code[pc++];
              stack[sp] = (long) (upStaticFrame( uplevels ) + offset);
              continue;
              }
       case tPushAddrUpParam : {
              if (++sp>=stackMax) fatal("stack overflow");
              int uplevels = code[pc++];
              int offset = code[pc++];
              stack[sp] = (long) ( upStaticFrame( uplevels ) + offset + FRAME_PARAMS_OFFSET );
              continue;
              }
       case tSwap : {
              long temp = stack[sp];
              stack[sp] = stack[sp-1];
              stack[sp-1] = temp;
              continue;
              }
       case tFetchI :
              stack[sp] = *(int*) stack[sp];
              continue;
       case tFetchB :
              stack[sp] = *(char*) stack[sp];
              continue;
       case tFetchP :
              stack[sp] = (long) *(void**) stack[sp];
              continue;
       case tFetchD :
              stack[sp] = (long) *(void**) stack[sp];
              continue;
       case tAssignI :
              *((int*) stack[sp-1]) = stack[sp];
              sp -= 2;
              continue;
       case tAssignB :
              *((char*) stack[sp-1]) = stack[sp];
              sp -= 2;
              continue;
       case tAssignP :
              *((void**) stack[sp-1]) = (void*) stack[sp];
              sp -= 2;
              continue;
       case tAssignD :
              *((double*) stack[sp-1]) = *(double*) &stack[sp];
              sp -= 2;
              continue;
       case tCopy : {
              char* ptr1 = (char*) stack[sp-1];
              char* ptr2 = (char*) stack[sp];
              for (int i = 0; i < code[pc]; ++i) {
                *ptr1++ = *ptr2++;
              }
              sp -= 2;
              pc++;
              continue;
              }
       case tCastBtoI :
              // zero-extends from uint8_t to int32_t
              stack[sp] = (int32_t) (uint8_t) stack[sp];
              continue;
       case tCastItoB :
              // truncates from int32_t to uint8_t
              stack[sp] = (uint8_t) stack[sp];
              continue;
       case tCastItoD : {
              // converts from int32_t to double
              // (There is not yet a reverse conversion; we need separate trunc and round)
              double value = (int32_t) stack[sp];
              // double values are encoded on the stack as long
              stack[sp] = *(long*) &value;
              continue;
              }
       case tIncI :
              stack[sp]++;
              continue;
       case tDecI :
              stack[sp]--;
              continue;
       case tMultI :
              stack[sp-1] *= stack[sp];
              sp--;
              continue;
       case tDivI :
              stack[sp-1] /= stack[sp];
              sp--;
              continue;
       case tAddPI :
              stack[sp-1] += stack[sp];
              sp--;
              continue;
       case tAddI :
              stack[sp-1] += stack[sp];
              sp--;
              continue;
       case tSubP :
              stack[sp-1] -= stack[sp];
              sp--;
              continue;
       case tSubPI :
              stack[sp-1] -= stack[sp];
              sp--;
              continue;
       case tSubI :
              stack[sp-1] -= stack[sp];
              sp--;
              continue;
       case tNegI :
              stack[sp] *= -1;
              continue;
       case tMultD : {
                // TO DO: consider a union for stack value, or other simpler access method
                double x = *(double*) &stack[sp-1];
                double y = *(double*) &stack[sp];
                double result = x * y;
                stack[sp-1] = *(long*) &result;
                sp--;
              }
              continue;
       case tDivD : {
                double x = *(double*) &stack[sp-1];
                double y = *(double*) &stack[sp];
                double result = x / y;
                stack[sp-1] = *(long*) &result;
                sp--;
              }
              continue;
       case tAddD : {
                double x = *(double*) &stack[sp-1];
                double y = *(double*) &stack[sp];
                double result = x + y;
                stack[sp-1] = *(long*) &result;
                sp--;
              }
              continue;
       case tSubD : {
                double x = *(double*) &stack[sp-1];
                double y = *(double*) &stack[sp];
                double result = x - y;
                stack[sp-1] = *(long*) &result;
                sp--;
              }
              continue;
       case tNegD : {
                double x = *(double*) &stack[sp];
                double result = -x;
                stack[sp] = *(long*) &result;
              }
              continue;
       case tNot :
              stack[sp] = !stack[sp];
              continue;

       case tEqualB :
              stack[sp-1] = stack[sp-1] == stack[sp];
              sp--;
              continue;
       case tNotEqualB :
              stack[sp-1] = stack[sp-1] != stack[sp];
              sp--;
              continue;
       case tGreaterB :
              stack[sp-1] = uint8_t(stack[sp-1]) > uint8_t(stack[sp]);
              sp--;
              continue;
       case tLessB :
              stack[sp-1] = uint8_t(stack[sp-1]) < uint8_t(stack[sp]);
              sp--;
              continue;
       case tGreaterEqualB :
              stack[sp-1] = uint8_t(stack[sp-1]) >= uint8_t(stack[sp]);
              sp--;
              continue;
       case tLessEqualB :
              stack[sp-1] = uint8_t(stack[sp-1]) <= uint8_t(stack[sp]);
              sp--;
              continue;

       case tEqualI :
              stack[sp-1] = stack[sp-1] == stack[sp];
              sp--;
              continue;
       case tNotEqualI :
              stack[sp-1] = stack[sp-1] != stack[sp];
              sp--;
              continue;
       case tGreaterI :
              stack[sp-1] = stack[sp-1] > stack[sp];
              sp--;
              continue;
       case tLessI :
              stack[sp-1] = stack[sp-1] < stack[sp];
              sp--;
              continue;
       case tGreaterEqualI :
              stack[sp-1] = stack[sp-1] >= stack[sp];
              sp--;
              continue;
       case tLessEqualI :
              stack[sp-1] = stack[sp-1] <= stack[sp];
              sp--;
              continue;

       case tEqualP :
              stack[sp-1] = stack[sp-1] == stack[sp];
              sp--;
              continue;
       case tNotEqualP :
              stack[sp-1] = stack[sp-1] != stack[sp];
              sp--;
              continue;
       case tGreaterP :
              stack[sp-1] = uint64_t(stack[sp-1]) > uint64_t(stack[sp]);
              sp--;
              continue;
       case tLessP :
              stack[sp-1] = uint64_t(stack[sp-1]) < uint64_t(stack[sp]);
              sp--;
              continue;
       case tGreaterEqualP :
              stack[sp-1] = uint64_t(stack[sp-1]) >= uint64_t(stack[sp]);
              sp--;
              continue;
       case tLessEqualP :
              stack[sp-1] = uint64_t(stack[sp-1]) <= uint64_t(stack[sp]);
              sp--;
              continue;

       case tEqualD : {
                double x = *(double*) &stack[sp-1];
                double y = *(double*) &stack[sp];
                stack[sp-1] = x == y;
                sp--;
              }
              continue;
       case tNotEqualD : {
                double x = *(double*) &stack[sp-1];
                double y = *(double*) &stack[sp];
                stack[sp-1] = x != y;
                sp--;
              }
              continue;
       case tGreaterD : {
                double x = *(double*) &stack[sp-1];
                double y = *(double*) &stack[sp];
                stack[sp-1] = x > y;
                sp--;
              }
              continue;
       case tLessD : {
                double x = *(double*) &stack[sp-1];
                double y = *(double*) &stack[sp];
                stack[sp-1] = x < y;
                sp--;
              }
              continue;
       case tGreaterEqualD : {
                double x = *(double*) &stack[sp-1];
                double y = *(double*) &stack[sp];
                stack[sp-1] = x >= y;
                sp--;
              }
              continue;
       case tLessEqualD : {
                double x = *(double*) &stack[sp-1];
                double y = *(double*) &stack[sp];
                stack[sp-1] = x <= y;
                sp--;
              }
              continue;

       case tAllocActuals :
              call_sp -= code[pc++];
              if (call_sp < callStackLowerBound) fatal("call stack overflow");
              continue;
       case tAllocActualsCdecl :
              call_sp -= code[pc++];
              if (call_sp < callStackLowerBound) fatal("call stack overflow");
              continue;
       case tFreeActuals :
              call_sp += code[pc++];
              continue;
       case tCall :
              if (++sp==stackMax) fatal("call stack overflow");
              stack[sp] = pc+1;
              pc = findLabel( code[pc] );
              continue;
       case tCallCdecl : {
              int label = code[pc++];
              callCdecl( externLabels[label] );
              }
              continue;
       case tReturn :
              assert( call_fp );
              pc = stack[sp--];
              // unwind tEnter
              call_sp = call_fp;
              call_fp = *(char**)(call_fp);
              call_sp += PTR_SIZE;
              call_sp += PTR_SIZE;;  // TO DO: remove when iCall pushes return address on call stack
              if ( pc == 0 ) {       //
                return;              // done program table (driver pushed return addr 0)
              }
              continue;
       case tEnter :       
              // Start a stack frame, on entry to a proc/func.
              //  TO DO: ideally tCall/tReturn would record return addr on call stack, for clarity
              call_sp -= PTR_SIZE;  // TO DO: remove when iCall pushes return address on call stack
              call_sp -= PTR_SIZE;
              *(char**)call_sp = call_fp;
              call_fp = call_sp;
              if (call_sp < callStackLowerBound) fatal("call stack overflow");
              call_sp -= code[pc++];
              if (call_sp < callStackLowerBound) fatal("call stack overflow");
              continue;
       case tJump :
              pc = findLabel( code[pc] );
              continue;
       case tJumpTrue :
              if (stack[sp--]) {
                pc = findLabel( code[pc] );
                continue;
              }
              pc++;
              continue;
       case tJumpFalse :
              if (!stack[sp--]) {
                pc = findLabel( code[pc] );
                continue;
              }
              pc++;
              continue;
       case tJumpCaseB : {
              // same as tJumpCaseI but with unsigned comparisons
              uint8_t value = stack[sp--];
              int table = findLabel( code[pc] );
              while ( true ) {
                if ( code[table] == tCase ) {
                  if ( value == uint8_t( code[table+1] ) ) {
                    pc = findLabel( code[table+2] );
                    break;
                  }
                  table += 3;
                } else if ( code[table] == tCaseRange ) {
                  if ( value >= uint8_t( code[table+1] ) && value <= uint8_t( code[table+2] ) ) {
                    pc = findLabel( code[table+3] );
                    break;
                  }
                  table += 4;
                } else if ( code[table] == tCaseEnd ) {
                  pc = findLabel( code[table+1] );
                  break;
                } else {
                  fatal( "unexpected instruction in case table\n" );
                }
              }
              continue;
              }
       case tJumpCaseI : {
              int value = stack[sp--];
              int table = findLabel( code[pc] );
              while ( true ) {
                if ( code[table] == tCase ) {
                  if ( value == code[table+1] ) {
                    pc = findLabel( code[table+2] );
                    break;
                  }
                  table += 3;
                } else if ( code[table] == tCaseRange ) {
                  if ( value >= code[table+1] && value <= code[table+2] ) {
                    pc = findLabel( code[table+3] );
                    break;
                  }
                  table += 4;
                } else if ( code[table] == tCaseEnd ) {
                  pc = findLabel( code[table+1] );
                  break;
                } else {
                  fatal( "unexpected instruction in case table\n" );
                }
              }
              continue;
              }
       case tJumpCaseS : {
              const char* valueShortStr = (const char*) stack[sp--];
              int table = findLabel( code[pc] );
              while ( true ) {
                if ( code[table] == tCase ) {
                  const char* tableShortStr = (const char*) ( data + code[table+1] );
                  if ( runlibShortStrCmp( valueShortStr, tableShortStr ) == 0 ) {
                    pc = findLabel( code[table+2] );
                    break;
                  }
                  table += 3;
                } else if ( code[table] == tCaseRange ) {
                  const char* tableLowShortStr = (const char*) ( data + code[table+1] );
                  const char* tableHighShortStr = (const char*) ( data + code[table+2] );
                  if ( runlibShortStrCmp( valueShortStr, tableLowShortStr ) >= 0 &&
                       runlibShortStrCmp( valueShortStr, tableHighShortStr ) <= 0 ) {
                    pc = findLabel( code[table+3] );
                    break;
                  }
                  table += 4;
                } else if ( code[table] == tCaseEnd ) {
                  pc = findLabel( code[table+1] );
                  break;
                } else {
                  fatal( "unexpected instruction in case table\n" );
                }
              }
              continue;
              }
       case tCase:
              // A no-op if we fall through to this case table
              pc += 2;
              break;
       case tCaseRange:
              // A no-op if we fall through to this case table
              pc += 3;
              break;
       case tCaseEnd:
              // A no-op if we fall through to this case table
              pc++;
              break;
       case tLabel :
              // Defines a label.  This is a no-op during execution.
              pc++;
              continue;
       case tLabelAlias :
              // Defines a label alias.  This is a no-op during execution.
              pc++;
              pc++;
              continue;
       case tLabelExtern :
              // Defines an extern label.  This is a no-op during execution.
              pc++;
              pc++;
              continue;
       case tWriteI :
              runlibWriteI( (int) stack[sp--] );
              continue;
       case tWriteBool :
              runlibWriteBool( stack[sp--] );
              continue;
       case tWriteChar :
              runlibWriteChar( (char) stack[sp--] );
              continue;
       case tWriteShortStr : {
              const char* shortStr = (const char*) stack[sp--];
              runlibWriteShortStr( shortStr );
              }
              continue;
       case tWritePChar :
              runlibWritePChar( (const char*) stack[sp--] );
              continue;
       case tWriteP :
              runlibWriteP( (const void*) stack[sp--] );
              continue;
       case tWriteEnum: {
              struct nameTableEntryT {
                int value;
                int padding;
                char* name;                
              };
              nameTableEntryT* table = (nameTableEntryT*) stack[sp--];
              int value = (int) stack[sp--];
              int i;
              for ( i = 0; table[i].name != nullptr; ++i ) {
                if ( table[i].value == value ) {
                  break;
                }
              }
              if ( table[i].name != nullptr ) {
                printf( "%s", table[i].name );
              } else {
                printf( "<?badEnum>" );
              }
              }
              continue;
       case tWriteD : {
              double val = *(double*) &stack[sp--];
              runlibWriteD( val );
              }
              continue;
       case tWriteCR :
              runlibWriteCR();
              continue;
       case tReadI :
              runlibReadI( (int*) stack[sp--] );
              continue;
       case tReadChar :
              runlibReadChar( (char*) stack[sp--] );
              continue;
       case tReadShortStr : {
              int capacity = code[pc++];
              runlibReadShortStr( (char*) stack[sp--], capacity );
              }
              continue;
       case tReadCR :
              runlibReadCR();
              continue;
       case tFile :
              // no-op, for debugging
              continue;
       case tLine :
              // no-op, for debugging
              continue;

       default:
              pc--;
              fatal("bad instruction");
     } /* switch */
   }
}



void
dumpTable()
{
  FILE* dmp;
  if((dmp=fopen("a.dmp","w"))==NULL) {
    printf("Can't open dump file a.dmp\n");
    exit(5);
  }

  int dumpedTo = tcodeDump( dmp, code, 0, codeWords );

  if ( dumpedTo != codeWords ) {
    fprintf( dmp, "...additional incomplete instruction\n" );
  }

  fclose( dmp );
}


void
hitBreak()
{
  printf(".Breaking...\n");
  cleanup();
  //return(1);
  exit(1);
}


void
fatal( const char* msg )
{
  printf(".FATAL - %s at %d\n",msg,pc);
  exit(5);
}


void
cleanup()
{
}
