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

// SSL Runtime module definitions
#include "ssl_rt.h"

// Schema database access functions
#include "node.h"

// SSL definitions generated for this program
#define  SSL_INCLUDE_ERR_TABLE
#include "pascal.h"

// SSL code generated for this program
#include "pascal.tbl"


/* The scanner is hardcoded in C.

   Scanner variables start with 't_'.
   Table walker variables start with 'w_'.
   Semantic operations start with 'o'.
   Corresponding data structures start with 'd'.

   The code to walk is in w_codeTable (pointer=w_pc).
   The output buffer is w_outTable (pointer=w_here).

   A token is read and screened when oInput, oInputAny, or oInputChoice
   is encountered.  When an id is accepted, set ssl_last_id to its id #.

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

FILE *t_out,*t_doc,*t_lst;

#define t_lineSize 256
#define t_strSize 256
#define t_nameSize 31
#define t_idTableSize 400

char   t_lineBuffer[t_lineSize];
char   t_lineBuffer2[t_lineSize];
char   *t_ptr;
short  t_lineNumber;
char   t_lineListed;
char   t_listing;                  /* do we want a listing file? */

/* token codes and id kinds are declared in the SSL program, not here */

char t_upper[256],t_blank[256],t_idStart[256],t_idCont[256];
char t_digit[256];
short t_punct[256];

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
void open_my_files();
void close_my_files();
void init_my_scanner();
void init_my_operations();

void t_initScanner ();
void t_getNextToken();
short lookup(short id);
void t_printToken();
void t_dumpTables();

void w_walkTable();




/* Variables for table walker */

#define w_stackSize 200
short w_stack[w_stackSize],w_sp;   /* Call stack */
short w_pc;                        /* Program counter */
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

// symbol stack
#define dSSsize 40
struct dStype dSS[dSSsize];
short dSSptr;

// scope stack
#define dScopeStackSize 40
NODE_PTR        dScopeStack[dScopeStackSize];
int             dScopeStackPtr;


// OLD:
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

// OLD:
#define dTSsize 40
short dTS[dTSsize], dTSptr;        /* type stack ... index into TT */

// New type table, owns nType nodes.
// Need to make dynamic size.
#define dTypeTableSize 50
NODE_PTR dTypeTable[dTypeTableSize];
int dTypeTablePtr;

// Type stack
#define dTypeStackSize 50
NODE_PTR dTypeStack[dTypeStackSize];
int dTypeStackPtr;

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


// A dynamic vector of NODE_PTR's
typedef struct NodeVecStruct {
  int size;
  int capacity;
  NODE_PTR*  elements;
} NodeVec, *NODE_VEC_PTR;


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

  ssl_init();
  init_my_scanner();

  strcpy(t_lineBuffer,argv[arg]);
  strcpy(t_lineBuffer+strlen(argv[arg]),".pas");

  ssl_set_input_filename( t_lineBuffer );
  ssl_set_recovery_token( pSemiColon );


  open_my_files();

  ssl_set_init_operations_callback( init_my_operations );

#ifdef AMIGA
  onbreak(&t_hitBreak);
#endif // AMIGA


  // Execute SSL program
  int status = ssl_run_program();

  close_my_files();

  if (status != 0) {
    exit( -1 );
  }

#if 0
  if (w_errorCount)
    printf("\n%d error(s)\n",w_errorCount);

  t_cleanup();
  if (w_errorCount)
    exit(5);
#endif

  exit( 0 );
}


/************************ S c a n n e r *********************/

struct ssl_token_table_struct my_keyword_table[] = {
  "program",            pProgram,
  "procedure",          pProcedure,
  "function",           pFunction,
  "const",              pConst,
  "type",               pType,
  "var",                pVar,
  "begin",              pBegin,
  "end",                pEnd,
  "array",              pArray,
  "record",             pRecord,
  "set",                pSet,
  "of",                 pOf,
  "if",                 pIf,
  "then",               pThen,
  "else",               pElse,
  "for",                pFor,
  "to",                 pTo,
  "downto",             pDownto,
  "do",                 pDo,
  "while",              pWhile,
  "repeat",             pRepeat,
  "until",              pUntil,
  "cycle",              pCycle,
  "exit",               pExit,
  "return",             pReturn,
  "and",                pAnd,
  "or",                 pOr,
  "not",                pNot,
  "writeln",            pWriteln,
  "write",              pWrite,
  "readln",             pReadln,
  "read",               pRead,

