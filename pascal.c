/*

  Pascal.c     Steve's Pascal Compiler

               This is the table walker for Pascal.ssl.

  HISTORY
    12Sep89    1st version : scanner and ssl instructions (based on ssl.c)
    18Sep89    Implemented a couple mechanisms (count,patch,typs)
    20Sep89    Implemented symbol table, symbol stack mechanisms
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef AMIGA
#include <dos.h>
#endif // AMIGA


// ----------------------------------------------------------------------

// This version of pascal.c does not yet integrate with ssl_rt.
// But, I want to use ssl 1.2.8 which produces a header file that expects ssl_rt.
// So provide the necessary definitions here.

struct ssl_error_table_struct
{
    char  *msg;
    short  val;
};

extern struct ssl_error_table_struct ssl_error_table[];
extern int ssl_error_table_size;

// ----------------------------------------------------------------------


#define  SSL_INCLUDE_ERR_TABLE
#include "pascal.h"

#define TABLE_FILE "pascal.tbl"   /* This is the ssl code to execute */

/* The scanner is hardcoded in C.

   Scanner variables start with 't_'.
   Table walker variables start with 'w_'.
   Semantic operations start with 'o'.
   Corresponding data structures start with 'd'.

   The code to walk is in w_codeTable (pointer=w_pc).
   The output buffer is w_outTable (pointer=w_here).

   A token is read and screened when oInput, oInputAny, or oInputChoice
   is encountered.  When an id is accepted, set t_lastId to its id #.

   The 'code' of a token is pComma, pIntLit, pIdent, etc.
   The 'id #' of an identifier is the unique # for that name (actually
   the index into the scanner's identifier table).

   The various stacks here don't use the [0] entry.  The stack variable
   points to the top entry.  Overflow is always checked; underflow is
   never checked.

   The scanner maintains an identifier table, which is used to find
   keywords, and to translate names into integers.  Whenever a new
   identifier is found, it is added to the table.  It is separate from
   the symbol table constructed by the compiler's semantic operations.

*/

short w_codeTable[SSL_CODE_TABLE_SIZE];     /* size defined in .h file */

FILE *t_src,*t_out,*t_doc,*t_lst;

#define t_lineSize 256
#define t_strSize 256
#define t_nameSize 31
#define t_idTableSize 400

char   t_lineBuffer[t_lineSize];
char   t_lineBuffer2[t_lineSize];
char   *t_ptr;
char   t_strBuffer[t_strSize+1];   /* for StrLit tokens */
short  t_lineNumber;
char   t_lineListed;
char   t_listing;                  /* do we want a listing file? */
short  t_lastId;                   /* id# of last id accepted */

/* token codes and id kinds are declared in the SSL program, not here */

char t_upper[256],t_blank[256],t_idStart[256],t_idCont[256];
char t_digit[256];
short t_punct[256];

struct t_tokenType {
  char accepted;
  char name[t_nameSize+1];
  short namelen;
  short val;                   /* id #, int lit, etc */
  short code;                  /* input token code (pIdent, pIntLit, etc) */
  short lineNumber;
  short colNumber;
} t_token;

#define t_idBufferSize 5000
char t_idBuffer[t_idBufferSize];  /* id names stored here.. avg 12 chars */
short t_idBufferNext;

struct t_idTableType {         /* identifier # is index into this table */
  char *name;
  short namelen;
  short code;                  /* keyword code, or 'pIdent' */
} t_idTable[t_idTableSize];
short t_idTableNext;

struct t_kwTableType {
  char *name;
  short code;
} t_kwTable[] = {
  "program",pProgram,
  "procedure",pProcedure,
  "function",pFunction,
  "const",pConst,
  "type",pType,
  "var",pVar,
  "begin",pBegin,
  "end",pEnd,
  "array",pArray,
  "record",pRecord,
  "set",pSet,
  "of",pOf,
  "if",pIf,
  "then",pThen,
  "else",pElse,
  "for",pFor,
  "to",pTo,
  "downto",pDownto,
  "do",pDo,
  "while",pWhile,
  "repeat",pRepeat,
  "until",pUntil,
  "cycle",pCycle,
  "exit",pExit,
  "return",pReturn,
  "and",pAnd,
  "or",pOr,
  "not",pNot,
  "writeln",pWriteln,
  "write",pWrite,
  "readln",pReadln,
  "read",pRead,
  "",0
};


