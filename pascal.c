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

// Optionally integrate SSL debugger
#ifdef INTEGRATE_SSL_DEBUGGER
#include "debug.h"
#define DEBUG_INFO_FILE      "pascal.dbg"
#define PROGRAM_LISTING_FILE "pascal.lst"
#endif

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
int   w_outTable[w_outTableSize];  /* Emit into this table */
int   w_here;                      /* Output position */

/*************** Variables for semantic mechanisms ***************/


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
int dCS[dCSsize], dCSptr;        /* count stack */

#define dVSsize 30
int dVS[dVSsize], dVSptr;        /* value stack */

#define dSLsize 400
int dSL[dSLsize], dSLptr;        /* string literal table */


// A dynamic vector of NODE_PTR's
typedef struct NodeVecStruct {
  int size;
  int capacity;
  NODE_PTR*  elements;
} NodeVec, *NODE_VEC_PTR;

int labelCount;

// loop stack
#define dLSsize 100
struct {
    int continueLabel;
    int breakLabel;
} dLS[dLSsize];
int dLSptr = 0;


// Integration with SSL debugger

int optionDebug = 0;

#ifdef INTEGRATE_SSL_DEBUGGER

void
dump_int( void* variable, void* udata )
{
  printf( "\t%d\n", *(int*)variable );        
}

void
dump_node_short( void* variable, void* udata )
{
  NODE_PTR node_ptr = *((NODE_PTR*)variable);
  // double-dereference, because debugger calls us with a pointer to the variable,
  // not the value of the variable.
  nodeDumpNodeShort( node_ptr );
}

void
dump_node_stack_short( void* variable, void* udata )
{
  int top = *(int*) udata;
  NODE_PTR* stack = (NODE_PTR*) variable;
  for ( int i = top; i > 0; --i ) {
    nodeDumpNodeShort( stack[i] );
  }
}

//
// NODE_PTR dTypeStack[dTypeStackSize];
// int dTypeStackPtr;


dbg_variables debug_variables[] = {
    /* "Name",     address,             udata,           function */

    "Here",        (char*) &w_here,       NULL,          dump_int,
    "TypeStack",   (char*) dTypeStack,    (char*) &dTypeStackPtr,    dump_node_stack_short,

    /*  Type displayers  */
    "Node",        DBG_TYPE_DISPLAY,      NULL,          dump_node_short,

    "",            NULL,                0,               NULL,
};

#endif // INTEGRATE_SSL_DEBUGGER

//
// ------------------------------------------------------------------------------------
//

void
usage()
{
  int hasDebugger = 0;
#ifdef INTEGRATE_SSL_DEBUGGER
  hasDebugger = 1;
#endif
  printf( "Usage:  pascal [-l]%s <file>\n", hasDebugger ? " [-d]" : "" );
  printf( "  -l : create listing\n" );
  if ( hasDebugger ) {
    printf( "  -d : run ssl debugger\n" );
  }
}


int
main( int argc, char* argv[] )
{
  int entries,temp;
  int arg;

  t_listing = 0;

  for ( arg = 1; ( arg < argc ) && ( argv[arg][0] == '-' ); ++arg ) {
    if ( strcmp( argv[arg], "-l" ) == 0 ) {
      t_listing = 1;
    } else if ( strcmp( argv[arg], "-d" ) == 0 ) {
      optionDebug = 1;
    } else {
      usage();
      exit( 1 );
    }
  }

  if ( arg >= argc ) {
    printf( "Missing filename\n" );
    usage();
    exit( 1 );
  }

  /* Prepare Files */

  ssl_init();
  init_my_scanner();

#ifdef INTEGRATE_SSL_DEBUGGER
  ssl_set_debug( optionDebug );
  ssl_set_debug_info( DEBUG_INFO_FILE, PROGRAM_LISTING_FILE, oBreak, debug_variables );
#endif

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
  "continue",           pContinue,
  "break",              pBreak,
  "return",             pReturn,
  "and",                pAnd,
  "or",                 pOr,
  "not",                pNot,
  "writeln",            pWriteln,
  "write",              pWrite,
  "readln",             pReadln,
  "read",               pRead,
  "forward",            pForward,
  "external",           pExternal,
  "name",               pName,
  "cdecl",              pCdecl,

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
  "@",                  pAt,
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

  labelCount = 0;

  // Note: global scope will be created by the ssl program,
  // as will predefined types and constants.
}


// Creates some global data that should appear in the output file.
// Given a pointer to the data to be stored, and the number of bytes.
// Returns the address of the data in the output file's global data space.
// 
int
createGlobalData( char* sourceData, int sourceBytes )
{
  NODE_PTR globalScope = dScopeStack[1];
  int numInts = (sourceBytes + 3) / 4;
  if (dSLptr + numInts + 2 >= dSLsize) ssl_fatal("SL overflow");

  int offset = nodeGetValue( globalScope, qSize );
  nodeSetValue( globalScope, qSize, offset + sourceBytes );

  // the data currenty resides in the output file's "strlit table"
  // with the format:
  //    <addr> <#ints> <int> <int> <int> ...

  dSL[dSLptr++] = offset;
  dSL[dSLptr++] = numInts;
  int* intPtr = (int*) sourceData;
  for (int i = 0; i < numInts; ++i) {
  dSL[dSLptr++] = *intPtr++;
  }

  return offset;
}


// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

NODE_PTR dNode;  // temporary for several mechanisms


#include "ssl_begin.h"

    // built-in Emit operation is handled by application

    case oEmit : {
            if (w_here >= w_outTableSize) {
              ssl_fatal( "output table overflow" );
            }

            int outToken = ssl_code_table[ssl_pc++];
            switch (outToken) {
              case tSpace :      w_outTable[w_here++] = 0;
                                 break;
              default :          w_outTable[w_here++] = outToken;
                                 break;
            }
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
    case oNodeSetLabel:
            nodeSetValue ((NODE_PTR)ssl_argv(0,3), ssl_argv(1,3), ssl_argv(2,3));
            continue;
    case oNodeGet:
            ssl_result = (long) nodeGet ((NODE_PTR)ssl_argv(0,2), ssl_argv(1,2));
            continue;
    case oNodeGetInt:
    case oNodeGetBoolean:
    case oNodeGetLabel:
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
    case oEmitLabel :
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
    case equal_label :
            ssl_result = (ssl_argv(0,2) == ssl_argv(1,2));
            continue;
    case oMININT :
            ssl_result = -2147483648;  // 0x80000000
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
    case oScopeAllocType: {
            ssl_assert (dScopeStackPtr >= 1);
            NODE_PTR scope = dScopeStack[dScopeStackPtr];
            NODE_PTR theType = (NODE_PTR)ssl_param;
            ssl_assert( theType != NULL );
            int size = nodeGetValue( theType, qSize );
            long offset = nodeGetValue( scope, qSize );
            if ( nodeGetValue( scope, qAllocDown ) ) {
              ssl_result = -offset - size;
              nodeSetValue( scope, qSize, offset + size );
            } else {
              ssl_result = offset;
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
    case oScopeFindInCurrentScope: {
            dNode = nodeFindValue_NoErrorChecking (dScopeStack[dScopeStackPtr], qDecls, qIdent, ssl_last_id);
            ssl_result = (long) dNode;
            continue;
            }

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


     /* Mechanism label_mech */

     case oLabelNew :
            ssl_result = ++labelCount;
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

            char* sourceStr = ssl_strlit_buffer;
            int sourceLen = ssl_token.namelen + 1;  // +1 for '\0'
            ssl_result = createGlobalData( sourceStr, sourceLen );
            continue;
            }

     case oStringAllocLitFromIdent : {
            char* sourceStr = ssl_get_id_string( ssl_param );
            int sourceLen = strlen( sourceStr) + 1;  // +1 for '\0'
            ssl_result = createGlobalData( sourceStr, sourceLen );
            continue;
            }



     /* Mechanism loop */

     case oLoopPush :
            if ( ++dLSptr == dLSsize ) {
                ssl_fatal("loop stack overflow");
            }
            dLS[dLSptr].continueLabel = ssl_argv(0, 2);
            dLS[dLSptr].breakLabel = ssl_argv(1, 2);
            continue;
     case oLoopContinueLabel:
            if ( dLSptr > 0 ) {
                ssl_result = dLS[dLSptr].continueLabel;
            } else {
                ssl_result = 0;
            }
            continue;
     case oLoopBreakLabel:
            if ( dLSptr > 0 ) {
                ssl_result = dLS[dLSptr].breakLabel;
            } else {
                ssl_result = 0;
            }
            continue;
     case oLoopPop:
            --dLSptr;
            continue;


     /* Mechanism msg_mech */

     case oMsg :
            printf( "oMsg %ld\n", ssl_param );
            continue;
     case oMsgTrace :
            printf( "oMsg %ld\n", ssl_param );
            ssl_warning( "  oMsg occurred" );
            ssl_traceback();
            // ssl_print_input_position();
            continue;
     case oMsgNode :
            nodeDumpNodeShort( (NODE_PTR) ssl_param );
            continue;
     case oMsgNodeVec : {
            NODE_VEC_PTR nv = (NODE_VEC_PTR) ssl_param;
            printf( "NodeVec: size %d:\n", nv->size );
            for ( int i = 0; i < nv->size; ++i ) {
              NODE_PTR node = nv->elements[i];
              nodeDumpNodeShort( node );
            }
            printf( "----\n" );
            }  
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
    fprintf(t_out,"%6d ",w_outTable[i]);
    if ((++col % 10) == 0)
      fprintf(t_out,"\n");
  }

  /* string literals table */
  for (i=0; i<dSLptr; i++) {
    fprintf(t_out,"%6d ",dSL[i]);
    if ((++col % 10) == 0)
      fprintf(t_out,"\n");
  }
}