  NULL,                 0
};


struct ssl_token_table_struct my_operator_table[] = {
  "=",                  pEqual,
  ";",                  pSemiColon,
  ":",                  pColon,
  ":=",                 pAssign,
  ",",                  pComma,
  "(",                  pLParen,
  ")",                  pRParen,
  "[",                  pLSquare,
  "]",                  pRSquare,
  "^",                  pCarat,
  "*",                  pTimes,
  "/",                  pDivide,
  "+",                  pPlus,
  "-",                  pMinus,
  "<",                  pLess,
  "<=",                 pLessEqual,
  "<>",                 pNotEqual,
  ">",                  pGreater,
  ">=",                 pGreaterEqual,
  ".",                  pDot,
  "..",                 pDotDot,

  NULL,                 0
};


struct ssl_special_codes_struct my_special_codes;

void
init_my_scanner()
{
  // Pascal is case-insensitive!  Variables and keywords match regardless of case.
  ssl_set_case_sensitive( 0 );

  // TO DO: Pascal identifiers must start with A-Z or underscore.
  //        They continue with A-Z, 0-9, underscore.
  //        They do -not- allow $  which the current hardcoded scanner accepts.

  my_special_codes.invalid = pInvalid;
  my_special_codes.eof     = pEof;
  my_special_codes.ident   = pIdent;
  my_special_codes.intlit  = pIntLit;
  my_special_codes.strlit  = pStrLit;

  ssl_init_scanner( my_keyword_table, my_operator_table, &my_special_codes );

  ssl_scanner_init_comment( "(*", "*)" );  // Original pascal comments
  ssl_scanner_init_comment( "{", "}" );    // Introduced by Turbo Pascal
  ssl_scanner_init_comment( "//", "" );    // to end of line. Delphi pascal.
}




/*********** S e m a n t i c   O p e r a t i o n s ***********/

void
init_my_operations()
{
  w_here = 0;

  // Initialize node package (schema runtime module)
  nodeInit();

  /* Symbol display level 0 is for global symbols.
     Symbol table entry 0 is unused; it is referenced when an undefined
     name is searched for. */

  dSD[0].sym = 1;
  dSD[0].allocOffset = 0;     /* offset of next var to allocate */
  dST[0].kind = kUndefined;
  dSTptr = 0;

  /* built-in types */
  /* note, entry 0 of TT is not used */

#if 0
  dST[++dSTptr].id = ssl_add_id("integer",pIdent);
  dST[dSTptr].kind = kType;
  dST[dSTptr].typ  = tyInteger;
  dTT[tyInteger].size = 1;

  dST[++dSTptr].id = ssl_add_id("boolean",pIdent);
  dST[dSTptr].kind = kType;
  dST[dSTptr].typ  = tyBoolean;
  dTT[tyBoolean].size = 1;

  dST[++dSTptr].id = ssl_add_id("char",pIdent);
  dST[dSTptr].kind = kType;
  dST[dSTptr].typ  = tyChar;
  dTT[tyChar].size = 1;

  dST[++dSTptr].id = ssl_add_id("string",pIdent);
  dST[dSTptr].kind = kType;
  dST[dSTptr].typ  = tyString;
  dTT[tyString].size = 128;  /* words */

  dST[++dSTptr].id = ssl_add_id("file",pIdent);
  dST[dSTptr].kind = kType;
  dST[dSTptr].typ  = tyFile;
  dTT[tyFile].size = 1;
#endif

  dTTptr = tyFile;
  dLastScalar = tyFile;

  /* built-in constants */

#if 0
  dST[++dSTptr].id = ssl_add_id("true",pIdent);
  dST[dSTptr].kind = kConst;
  dST[dSTptr].typ  = tyBoolean;
  dST[dSTptr].val  = 1;

  dST[++dSTptr].id = ssl_add_id("false",pIdent);
  dST[dSTptr].kind = kConst;
  dST[dSTptr].typ  = tyBoolean;
  dST[dSTptr].val  = 0;
#endif

}


// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

NODE_PTR dNode;  // temporary for several mechanisms


#include "ssl_begin.h"

    // built-in Emit operation is handled by application

    case oEmit :
            if (w_here >= w_outTableSize) {
              ssl_fatal( "output table overflow" );
            }

            dTemp = ssl_code_table[ssl_pc++];
            switch (dTemp) {
              case tSpace :      w_outTable[w_here++] = 0;
                                 continue;
              case tSymVal :     w_outTable[w_here++] = dST[dSym].val;
                                 continue;
              default :          w_outTable[w_here++] = dTemp;
                                 continue;
            }


    /* Mechanism node_mech */

    case oNodeNew:      /* (node_type) -- create node */
            ssl_result = (long) nodeNew (ssl_param);
            continue;
    case oNodeSet:
            nodeLink ((NODE_PTR)ssl_argv(0,3), ssl_argv(1,3), (NODE_PTR)ssl_argv(2,3));
            continue;
    case oNodeSetInt:
    case oNodeSetBoolean:
    case oNodeSetKind:
            nodeSetValue ((NODE_PTR)ssl_argv(0,3), ssl_argv(1,3), ssl_argv(2,3));
            continue;
    case oNodeGet:
            ssl_result = (long) nodeGet ((NODE_PTR)ssl_argv(0,2), ssl_argv(1,2));
            continue;
    case oNodeGetInt:
    case oNodeGetBoolean:
            ssl_result = (long) nodeGetValue ((NODE_PTR)ssl_argv(0,2), ssl_argv(1,2));
            continue;
    case oNodeNull:
            ssl_result = ((NODE_PTR)ssl_param == NULL);
            continue;
    case oNodeNext:
            ssl_var_stack[ssl_param] = (long) nodeNext((NODE_PTR)ssl_var_stack[ssl_param]);
            continue;
    case oNodeType:
            // SARICE TEST: allow for Null, returning type nINVALID.
            //   Maybe nodeType should allow this.
            if ((NODE_PTR)ssl_param == NULL) {
              ssl_result = nINVALID;
            } else {
              ssl_result = nodeType((NODE_PTR)ssl_param);
            }
            continue;
    case oNodeEqual:
            ssl_result = (ssl_argv(0,2) == ssl_argv(1,2));
            continue;


    /* Mechanism node_vec_mech */

    case oNodeVecNew: {
            NODE_VEC_PTR nv = (NODE_VEC_PTR) malloc( sizeof( NodeVec ) );
            nv->size = 0;
            nv->capacity = 4;
            nv->elements = (NODE_PTR*) malloc( nv->capacity * sizeof( NODE_PTR ) );
            memset( nv->elements, nv->capacity * sizeof( NODE_PTR ), 0 );
            ssl_result = (long) nv;
            continue;
            }
    case oNodeVecDelete: {
            NODE_VEC_PTR nv = (NODE_VEC_PTR) ssl_param;
            free( nv->elements );
            free( nv );
            continue;
            }
    case oNodeVecAppend: {
            NODE_VEC_PTR nv = (NODE_VEC_PTR) ssl_argv(0,2);
            NODE_PTR n = (NODE_PTR) ssl_argv(1,2);
            if ( nv->size == nv->capacity ) {
              int new_capacity = nv->capacity * 2;
              nv->elements = realloc( nv->elements, new_capacity * sizeof( NODE_PTR ) );
              memset( &(nv->elements[nv->capacity]),
                      (new_capacity - nv->capacity) * sizeof( NODE_PTR ),
                      0 );
              nv->capacity = new_capacity;
            }
            nv->elements[nv->size++] = n;
            continue;
            }
    case oNodeVecSize: {
            NODE_VEC_PTR nv = (NODE_VEC_PTR) ssl_param;
            ssl_result = nv->size;
            continue;
            }
    case oNodeVecElement: {
            NODE_VEC_PTR nv = (NODE_VEC_PTR) ssl_argv(0,2);
            ssl_result = (long) nv->elements[ ssl_argv(1,2) ];
            continue;
            }


    /* Mechanism emit_mech */

    case oEmitInt :
            w_outTable[w_here++] = ssl_param;
            continue;
    case Here :
            ssl_result = w_here;
            continue;
    case oPatch :
            w_outTable[ssl_argv(0,2)] = ssl_argv(1,2);
            continue;


    /* Mechanism math */

    case inc :
            ssl_var_stack[ssl_param]++;
            continue;
    case dec :
            ssl_var_stack[ssl_param]--;
            continue;
    case negate :
            ssl_result = -ssl_param;
            continue;
    case subtract :
            ssl_result = (ssl_argv(0,2) - ssl_argv(1,2));
            continue;
    case multiply :
            ssl_result = (ssl_argv(0,2) * ssl_argv(1,2));
            continue;
    case equal :
            ssl_result = (ssl_argv(0,2) == ssl_argv(1,2));
            continue;
    case equal_zero :
            ssl_result = (ssl_param == 0);
            continue;
    case equal_node_type :
            ssl_result = (ssl_argv(0,2) == ssl_argv(1,2));
            continue;


    /* Mechanism more_builtins */

    case TOKEN_VALUE :
            ssl_result = ssl_token.val;
            continue;
    case LAST_ID :
            ssl_result = ssl_last_id;
            continue;


    /* Mechanism scope_mech */

    case oScopeBegin:
            dNode = nodeNew (nScope);
            if (dScopeStackPtr > 0)
                nodeLink (dNode, qParentScope, dScopeStack[dScopeStackPtr]);
            if (++dScopeStackPtr == dScopeStackSize) ssl_fatal ("Scope Stack overflow");
            dScopeStack[dScopeStackPtr] = dNode;
            continue;
    case oScopeEnd:
            ssl_assert (dScopeStackPtr >= 1);
            dScopeStackPtr--;
            continue;
    case oScopeCurrent:
            ssl_assert (dScopeStackPtr >= 1);
            ssl_result = (long) dScopeStack[dScopeStackPtr];
            continue;
    case oScopeDeclare:
            ssl_assert (dScopeStackPtr >= 1);
            nodeAppend (dScopeStack[dScopeStackPtr], qDecls, (NODE_PTR)ssl_param);
            continue;
    case oScopeDeclareAlloc: {
            ssl_assert (dScopeStackPtr >= 1);
            NODE_PTR scope = dScopeStack[dScopeStackPtr];
            NODE_PTR node = (NODE_PTR)ssl_param;
            NODE_PTR theType = nodeGet( node, qType );
            ssl_assert( theType != NULL );
            long offset = nodeGetValue( scope, qNextOffset );
            nodeSetValue( node, qValue, offset );
            nodeSetValue( scope, qNextOffset, offset + nodeGetValue( theType, qSize ) );
            nodeAppend (scope, qDecls, node);
            continue;
            }
    case oScopeFind:
            for (int dScopeStackLookup = dScopeStackPtr; dScopeStackLookup > 0; dScopeStackLookup--)
            {  
                dNode = nodeFindValue_NoErrorChecking (dScopeStack[dScopeStackLookup], qDecls, qIdent, ssl_last_id);
                if (dNode != NULL)
                    break;
            }
            ssl_result = (long) dNode;
            continue;
    case oScopeFindRequire:
            for (int dScopeStackLookup = dScopeStackPtr; dScopeStackLookup > 0; dScopeStackLookup--)
            {  
                dNode = nodeFindValue_NoErrorChecking (dScopeStack[dScopeStackLookup], qDecls, qIdent, ssl_last_id);
                if (dNode != NULL)
                    break;
            }  
            ssl_result = (long) dNode;
            if (dNode == NULL)
            {
                ssl_error ("Undefined symbol");
                ssl_pc--;
                ssl_error_recovery ();
            }
            continue;


    /* Mechanism type_mech */

    case oTypeAdd:
            if (++dTypeTablePtr==dTypeTableSize) ssl_fatal("Type Table overflow");
            dTypeTable[dTypeTablePtr] = (NODE_PTR) ssl_param;
            continue;


    /* Mechanism type_stack_mech */

    case oTypeSPush:
            if (++dTypeStackPtr==dTypeStackSize) ssl_fatal("Type Stack overflow");
            dTypeStack[dTypeStackPtr] = (NODE_PTR) ssl_param;
            continue;
    case oTypeSPop:
            ssl_assert( dTypeStackPtr > 0 );
            --dTypeStackPtr;
            continue;
    case oTypeSTop:
            ssl_assert( dTypeStackPtr > 0 );
            ssl_result = (long) dTypeStack[dTypeStackPtr];
            continue;
    case oTypeSNodeType: {
            ssl_assert( dTypeStackPtr > 0 );
            NODE_PTR t = dTypeStack[dTypeStackPtr];
            // Skip past subranges, to the underlying base type.
            // If a caller wants the raw type including subranges, they can examine
            // the top node themselves using oTypeSTop.
            while ( nodeType(t) == nSubrangeType ) {
              t = nodeGet( t, qBaseType );
            }
            ssl_result = nodeType( t );
            continue;
            }


    /* Mechanism id_mech */

    case oIdAdd_File:
            ssl_result = ssl_add_id( "file", pIdent );
            continue;
    case oIdAdd_Integer:
            ssl_result = ssl_add_id( "integer", pIdent );
            continue;
    case oIdAdd_Boolean:
            ssl_result = ssl_add_id( "boolean", pIdent );
            continue;
    case oIdAdd_Char:
            ssl_result = ssl_add_id( "char", pIdent );
            continue;
    case oIdAdd_String:
            ssl_result = ssl_add_id( "string", pIdent );
            continue;
    case oIdAdd_True:
            ssl_result = ssl_add_id( "true", pIdent );
            continue;
    case oIdAdd_False:
            ssl_result = ssl_add_id( "false", pIdent );
            continue;


     /* Mechanism count */

     case oCountPush :
            if (++dCSptr==dCSsize) ssl_fatal("CS overflow");
            dCS[dCSptr] = ssl_param;
            continue;
     case oCountInc :
            dCS[dCSptr]++;
            continue;
     case oCountDec :
            dCS[dCSptr]--;
            continue;
     case oCountIsZero :
            ssl_result = !dCS[dCSptr];
            continue;
     case oCountPop :
            dCSptr--;
            continue;

     /* Mechanism value */

     case oValuePushToken :
            if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
            dVS[dVSptr] = ssl_token.val;
            continue;
     case oValuePushVal :
            if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
            dVS[dVSptr] = dST[dSym].val;
            continue;
     case oValuePush :
            if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
            dVS[dVSptr] = ssl_param;
            continue;
     case oValuePushSizeTS :
            if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
            dVS[dVSptr] = dTT[dTS[dTSptr]].size;
            continue;
     case oValuePushLowTS :
            if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
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
            ssl_result = !dVS[dVSptr];
            continue;
     case oValueIsOne :
            ssl_result = (dVS[dVSptr] == 1);
            continue;
     case oValueTop:
            ssl_result = dVS[dVSptr];
            continue;
     case oValuePop :
            dVSptr--;
            continue;

     /* Mechanism string */

     case oStringAllocLit :
            dWords = (ssl_token.namelen + 2) / 2;  /* #words for string */
            if (dSLptr + dWords + 2 >= dSLsize) ssl_fatal("SL overflow");
            /* push string addr (global data ptr) on value table */
            if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
            dVS[dVSptr] = dSD[0].allocOffset;
            /* save in strlit table: <addr> <words> <data> */
            dSL[dSLptr++] = dSD[0].allocOffset;
            dSL[dSLptr++] = dWords;
            dPtr = (short *) ssl_strlit_buffer;
            for (dTemp = 0; dTemp < dWords; dTemp++)
              dSL[dSLptr++] = *dPtr++;
            /* advance global data ptr */
            dSD[0].allocOffset += dWords;
            continue;

     /* Mechanism typ */

     case oTypSPushTyp :
            if (++dTSptr==dTSsize) ssl_fatal("TS overflow");
            dTS[dTSptr] = dST[dSym].typ;
            continue;
     case oTypSPush :
            if (++dTSptr==dTSsize) ssl_fatal("TS overflow");
            dTS[dTSptr] = ssl_param;
            continue;
     case oTypSPopPushBase :
            dTS[dTSptr] = dTT[dTS[dTSptr]].base;
            continue;
     case oTypSPopPushPtr :
            dTS[dTSptr] = dTT[dTS[dTSptr]].ptrType;
            continue;
     case oTypSMatch :     /* actually, should follow to roots of types */
            ssl_result = (dTS[dTSptr] == dTS[dTSptr-1]);
            dTSptr--;
            continue;
     case oTypSChoose :
            ssl_result = dTS[dTSptr];
            continue;
     case oTypSChoosePop :
            ssl_result = dTS[dTSptr--];
            continue;
     case oTypSChooseKind :
            ssl_result = dTT[dTS[dTSptr]].kind;
            continue;
     case oTypSChoosePtr :
            ssl_result = dTT[dTS[dTSptr]].ptrType;
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
            if (++dTTptr==dTTsize) ssl_fatal("TT overflow");
            if (++dTSptr==dTSsize) ssl_fatal("TS overflow");
            dTT[dTTptr].kind = ssl_param;
            dTT[dTTptr].ptrType = tyNone; /* no ptr to this type defined */
            dTS[dTSptr] = dTTptr;
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
            if (++dPS[ssl_param].ptr==dPS[ssl_param].size)
                  ssl_fatal("patch overflow");
            dPS[ssl_param].stack[dPS[ssl_param].ptr] = w_here;
            continue;
     case oPatchAnyEntries :
            ssl_result = dPS[ssl_param].ptr>0;
            continue;
     case oPatchSwap :
            dPtr = &dPS[ssl_param].stack[dPS[ssl_param].ptr-1];
            dTemp = dPtr[0];
            dPtr[0] = dPtr[1];
            dPtr[1] = dTemp;
            continue;
     case oPatchDup :
            dPtr = &dPS[ssl_param].stack[dPS[ssl_param].ptr++];
            dPtr[1] = dPtr[0];
            continue;
     case oPatchPopFwd :
            dTemp = dPS[ssl_param].stack[dPS[ssl_param].ptr--];
            w_outTable[dTemp] = w_here;
            continue;
     case oPatchPopBack :
            w_outTable[w_here] = dPS[ssl_param].stack[dPS[ssl_param].ptr--];
            w_here++;
            continue;

     /* Mechanism sym */

     case oSymPushLevel :
            if (++dSDptr==dSDsize) ssl_fatal("SD overflow");
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
            dSym = lookup(ssl_last_id);
            continue;
     case oSymChooseKind :
            ssl_result = dST[dSym].kind;
            continue;
     case oSymAddSPop :
            if (++dSTptr==dSTsize) ssl_fatal("ST overflow");
            dST[dSTptr] = dSS[dSSptr--];
            continue;
     case oSymLevelAnySyms :
            ssl_result = dSTptr >= dSD[dSDptr].sym;
            continue;
     case oSymSPushId :
            if (++dSSptr==dSSsize) ssl_fatal("SS overflow");
            dSS[dSSptr].id = ssl_last_id;
            /* perhaps clear other fields, for debugging */
            continue;
     case oSymSSetKind :
            dSS[dSSptr].kind = ssl_param;
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