// Forward declarations
void t_initScanner ();
void t_getNextToken();
void t_lookupId();
short t_addId( char* name, short code );
short lookup(short id);
int t_hitBreak();
void t_cleanup();
void t_fatal( char* msg );
void t_printToken();
void t_error( char* msg );
void t_dumpTables();

void w_initSemanticOperations();
void w_walkTable();
void w_traceback();
void w_errorSignal( short err );



#define t_getToken()   { if (t_token.accepted) t_getNextToken(); }

/* Variables for table walker */

#define w_stackSize 200
short w_stack[w_stackSize],w_sp;   /* Call stack */
short w_pc;                        /* Program counter */
short w_result, w_param;           /* For oSetResult, oSetParam */
short w_options;                   /* # choice options */
char w_errBuffer[256];             /* build up err messages here */

#define w_outTableSize 5000
short w_outTable[w_outTableSize];  /* Emit into this table */
short w_here;                      /* Output position */
short w_errorCount;                /* Number of error signals emitted */

/*************** Variables for semantic mechanisms ***************/

short dTemp;                       /* multi-purpose */
short *dPtr;                       /* multi-purpose */
short dWords;                      /* multi-purpose */

short dSym;                        /* index of looked-up symbol in ST*/

struct dStype {                    /* entry for symbol table, symbol stack */
  short id;        /* id# of name */
  short kind;      /* kVar, kProgram, etc */
  short val;       /* constant value */
  short paramTyp;  /* type of proc/func params */
                   /*   (tyNone, or type# of a type of class 'tyParams') */
  short typ;       /* type of var, type, func */
};

#define dSTsize 200
struct dStype dST[dSTsize];               /* symbol table */
short dSTptr;

#define dSDsize 15
struct dSDtype {            /* symbol table display */
  short sym, allocOffset;
} dSD[dSDsize];
short dSDptr;

#define dSSsize 40
struct dStype dSS[dSSsize];               /* symbol stack */
short dSSptr;

#define dTTsize 50
struct dTTtype {          /* type table */
  short kind;             /* for user types: array, record, set, etc */
  short size;             /* #words required for this type */
  short low, high;        /* range of an array dimension */
  short base;             /* base type of this type (e.g. array OF-type) */
  short ptrType;          /* type# of pointer to this type (0 if none yet) */
} dTT[dTTsize];
short dTTptr;
short dLastScalar;        /* typ# of last basic type (user typ#'s follow) */

#define dTSsize 40
short dTS[dTSsize], dTSptr;        /* type stack ... index into TT */

#define dCSsize 30
short dCS[dCSsize], dCSptr;        /* count stack */

#define dVSsize 30
short dVS[dVSsize], dVSptr;        /* value stack */

#define dSLsize 400
short dSL[dSLsize], dSLptr;        /* string literal table */

/* code patch stacks */

#define dPatchLsize 40
#define dPatchEsize 200
#define dPatchIsize 40

short dPatchL[dPatchLsize];    /* loop starts */
short dPatchE[dPatchEsize];    /* loop exits */
short dPatchI[dPatchIsize];    /* 'if' jumps */

/* array pointing to patch stacks */

struct dPStype {
  short *stack;
  short ptr;
  short size;
} dPS[3] = 
   {dPatchL, 0, dPatchLsize,
    dPatchE, 0, dPatchEsize,
    dPatchI, 0, dPatchIsize};


