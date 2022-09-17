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


/* The scanner is part of ssl_rt 2.0
   with configuration done here to (ideally) match pascal requirements.
   It's still not 100% correct though.

   Scanner variables start with 't_'.
   Table walker variables start with 'w_'.
   Semantic operations start with 'o'.
   Corresponding data structures start with 'd'.

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
char   t_lineBuffer[t_lineSize];
char   t_listing;                  /* do we want a listing file? */


// Forward declarations
void open_my_files();
void close_my_files();
void init_my_scanner();
void init_my_operations();

void t_printToken();
void t_dumpTables();


#define w_outTableSize 5000
short w_outTable[w_outTableSize];  /* Emit into this table */
short w_here;                      /* Output position */

/*************** Variables for semantic mechanisms ***************/

short dTemp;                       /* multi-purpose */
short *dPtr;                       /* multi-purpose */
short dWords;                      /* multi-purpose */


// scope stack
#define dScopeStackSize 40
NODE_PTR        dScopeStack[dScopeStackSize];
int             dScopeStackPtr;


// Type table, owns nType nodes.
// TO DO: make all of these tables dynamic size.
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

  // Note: global scope will be created by the ssl program,
  // as will predefined types and constants.
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
            if (++dScopeStackPtr == dScopeStackSize) ssl_fatal ("Scope Stack overflow");
            dScopeStack[dScopeStackPtr] = dNode;
            continue;
    case oScopeEnter:
            if (++dScopeStackPtr == dScopeStackSize) ssl_fatal ("Scope Stack overflow");
            dScopeStack[dScopeStackPtr] = (NODE_PTR) ssl_param;
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
            nodeAppend (scope, qDecls, node);
            NODE_PTR theType = nodeGet( node, qType );
            ssl_assert( theType != NULL );
            int size = nodeGetValue( theType, qSize );
            long offset = nodeGetValue( scope, qSize );
            if ( nodeGetValue( scope, qAllocDown ) ) {
              nodeSetValue( node, qValue, -offset - size );
              nodeSetValue( scope, qSize, offset + size );
            } else {
              nodeSetValue( node, qValue, offset );
              nodeSetValue( scope, qSize, offset + size );
            }
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

     case oValuePush :
            if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
            dVS[dVSptr] = ssl_param;
            continue;
     case oValueNegate :
            dVS[dVSptr] *= -1;
            continue;
     case oValueTop:
            ssl_result = dVS[dVSptr];
            continue;
     case oValuePop :
            dVSptr--;
            continue;

     /* Mechanism string */

     case oStringAllocLit : {
            // Current approach is: space for the string literal is reserved in the global scope.
            // And separately, the literal is recorded in a string table, which will be written
            // to the output file after all the code.
            // The written string table says where each literal should be installed in the data space.
            //
            // A more conventional approach would have the output file contain separate records for
            // initialized static data, and blank (0-initialized) static data.
            // To do that, though, I'd also need to patch all the data references in the code,
            // since I don't know on the first pass how big each section will be.
            // And with multiple output object files, relocation tables would need to be resolved by
            // a linker.   Leaving all that aside for now.

            NODE_PTR globalScope = dScopeStack[1];

            dWords = (ssl_token.namelen + 2) / 2;  /* #words for string */
            if (dSLptr + dWords + 2 >= dSLsize) ssl_fatal("SL overflow");

            int offset = nodeGetValue( globalScope, qSize );
            nodeSetValue( globalScope, qSize, offset + dWords );

            /* push string addr (global data ptr) on value stack */
            if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
            dVS[dVSptr] = offset;

            /* save in strlit table: <addr> <words> <data> */
            dSL[dSLptr++] = offset;
            dSL[dSLptr++] = dWords;
            dPtr = (short *) ssl_strlit_buffer;
            for (dTemp = 0; dTemp < dWords; dTemp++)
              dSL[dSLptr++] = *dPtr++;
            continue;
            }


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


#include "ssl_end.h"

// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------


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