#include "ssl_end.h"

// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------



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


#if 0
// ---------------------------------------------------------------------------------------
// Pre-2.0
// ---------------------------------------------------------------------------------------

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
  ssl_token.code = pColon;           /* dummy */
  ssl_token.accepted = 1;
  for (i=0; *t_kwTable[i].name; i++)
    (void) t_addId(t_kwTable[i].name,t_kwTable[i].code);
}  


void
t_getNextToken()
{
  char *p;
  short lit;

  ssl_token.accepted = 0;

  /* error if we try to read past the end of file */
  if (ssl_token.code == pEof)              /* last token was eof */
    ssl_fatal ("Unexpected end of file"); 

  /* skip to start of token */

  while (1) {
    while (t_blank[*t_ptr])
      t_ptr++;
    if (!*t_ptr) {
      if (t_listing && !t_lineListed)
        fprintf(t_lst,"      %s",t_lineBuffer);
      if (!fgets(t_lineBuffer,t_lineSize,t_src))
            {  ssl_token.code = pEof; return;  }
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
                {  ssl_token.code = pEof; return;  }
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

  ssl_token.lineNumber = t_lineNumber;
  ssl_token.colNumber = t_ptr-t_lineBuffer;    /* from 0, for err msgs */
  p = ssl_token.name;
  if (t_idStart[*t_ptr]) {
    while (t_idCont[*t_ptr] && p-ssl_token.name < t_nameSize)
      *p++ = *t_ptr++;
    *p = '\0';
    ssl_token.namelen = p-ssl_token.name;
    ssl_token.code = pIdent;
    t_lookupId();
  } else if (t_digit[*t_ptr]) {
    lit = 0;
    while (t_digit[*t_ptr]) {
      lit = lit*10 - '0' + *t_ptr;
      *p++ = *t_ptr++;
    }
    *p = '\0';
    ssl_token.val = lit;
    ssl_token.code = pIntLit;
  } else if (*t_ptr=='\'') {
    p = ssl_strlit_buffer;
    t_ptr++;
    ssl_token.code = pStrLit;
    while (p-ssl_strlit_buffer < t_strSize) {
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
    ssl_token.namelen = p-ssl_strlit_buffer;
  } else if (*t_ptr==':') {
    if (*++t_ptr=='=') {
      t_ptr++;
      ssl_token.code = pAssign;
    } else {
      ssl_token.code = pColon;
    }
  } else if (*t_ptr=='<') {
    if (*++t_ptr=='>') {
      t_ptr++;
      ssl_token.code = pNotEqual;
    } else if (*t_ptr=='=') {
      t_ptr++;
      ssl_token.code = pLessEqual;
    } else {
      ssl_token.code = pLess;
    }
  } else if (*t_ptr=='>') {
    if (*++t_ptr=='=') {
      t_ptr++;
      ssl_token.code = pGreaterEqual;
    } else {
      ssl_token.code = pGreater;
    }
  } else if (*t_ptr=='.') {
    if (*++t_ptr=='.') {
      t_ptr++;
      ssl_token.code = pDotDot;
    } else {
      ssl_token.code = pDot;
    }
  } else {
    ssl_token.code = t_punct[*t_ptr++];
    if (ssl_token.code == pInvalid) {
      --t_ptr;
      ssl_token.name[0] = *t_ptr++;
      ssl_token.name[1] = '\0';
      ssl_error ("Invalid character");
    }
  }
}


int
t_hitBreak()
{
  printf("Breaking...\n");
  t_cleanup();
  return(1);
}

#endif



void
open_my_files()
{
  if((t_out=fopen("a.out","w"))==NULL) {
    printf("Can't open output file a.out\n");
    exit(10);
  }
  if((t_doc=fopen("a.doc","w"))==NULL) {
    printf("Can't open output file a.doc\n");
    exit(10);
  }
  if (t_listing) {
    if((t_lst=fopen("a.lst","w"))==NULL) {
      printf("Can't open listing file a.lst\n");
      exit(10);
    }
  }
}


void
close_my_files()
{
  // In other ssl programs this is done via a mechanism in the ssl source
  t_dumpTables();

  fclose(t_out);
  fclose(t_doc);
  if (t_listing) fclose(t_lst);
}



void
t_printToken()
{
  printf(" (Token=");
  fprintf(t_doc," (Token=");
  if (ssl_token.code==pIdent) {
    printf("%s)\n",ssl_token.name);
    fprintf(t_doc,"%s)\n",ssl_token.name);
  } else if (ssl_token.code==pStrLit) {
    printf("'%s')\n",ssl_strlit_buffer);
    fprintf(t_doc,"'%s')\n",ssl_strlit_buffer);
  } else if (ssl_token.code==pInvalid) {
    printf("'%s')\n",ssl_token.name);
    fprintf(t_doc,"'%s')\n",ssl_token.name);
  } else {
    printf("<%d>)\n",ssl_token.code);
    fprintf(t_doc,"<%d>)\n",ssl_token.code);
  }
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