void
main(argc,argv)
int argc;
char *argv[];
{
  int entries,temp;                /* I can't seem to read a 'short' */
  short arg;

  /* Prepare Files */

  t_listing = 0;
  arg = 1;
  if (arg<argc && argv[arg][0]=='-') {
    if (argv[arg][1]=='L' || argv[arg][1]=='l')
      t_listing = 1;
    arg++;
  }
  if (arg>=argc) {
    printf("Usage:  pascal [-List] <file>\n");
    exit(10);
  }
  if((t_src=fopen(TABLE_FILE,"r"))==NULL) {
    printf("Can't open table file %s\n",TABLE_FILE);
    exit(10);
  }
  char* p = fgets(t_lineBuffer,t_lineSize,t_src);   /* get title string */
  assert( p != NULL );
  p = fgets(t_lineBuffer2,t_lineSize,t_src);   /* get ssl_rt string */
  assert( p != NULL );
  int read = fscanf(t_src,"%d",&entries);
  assert( read == 1 );
  if (entries>SSL_CODE_TABLE_SIZE) {
    printf("Table file too big; must recompile %s.c\n",argv[0]);
    exit(10);
  }
  for (w_pc=0; w_pc<entries; w_pc++) {
    read = fscanf(t_src,"%d",&temp);
    assert( read == 1 );
    w_codeTable[w_pc] = temp;
  }
  fclose(t_src);
  printf("%s",t_lineBuffer);   /* print title string */
  strcpy(t_lineBuffer,argv[arg]);
  strcpy(t_lineBuffer+strlen(argv[arg]),".pas");
  if((t_src=fopen(t_lineBuffer,"r"))==NULL) {
    printf("Can't open source file %s\n",t_lineBuffer);
    exit(10);
  }
  if((t_out=fopen("a.out","w"))==NULL) {
    printf("Can't open output file ram_a.out\n");
    exit(10);
  }
  if((t_doc=fopen("a.doc","w"))==NULL) {
    printf("Can't open output file ram_a.doc\n");
    exit(10);
  }
  if (t_listing) {
    if((t_lst=fopen("a.lst","w"))==NULL) {
      printf("Can't open listing file ram_a.lst\n");
      exit(10);
    }
  }

#ifdef AMIGA
  onbreak(&t_hitBreak);
#endif // AMIGA

  t_initScanner();

  w_pc = 0;                        /* Initialize walker */
  w_sp = 0;
  w_here = 0;
  w_errorCount = 0;

  w_initSemanticOperations();

  w_walkTable();

  if (w_errorCount)
    printf("\n%d error(s)\n",w_errorCount);

  t_cleanup();
  if (w_errorCount)
    exit(5);
}


/*********** S e m a n t i c   O p e r a t i o n s ***********/

void
w_errorSignal( short err )
{
  short i;
  i = err;
  if (ssl_error_table[i].val != i)
    for (i=0; i<ssl_error_table_size && ssl_error_table[i].val != err; i++);
  sprintf(w_errBuffer,"Error: %s",ssl_error_table[i].msg);
  t_error(w_errBuffer);
}

void
w_initSemanticOperations()
{
  /* Symbol display level 0 is for global symbols.
     Symbol table entry 0 is unused; it is referenced when an undefined
     name is searched for. */

  dSD[0].sym = 1;
  dSD[0].allocOffset = 0;     /* offset of next var to allocate */
  dST[0].kind = kUndefined;
  dSTptr = 0;

  /* built-in types */
  /* note, entry 0 of TT is not used */

  dST[++dSTptr].id = t_addId("integer",pIdent);
  dST[dSTptr].kind = kType;
  dST[dSTptr].typ  = tyInteger;
  dTT[tyInteger].size = 1;

  dST[++dSTptr].id = t_addId("boolean",pIdent);
  dST[dSTptr].kind = kType;
  dST[dSTptr].typ  = tyBoolean;
  dTT[tyBoolean].size = 1;

  dST[++dSTptr].id = t_addId("char",pIdent);
  dST[dSTptr].kind = kType;
  dST[dSTptr].typ  = tyChar;
  dTT[tyChar].size = 1;

  dST[++dSTptr].id = t_addId("string",pIdent);
  dST[dSTptr].kind = kType;
  dST[dSTptr].typ  = tyString;
  dTT[tyString].size = 128;  /* words */

  dST[++dSTptr].id = t_addId("file",pIdent);
  dST[dSTptr].kind = kType;
  dST[dSTptr].typ  = tyFile;
  dTT[tyFile].size = 1;

  dTTptr = tyFile;
  dLastScalar = tyFile;

  /* built-in constants */

  dST[++dSTptr].id = t_addId("true",pIdent);
  dST[dSTptr].kind = kConst;
  dST[dSTptr].typ  = tyBoolean;
  dST[dSTptr].val  = 1;

  dST[++dSTptr].id = t_addId("false",pIdent);
  dST[dSTptr].kind = kConst;
  dST[dSTptr].typ  = tyBoolean;
  dST[dSTptr].val  = 0;

}


