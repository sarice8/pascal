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
#define dataSize 100000
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
void fatal( char* msg );
void cleanup();


void
main(argc,argv)
int argc;
char *argv[];
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

  /* Execute code */

  pc = 0;                        /* Initialize walker */
  sp = 0;
  call_sp = &data[dataSize] - PTR_SIZE;

  walkTable();

  printf(".Done\n");
  cleanup();
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
       case tAnd :
              stack[sp-1] = stack[sp-1] && stack[sp];
              sp--;
              continue;
       case tOr :
              stack[sp-1] = stack[sp-1] || stack[sp];
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
       case tAllocActuals :
              call_sp -= code[pc++];
              if (call_sp < callStackLowerBound) fatal("call stack overflow");
              continue;
       case tFreeActuals :
              call_sp += code[pc++];
              continue;
       case tCall :
              if (++sp==stackMax) fatal("call stack overflow");
              stack[sp] = pc+1;
              pc = code[pc];
              continue;
       case tReturn :
              if (call_fp) {
                pc = stack[sp--];
                // unwind tEnter
                call_sp = call_fp;
                call_fp = *(char**)(call_fp);
                call_sp += PTR_SIZE;
                call_sp += PTR_SIZE;;  // TO DO: remove when iCall pushes return address on call stack
                continue;
              } else
                return;            /* done program table */
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
              pc = code[pc];
              continue;
       case tJumpTrue :
              if (stack[sp--]) {
                pc = code[pc];
                continue;
              }
              pc++;
              continue;
       case tJumpFalse :
              if (!stack[sp--]) {
                pc = code[pc];
                continue;
              }
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


#define op0(str) { fprintf(dmp,"%s\n",str); continue; }
#define op1(str) { fprintf(dmp,"%s\t%4d\n",str,code[pc++]); continue; }

void
dumpTable()
{
  if((dmp=fopen("a.dmp","w"))==NULL) {
    printf("Can't open dump file a.dmp\n");
    exit(5);
  }

   pc = 0;
   while (pc < codeWords) {
     fprintf(dmp,"%4d\t",pc);
     switch (code[pc++]) {
       case tPushGlobalI :   op1("tPushGlobalI");
       case tPushGlobalB :   op1("tPushGlobalB"); 
       case tPushGlobalP :   op1("tPushGlobalP");
       case tPushLocalI :    op1("tPushLocalI");
       case tPushLocalB :    op1("tPushLocalB"); 
       case tPushLocalP :    op1("tPushLocalP");
       case tPushParamI :    op1("tPushParamI");
       case tPushParamB :    op1("tPushParamB"); 
       case tPushParamP :    op1("tPushParamP");
       case tPushConstI :    op1("tPushConstI");
       case tPushAddrGlobal : op1("tPushAddrGlobal");
       case tPushAddrLocal :  op1("tPushAddrLocal");
       case tPushAddrParam :  op1("tPushAddrParam");
       case tPushAddrActual : op1("tPushAddrActual");
       case tFetchI :        op0("tFetchI");
       case tFetchB :        op0("tFetchB");
       case tFetchP :        op0("tFetchP");
       case tAssignI :       op0("tAssignI");
       case tAssignB :       op0("tAssignB");
       case tAssignP :       op0("tAssignP");
       case tCopy :          op1("tCopy   ");
       case tIncI :          op0("tIncI");
       case tDecI :          op0("tDecI");
       case tMultI :         op0("tMultI");
       case tDivI :          op0("tDivI");
       case tAddI :          op0("tAddI");
       case tSubI :          op0("tSubI");
       case tNegI :          op0("tNegI");
       case tNot :           op0("tNot");
       case tAnd :           op0("tAnd");
       case tOr :            op0("tOr");
       case tEqualI :        op0("tEqualI");
       case tNotEqualI :     op0("tNotEqualI");
       case tGreaterI   :    op0("tGreaterI");
       case tLessI :         op0("tLessI");
       case tGreaterEqualI : op0("tGreaterEqualI");
       case tLessEqualI :    op0("tLessEqualI");
       case tEqualP :        op0("tEqualP");
       case tNotEqualP :     op0("tNotEqualP");
       case tAllocActuals :  op1("tAllocActuals");
       case tFreeActuals :   op1("tFreeActuals");
       case tCall :          op1("tCall   ");
       case tReturn :        op0("tReturn");
       case tEnter :         op1("tEnter");
       case tJump :          op1("tJump   ");
       case tJumpTrue :      op1("tJumpTrue");
       case tJumpFalse :     op1("tJumpFalse");
       case tWriteI :        op0("tWriteI");
       case tWriteBool :     op0("tWriteBool");
       case tWriteStr :      op0("tWriteStr");
       case tWriteP :        op0("tWriteP");
       case tWriteCR :       op0("tWriteCR");
       default :             op0("???");
     }
   }
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
fatal( char* msg )
{
  printf(".FATAL - %s at %d\n",msg,pc);
  exit(5);
}


void
cleanup()
{
}

