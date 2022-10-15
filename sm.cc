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


#include "pascal.h"

FILE *src,*dmp;

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



// good grief, can't hardcode these
#define dataSize 500000
char data[dataSize];         /* Data memory */


// call stack - used for function params, local vars, temp vars
// (and TO DO: return addresses)
//
// The call stack resides within the data[] array (at the end, growing down).
// This is because in my runtime model, an 'address' is always an index into data.
// e.g. so tAssignI can work equally on global var and local var.
// 
#define callStackSize 2000
#define callStackLowerBound (&data[dataSize - callStackSize])
char* call_sp;
char* call_fp;


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

struct instrInfo_s {
  const char* name;
  int args;
};

struct instrInfo_s tCodeInstrs[] = {
  { "tPushGlobalI", 1 },
  { "tPushGlobalB", 1 },
  { "tPushGlobalP", 1 },
  { "tPushLocalI", 1 },
  { "tPushLocalB", 1 }, 
  { "tPushLocalP", 1 },
  { "tPushParamI", 1 },
  { "tPushParamB", 1 }, 
  { "tPushParamP", 1 },
  { "tPushUpLocalI", 2 },
  { "tPushUpLocalB", 2 }, 
  { "tPushUpLocalP", 2 },
  { "tPushUpParamI", 2 },
  { "tPushUpParamB", 2 }, 
  { "tPushUpParamP", 2 },
  { "tPushConstI", 1 },
  { "tPushAddrGlobal", 1 },
  { "tPushAddrLocal", 1 },
  { "tPushAddrParam", 1 },
  { "tPushAddrActual", 1 },
  { "tPushAddrUpLocal", 2 },
  { "tPushAddrUpParam", 2 },
  { "tFetchI", 0 },
  { "tFetchB", 0 },
  { "tFetchP", 0 },
  { "tAssignI", 0 },
  { "tAssignB", 0 },
  { "tAssignP", 0 },
  { "tCopy", 1 },
  { "tIncI", 0 },
  { "tDecI", 0 },
  { "tMultI", 0 },
  { "tDivI", 0 },
  { "tAddPI", 0 },
  { "tAddI", 0 },
  { "tSubI", 0 },
  { "tNegI", 0 },
  { "tNot", 0 },
  { "tEqualI", 0 },
  { "tNotEqualI", 0 },
  { "tGreaterI", 0 },
  { "tLessI", 0 },
  { "tGreaterEqualI", 0 },
  { "tLessEqualI", 0 },
  { "tEqualP", 0 },
  { "tNotEqualP", 0 },
  { "tAllocActuals", 1 },
  { "tAllocActualsCdecl", 1 },
  { "tFreeActuals", 1 },
  { "tCall", 1 },
  { "tCallCdecl", 1 },
  { "tReturn", 0 },
  { "tEnter", 1 },
  { "tJump", 1 },
  { "tJumpTrue", 1 },
  { "tJumpFalse", 1 },
  { "tLabel", 1 },
  { "tLabelAlias", 2 },
  { "tLabelExtern", 2 },
  { "tWriteI", 0 },
  { "tWriteBool", 0 },
  { "tWriteStr", 0 },
  { "tWriteP", 0 },
  { "tWriteCR", 0 }
};
int numtCodeInstrs = sizeof(tCodeInstrs) / sizeof(tCodeInstrs[0]);


std::unordered_map<int, int> labels;   // label# -> tcode addr
std::unordered_map<int, int> labelAliases;  // label# -> aliasToLabel#


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
    int instr = code[pc];
    if ( instr == tLabel ) {
      defineLabel( code[pc+1], pc+2 );
    } else if ( instr == tLabelAlias ) {
      defineLabelAlias( code[pc+1], code[pc+2] );
    }
    pc += 1 + tCodeInstrs[instr].args;
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
       case tPushConstI :
              if (++sp>=stackMax) fatal("stack overflow");
              stack[sp] = code[pc++];
              continue;
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
       case tFetchI :
              stack[sp] = *(int*) stack[sp];
              continue;
       case tFetchB :
              stack[sp] = *(char*) stack[sp];
              continue;
       case tFetchP :
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
       case tSubI :
              stack[sp-1] -= stack[sp];
              sp--;
              continue;
       case tNegI :
              stack[sp] *= -1;
              continue;
       case tNot :
              stack[sp] = !stack[sp];
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
       case tCallCdecl :
              printf( "sm: tCallCdecl not supported yet\n" );
              pc++;
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
              printf("%d", (int) stack[sp--]);
              continue;
       case tWriteBool :
              printf(stack[sp--] ? "TRUE" : "FALSE");
              continue;
       case tWriteStr :
              printf("%s", (char*) stack[sp--]);
              continue;
       case tWriteP :
              printf(" <%p>", (void*) stack[sp--]);
              continue;
       case tWriteCR :
              printf("\n");
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
  if((dmp=fopen("a.dmp","w"))==NULL) {
    printf("Can't open dump file a.dmp\n");
    exit(5);
  }

  for ( pc = 0; pc < codeWords; ) {
    fprintf( dmp, "%4d\t", pc );

    int instr = code[pc++];
    if ( instr >= 0 && instr < numtCodeInstrs ) {
      fprintf( dmp, "%s", tCodeInstrs[instr].name );
      for ( int a = 0; a < tCodeInstrs[instr].args; ++a ) {
        fprintf( dmp, "\t%4d", code[pc++] );
      }
      fprintf( dmp, "\n" );
    } else {
      fprintf( dmp, "?\n" );
      pc++;
    }
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