void
w_walkTable()
{
   while (1) {
     switch (w_codeTable[w_pc++]) {

       /* SSL Instructions */

       case oJumpForward :
              w_pc += w_codeTable[w_pc];
              continue;
       case oJumpBack :
              w_pc -= w_codeTable[w_pc];
              continue;
       case oInput :
              t_getToken();
              if (t_token.code != w_codeTable[w_pc++])
                t_error ("Syntax error: Missing required token");
              t_token.accepted = 1;
              if (t_listing && !t_lineListed) {
                fprintf(t_lst,"%4d: %s",w_here,t_lineBuffer);
                t_lineListed = 1;
              }
              if (t_token.code == pIdent)
                t_lastId = t_token.val;
              continue;
       case oInputAny :
              t_getToken();
              t_token.accepted = 1;
              if (t_listing && !t_lineListed) {
                fprintf(t_lst,"%4d: %s",w_here,t_lineBuffer);
                t_lineListed = 1;
              }
              if (t_token.code == pIdent)
                t_lastId = t_token.val;
              continue;
       case oEmit :
              dTemp = w_codeTable[w_pc++];
              switch (dTemp) {
                case tConstant :   w_outTable[w_here++] = dVS[dVSptr];
                                   continue;
                case tSpace :      w_outTable[w_here++] = 0;
                                   continue;
                case tSymVal :     w_outTable[w_here++] = dST[dSym].val;
                                   continue;
                default :          w_outTable[w_here++] = dTemp;
                                   continue;
              }
       case oError :
              w_errorSignal(w_codeTable[w_pc++]);
              continue;
       case oInputChoice :
              t_getToken();
              w_pc += w_codeTable[w_pc];
              w_options = w_codeTable[w_pc++];
              while (w_options--) {
                if (w_codeTable[w_pc++] == t_token.code) {
                  w_pc -= w_codeTable[w_pc];
                  t_token.accepted = 1;
                  if (t_listing && !t_lineListed) {
                    fprintf(t_lst,"%4d: %s",w_here,t_lineBuffer);
                    t_lineListed = 1;
                  }
                  if (t_token.code == pIdent)
                    t_lastId = t_token.val;
                  break;
                }
                w_pc++;
              }
              continue;
       case oCall :
              if (++w_sp==w_stackSize) t_fatal ("SSL: call stack overflow");
              w_stack[w_sp] = w_pc+1;
              w_pc = w_codeTable[w_pc];
              continue;
       case oReturn :
              if (w_sp) {
                w_pc = w_stack[w_sp--];
                continue;
              } else
                return;            /* done walking table */
       case oSetResult :
              w_result = w_codeTable[w_pc++];
              continue;
       case oChoice :
              w_pc += w_codeTable[w_pc];
              w_options = w_codeTable[w_pc++];
              while (w_options--) {
                if (w_codeTable[w_pc++] == w_result) {
                  w_pc -= w_codeTable[w_pc];
                  break;
                }
                w_pc++;
              }
              continue;
       case oEndChoice :           /* choice or inputChoice didn't match */
              t_error ("Syntax error");
              t_token.accepted = 1;  /* actually, only if input choice err*/
              continue;
       case oSetParameter :
              w_param = w_codeTable[w_pc++];
              continue;

       /* Mechanism count */

       case oCountPush :
              if (++dCSptr==dCSsize) t_fatal("CS overflow");
              dCS[dCSptr] = w_param;
              continue;
       case oCountInc :
              dCS[dCSptr]++;
              continue;
       case oCountDec :
              dCS[dCSptr]--;
              continue;
       case oCountIsZero :
              w_result = !dCS[dCSptr];
              continue;
       case oCountPop :
              dCSptr--;
              continue;

       /* Mechanism value */

       case oValuePushToken :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = t_token.val;
              continue;
       case oValuePushVal :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = dST[dSym].val;
              continue;
       case oValuePush :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = w_param;
              continue;
       case oValuePushSizeTS :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = dTT[dTS[dTSptr]].size;
              continue;
       case oValuePushLowTS :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = dTT[dTS[dTSptr]].low;
              continue;
       case oValueNegate :
              dVS[dVSptr] *= -1;
              continue;
       case oValueDifference :
              dVS[dVSptr-1] = dVS[dVSptr] - dVS[dVSptr-1] + 1;
              dVSptr--;
              continue;
       case oValueMultiply :
              dVS[dVSptr-1] *= dVS[dVSptr];
              dVSptr--;
              continue;
       case oValueIsZero :
              w_result = !dVS[dVSptr];
              continue;
       case oValueIsOne :
              w_result = (dVS[dVSptr] == 1);
              continue;
       case oValuePop :
              dVSptr--;
              continue;

       /* Mechanism string */

       case oStringAllocLit :
              dWords = (t_token.namelen + 2) / 2;  /* #words for string */
              if (dSLptr + dWords + 2 >= dSLsize) t_fatal("SL overflow");
              /* push string addr (global data ptr) on value table */
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = dSD[0].allocOffset;
              /* save in strlit table: <addr> <words> <data> */
              dSL[dSLptr++] = dSD[0].allocOffset;
              dSL[dSLptr++] = dWords;
              dPtr = (short *) t_strBuffer;
              for (dTemp = 0; dTemp < dWords; dTemp++)
                dSL[dSLptr++] = *dPtr++;
              /* advance global data ptr */
              dSD[0].allocOffset += dWords;
              continue;

       /* Mechanism typ */

       case oTypSPushTyp :
              if (++dTSptr==dTSsize) t_fatal("TS overflow");
              dTS[dTSptr] = dST[dSym].typ;
              continue;
       case oTypSPush :
              if (++dTSptr==dTSsize) t_fatal("TS overflow");
              dTS[dTSptr] = w_param;
              continue;
       case oTypSPopPushBase :
              dTS[dTSptr] = dTT[dTS[dTSptr]].base;
              continue;
       case oTypSPopPushPtr :
              dTS[dTSptr] = dTT[dTS[dTSptr]].ptrType;
              continue;
       case oTypSMatch :     /* actually, should follow to roots of types */
              w_result = (dTS[dTSptr] == dTS[dTSptr-1]);
              dTSptr--;
              continue;
       case oTypSChoose :
              w_result = dTS[dTSptr];
              continue;
       case oTypSChoosePop :
              w_result = dTS[dTSptr--];
              continue;
       case oTypSChooseKind :
              w_result = dTT[dTS[dTSptr]].kind;
              continue;
       case oTypSChoosePtr :
              w_result = dTT[dTS[dTSptr]].ptrType;
              continue;
       case oTypSSwap :
              dTemp = dTS[dTSptr-1];
              dTS[dTSptr-1] = dTS[dTSptr];
              dTS[dTSptr] = dTemp;
              continue;
       case oTypSPop :
              dTSptr--;
              continue;
       case oTypNew :
              if (++dTTptr==dTTsize) t_fatal("TT overflow");
              if (++dTSptr==dTSsize) t_fatal("TS overflow");
              dTT[dTTptr].kind = w_param;
              dTT[dTTptr].ptrType = tyNone; /* no ptr to this type defined */
              dTS[dTSptr] = dTTptr;
              continue;
       case oTypSetLow :
              dTT[dTS[dTSptr]].low = dVS[dVSptr];
              continue;
       case oTypSetHigh :
              dTT[dTS[dTSptr]].high = dVS[dVSptr];
              continue;
       case oTypSetSize :
              dTT[dTS[dTSptr]].size = dVS[dVSptr];
              continue;
       case oTypAssignBasePop :
              dTT[dTS[dTSptr-1]].base = dTS[dTSptr];
              dTSptr--;
              continue;
       case oTypAssignPtr :
              dTT[dTS[dTSptr-1]].ptrType = dTS[dTSptr];
              continue;

       /* Mechanism patch */

       case oPatchPushHere :
              if (++dPS[w_param].ptr==dPS[w_param].size)
                    t_fatal("patch overflow");
              dPS[w_param].stack[dPS[w_param].ptr] = w_here;
              continue;
       case oPatchAnyEntries :
              w_result = dPS[w_param].ptr>0;
              continue;
       case oPatchSwap :
              dPtr = &dPS[w_param].stack[dPS[w_param].ptr-1];
              dTemp = dPtr[0];
              dPtr[0] = dPtr[1];
              dPtr[1] = dTemp;
              continue;
       case oPatchDup :
              dPtr = &dPS[w_param].stack[dPS[w_param].ptr++];
              dPtr[1] = dPtr[0];
              continue;
       case oPatchPopFwd :
              dTemp = dPS[w_param].stack[dPS[w_param].ptr--];
              w_outTable[dTemp] = w_here;
              continue;
       case oPatchPopBack :
              w_outTable[w_here] = dPS[w_param].stack[dPS[w_param].ptr--];
              w_here++;
              continue;

       /* Mechanism sym */

       case oSymPushLevel :
              if (++dSDptr==dSDsize) t_fatal("SD overflow");
              dSD[dSDptr].sym = dSTptr + 1;
              dSD[dSDptr].allocOffset = 0;
              continue;
       case oSymPopLevel :
              dSTptr = dSD[dSDptr--].sym - 1;
              continue;
       case oSymPopLevelSaveType :
              /* attach id's at top level to syms field of top type stack */
              /****/
              dSTptr = dSD[dSDptr--].sym - 1;
              continue;
       case oSymLookup :
              dSym = lookup(t_lastId);
              continue;
       case oSymChooseKind :
              w_result = dST[dSym].kind;
              continue;
       case oSymAddSPop :
              if (++dSTptr==dSTsize) t_fatal("ST overflow");
              dST[dSTptr] = dSS[dSSptr--];
              continue;
       case oSymLevelAnySyms :
              w_result = dSTptr >= dSD[dSDptr].sym;
              continue;
       case oSymSPushId :
              if (++dSSptr==dSSsize) t_fatal("SS overflow");
              dSS[dSSptr].id = t_lastId;
              /* perhaps clear other fields, for debugging */
              continue;
       case oSymSSetKind :
              dSS[dSSptr].kind = w_param;
              continue;
       case oSymSSetValPop :
              dSS[dSSptr].val = dVS[dVSptr--];
              continue;
       case oSymSSetTypS :
              dSS[dSSptr].typ = dTS[dTSptr];
              continue;
       case oSymSSetParamTypS :
              dSS[dSSptr].paramTyp = dTS[dTSptr];
              continue;
       case oSymSAllocate :
              dSS[dSSptr].val = dSD[dSDptr].allocOffset;
              dSD[dSDptr].allocOffset += dTT[dTS[dTSptr]].size;
              continue;

       default:
              w_pc--;
              sprintf(w_errBuffer,"SSL: Bad instruction #%d at %d",
                      w_codeTable[w_pc],w_pc);
              t_fatal(w_errBuffer);
     } /* switch */
   }
}


/* Search the symbol table for the id given.  If found, return index in
   table.  Otherwise, return 0. */

short
lookup( short id )
{
  short i;
  for (i=dSTptr; i>=1; i--)
    if (dST[i].id == id)
      return(i);
  return(0);
}


void
w_traceback()
{
  short i;
  printf("SSL Traceback:\n");
  fprintf(t_doc,"SSL Traceback:\n");
  printf("  %d",w_pc);
  fprintf(t_doc,"  %d",w_pc);
  for (i=w_sp;i>0;i--) {
    printf("  called from\n  %d",w_stack[i]-2);
    fprintf(t_doc,"  called from\n  %d",w_stack[i]-2);
  }
  printf("\n");
  fprintf(t_doc,"\n");
}

/************************ S c a n n e r *********************/

void
t_initScanner ()
{
  register short i;
  short addId();

  /* C guarantees static vars to start with 0, so here I don't
       bother to initialize all of blank,idStart,idCont,digit */

  for (i=0;i<256;i++)
    t_upper[i] = i;
  for (i='a';i<='z';i++) {
    t_upper[i] = i - ('a'-'A');
    t_idStart[i] = 1;
    t_idCont[i] = 1;
  }
  for (i='A';i<='Z';i++) {
    t_idStart[i] = 1;
    t_idCont[i] = 1;
  }
  t_idStart['_'] = 1;
  t_idCont['_'] = 1;
  t_idStart['$'] = 1;
  t_idCont['$'] = 1;
  for (i='0';i<='9';i++) {
    t_digit[i] = 1;
    t_idCont[i] = 1;
  }
  t_blank[' '] = 1;
  t_blank['\t'] = 1;
  t_blank['\n'] = 1;

  /* single-character tokens */
  for (i=0;i<256;i++)
    t_punct[i] = pInvalid;
  t_punct['='] = pEqual;
  t_punct[';'] = pSemiColon;
  t_punct[':'] = pColon;
  t_punct[','] = pComma;
  t_punct['('] = pLParen;
  t_punct[')'] = pRParen;
  t_punct['['] = pLSquare;
  t_punct[']'] = pRSquare;
  t_punct['^'] = pCarat;
  t_punct['*'] = pTimes;
  t_punct['/'] = pDivide;
  t_punct['+'] = pPlus;
  t_punct['-'] = pMinus;
  t_lineBuffer[0] = '\0';
  t_ptr = t_lineBuffer;
  t_lineNumber = 0;
  t_lineListed = 1;
  t_idTableNext = 0;
  t_token.code = pColon;           /* dummy */
  t_token.accepted = 1;
  for (i=0; *t_kwTable[i].name; i++)
    (void) t_addId(t_kwTable[i].name,t_kwTable[i].code);
}  


void
t_getNextToken()
{
  char *p;
  short lit;

  t_token.accepted = 0;

  /* error if we try to read past the end of file */
  if (t_token.code == pEof)              /* last token was eof */
    t_fatal ("Unexpected end of file"); 

  /* skip to start of token */

  while (1) {
    while (t_blank[*t_ptr])
      t_ptr++;
    if (!*t_ptr) {
      if (t_listing && !t_lineListed)
        fprintf(t_lst,"      %s",t_lineBuffer);
      if (!fgets(t_lineBuffer,t_lineSize,t_src))
            {  t_token.code = pEof; return;  }
      t_ptr = t_lineBuffer;
      t_lineNumber++;
      t_lineListed = 0;
    } else if (*t_ptr=='{') {
      t_ptr++;
      while (*t_ptr!='}') {
        if (!*t_ptr++) {
          if (t_listing && !t_lineListed)
            fprintf(t_lst,"      %s",t_lineBuffer);
          if (!fgets(t_lineBuffer,t_lineSize,t_src))
                {  t_token.code = pEof; return;  }
          t_ptr = t_lineBuffer;
          t_lineNumber++;
          t_lineListed = 0;
        }
      }
      t_ptr++;
      continue;
    } else {
      break;
    }
  }

  /* copy token, screen ids and integers */

  t_token.lineNumber = t_lineNumber;
  t_token.colNumber = t_ptr-t_lineBuffer;    /* from 0, for err msgs */
  p = t_token.name;
  if (t_idStart[*t_ptr]) {
    while (t_idCont[*t_ptr] && p-t_token.name < t_nameSize)
      *p++ = *t_ptr++;
    *p = '\0';
    t_token.namelen = p-t_token.name;
    t_token.code = pIdent;
    t_lookupId();
  } else if (t_digit[*t_ptr]) {
    lit = 0;
    while (t_digit[*t_ptr]) {
      lit = lit*10 - '0' + *t_ptr;
      *p++ = *t_ptr++;
    }
    *p = '\0';
    t_token.val = lit;
    t_token.code = pIntLit;
  } else if (*t_ptr=='\'') {
    p = t_strBuffer;
    t_ptr++;
    t_token.code = pStrLit;
    while (p-t_strBuffer < t_strSize) {
      if (*t_ptr=='\'') {
        if (*++t_ptr=='\'') {
          *p++ = '\'';
          t_ptr++;
        } else {
          break;
        }
      } else if (*t_ptr=='\0') {
        break;
      } else {
        *p++ = *t_ptr++;
      }
    }
    *p = '\0';
    t_token.namelen = p-t_strBuffer;
  } else if (*t_ptr==':') {
    if (*++t_ptr=='=') {
      t_ptr++;
      t_token.code = pAssign;
    } else {
      t_token.code = pColon;
    }
  } else if (*t_ptr=='<') {
    if (*++t_ptr=='>') {
      t_ptr++;
      t_token.code = pNotEqual;
    } else if (*t_ptr=='=') {
      t_ptr++;
      t_token.code = pLessEqual;
    } else {
      t_token.code = pLess;
    }
  } else if (*t_ptr=='>') {
    if (*++t_ptr=='=') {
      t_ptr++;
      t_token.code = pGreaterEqual;
    } else {
      t_token.code = pGreater;
    }
  } else if (*t_ptr=='.') {
    if (*++t_ptr=='.') {
      t_ptr++;
      t_token.code = pDotDot;
    } else {
      t_token.code = pDot;
    }
  } else {
    t_token.code = t_punct[*t_ptr++];
    if (t_token.code == pInvalid) {
      --t_ptr;
      t_token.name[0] = *t_ptr++;
      t_token.name[1] = '\0';
      t_error ("Invalid character");
    }
  }
}


short t_addId( char* name, short code )
{
  short namelen;
  if (t_idTableNext >= t_idTableSize)
    t_fatal("id table overflow");
  namelen = strlen(name);
  t_idTable[t_idTableNext].namelen = namelen;
  if (t_idBufferNext+namelen >= t_idBufferSize)
    t_fatal("id name buffer overflow");
  t_idTable[t_idTableNext].name = &t_idBuffer[t_idBufferNext];
  strcpy(&t_idBuffer[t_idBufferNext],name);
  t_idBufferNext += namelen + 1;
  t_idTable[t_idTableNext].code = code;
  return(t_idTableNext++);
}


void
t_lookupId()
{
  short i;
  register char *a,*b;

  for (i=0;i<t_idTableNext;i++) {
    if (t_token.namelen==t_idTable[i].namelen) {
      a = t_token.name;
      b = t_idTable[i].name;
      while (t_upper[*a++]==t_upper[*b++]) {
        if (!*a) {   /* match */
          t_token.code = t_idTable[i].code;  /* for built-in ids */
          t_token.val = i;                   /* for user-defined ids */
          return;
        }
      }
    }
  }
  t_token.val = t_addId(t_token.name,pIdent);
}


int
t_hitBreak()
{
  printf("Breaking...\n");
  t_cleanup();
  return(1);
}


void
t_cleanup()
{
  t_dumpTables();
  fclose(t_src);
  fclose(t_out);
  fclose(t_doc);
  if (t_listing) fclose(t_lst);
}


void
t_fatal( char* msg )
{
  printf("[FATAL] ");
  fprintf(t_doc,"[FATAL] ");
  t_error (msg);
  w_traceback();
  t_cleanup();
  exit(5);
}


void
t_printToken()
{
  printf(" (Token=");
  fprintf(t_doc," (Token=");
  if (t_token.code==pIdent) {
    printf("%s)\n",t_token.name);
    fprintf(t_doc,"%s)\n",t_token.name);
  } else if (t_token.code==pStrLit) {
    printf("'%s')\n",t_strBuffer);
    fprintf(t_doc,"'%s')\n",t_strBuffer);
  } else if (t_token.code==pInvalid) {
    printf("'%s')\n",t_token.name);
    fprintf(t_doc,"'%s')\n",t_token.name);
  } else {
    printf("<%d>)\n",t_token.code);
    fprintf(t_doc,"<%d>)\n",t_token.code);
  }
}


void
t_error( char* msg )
{
  char *p,spaceBuf[256];
  short col;

  w_errorCount++;
  printf("%s",t_lineBuffer);
  fprintf(t_doc,"%s",t_lineBuffer);
  for (p = spaceBuf, col = t_token.colNumber; col; *p++ = ' ', col--);
  *p = '\0';
  printf("%s^\n",spaceBuf);
  fprintf(t_doc,"%s^\n",spaceBuf);
  printf("%s on line %d\n",msg,t_token.lineNumber);
  fprintf(t_doc,"%s on line %d\n",msg,t_token.lineNumber);
  /* t_printToken(); */
}


void
t_dumpTables()
{
  short i,col;
  printf("\nWriting Files...\n");

  /* code segment */
  fprintf(t_out,"%d\n",w_here);
  col = 0;
  for (i=0; i<w_here; i++) {
    fprintf(t_out,"%6d",w_outTable[i]);
    if ((++col % 10) == 0)
      fprintf(t_out,"\n");
  }

  /* string literals table */
  for (i=0; i<dSLptr; i++) {
    fprintf(t_out,"%6d",dSL[i]);
    if ((++col % 10) == 0)
      fprintf(t_out,"\n");
  }
}

