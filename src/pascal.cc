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

#include <string>
#include <vector>
#include <unordered_map>


// This part of the toolchain is still in C
extern "C" {
// SSL Runtime module definitions
#include "ssl_rt.h"

// Schema database access functions
#include "schema_db.h"

// Optionally integrate SSL debugger
#ifdef INTEGRATE_SSL_DEBUGGER
#include "debug.h"
#define DEBUG_INFO_FILE      "pascal.dbg"
#define PROGRAM_LISTING_FILE "pascal.lst"
#endif

}  // extern "C"


// SSL definitions generated for this program
#define  SSL_INCLUDE_ERR_TABLE
#include "pascal.h"

// SSL code generated for this program
#include "pascal.tbl"

#include "tcode.h"

/* The scanner is part of ssl_rt 2.0
   with configuration done here to (ideally) match pascal requirements.
   It's still not 100% correct though.

   Scanner variables start with 't_'.
   Table walker variables start with 'w_'.
   Semantic operations start with 'o'.
   Corresponding data structures start with 'd'.

   A token is read and screened when oInput, oInputAny, or oInputChoice
   is encountered.  When an id is accepted, set ssl_last_id to its id #.

   The 'code' of a token is pComma, pIntLit, pIdent, etc.
   The 'id #' of an identifier is the unique # for that name (actually
   the index into the scanner's identifier table).

   The scanner maintains an identifier table, which is used to find
   keywords, and to translate names into integers.  Whenever a new
   identifier is found, it is added to the table.  It is separate from
   the symbol table constructed by the compiler's semantic operations.

*/

FILE *t_out,*t_doc,*t_lst;

#define t_lineSize 256
char   t_lineBuffer[t_lineSize];
int    optionList;                  /* do we want a listing file? */
int    listing_prev_pc = 0;

std::vector<std::string> search_path;

// Forward declarations
void open_my_files();
void close_my_files();
void init_my_scanner();
void init_my_operations();
void my_listing_function( char* source_line, int token_accepted );

void t_printToken();
void t_dumpTables();


// A t-code output stream.
// We have one main 'default' stream, and other temporary ones that are copied into the default stream.
typedef std::vector<int> CodeStream;
// Schema doesn't support client pointers yet, so the SSL/schema refers to code streams by integer id,
// thanks to this table.  id 0 is unused; id 1 is the default stream.
std::vector<CodeStream*> codeStreams;
// We can temporarily change the curernt code stream
std::vector<int> codeStreamStack;
CodeStream* currentCodeStream = nullptr;


/*************** Variables for semantic mechanisms ***************/


// workspace
Node            workspace = nullptr;

std::vector<Node> dScopeStack;

// Type table, owns nType nodes.
std::vector<Node> dTypeTable;

// Type stack, references nType nodes involved in current declaration or expression.
std::vector<Node> dTypeStack;

// Value stack
// This is mainly used for constant expressions
//
struct ValueEntry
{
  ValueEntry( int i )
    : _int( i )
  {}
  ValueEntry( double d )
    : _double( d ), _isDouble( true )
  {}
  ValueEntry( const char* str )
    : _string( str ), _isString( true )
  {}
  int   _int = 0;
  double _double = 0;
  std::string _string;
  bool  _isString = false;
  bool  _isDouble = false;
};

class ValueStack : public std::vector<ValueEntry>
{
public:
  ValueEntry& top() { return (*this)[ this->size()-1 ]; }
  ValueEntry& down1() { return (*this)[ this->size()-2 ]; }
  void pop() { pop_back(); }
};
ValueStack valueStack;

// string literals and other global data
std::vector<int> dSL;


// A dynamic vector of Node's
typedef struct NodeVecStruct {
  int size;
  int capacity;
  Node*  elements;
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
int optionVerboseErrors = 0;

#ifdef INTEGRATE_SSL_DEBUGGER

void
dump_int( void* variable, void* udata )
{
  printf( "\t%d\n", *(int*)variable );        
}

void
dump_node_short( void* variable, void* udata )
{
  Node node_ptr = *((Node*)variable);
  // double-dereference, because debugger calls us with a pointer to the variable,
  // not the value of the variable.
  DumpNodeShort( node_ptr );
}

void
dump_node_stack_short( void* variable, void* udata )
{
  auto ts = ( std::vector<Node>* )( variable );
  for ( int i = (*ts).size() - 1; i >= 0; --i ) {
    DumpNodeShort( (*ts)[i] );
  }
}



dbg_variables debug_variables[] = {
    /* "Name",     address,             udata,           function */

    "TypeStack",   (char*) &dTypeStack,   NULL,          dump_node_stack_short,

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
  printf( "Usage:  pascal [-l] [-I<dir>]%s <file>\n", hasDebugger ? " [-d]" : "" );
  printf( "  -l : create listing\n" );
  printf( "  -I<dir> : add directory to unit search path\n" );
  printf( "  -v : verbose error messages, giving internal details about errors\n" );
  if ( hasDebugger ) {
    printf( "  -d : run ssl debugger\n" );
  }
}


int
main( int argc, char* argv[] )
{
  int entries,temp;
  int arg;

  optionList = 0;
  search_path.push_back( "." );

  for ( arg = 1; ( arg < argc ) && ( argv[arg][0] == '-' ); ++arg ) {
    if ( strcmp( argv[arg], "-l" ) == 0 ) {
      optionList = 1;
    } else if ( strcmp( argv[arg], "-d" ) == 0 ) {
      optionDebug = 1;
    } else if ( strcmp( argv[arg], "-v" ) == 0 ) {
      optionVerboseErrors = 1;
    } else if ( strncmp( argv[arg], "-I", 2 ) == 0 ) {
      char* dir = argv[arg] + 2;
      if ( strlen( dir ) > 0 ) {
        search_path.push_back( argv[arg] + 2 );
      }
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
  ssl_set_verbose_errors( optionVerboseErrors );

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

  if ( optionList ) {
    ssl_set_listing_callback( my_listing_function );
  }

  ssl_set_init_operations_callback( init_my_operations );

#ifdef AMIGA
  onbreak(&t_hitBreak);
#endif // AMIGA


  // Execute SSL program
  int status = ssl_run_program();

  if ( optionList ) {
    // flush last bit of code after the last listed source line
    my_listing_function( (char*) "\n", false );
  }

  close_my_files();

  SCH_Term();
  
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
  "label",              pLabel,
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
  "case",               pCase,
  "otherwise",          pOtherwise,
  "goto",               pGoto,
  "and",                pAnd,
  "or",                 pOr,
  "not",                pNot,
  "uses",               pUses,
  "unit",               pUnit,
  "interface",          pInterface,
  "implementation",     pImplementation,
  "initialization",     pInitialization,
  "finalization",       pFinalization,
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
  ssl_set_code_charlit( pCharLit );
  ssl_enable_pascal_char_codes( 1 );
  ssl_set_code_double_lit( pDoubleLit );

  ssl_scanner_init_comment( "(*", "*)" );  // Original pascal comments
  ssl_scanner_init_comment( "{", "}" );    // Introduced by Turbo Pascal
  ssl_scanner_init_comment( "//", "" );    // to end of line. Delphi pascal.

  // I'm using the scanner's include mechanism to read unit files.
  // But I'll control when the including should stop, e.g. in case a unit file is malformed.
  ssl_end_include_at_eof( 0 );
}


/*  Callback when first token of each source line accepted.
 *  Used to list source file, intermixed with generated tcode.
 *
 *  Line supplied to us contains '\n' for now.
 */

void
my_listing_function( char* source_line, int token_accepted )
{
  int     line_number;
  int     addr;

  // First, dump any tcode issued since the previous source line was printed.
  // We're only dumping the default code stream.  We'll probably incorporate temp streams into it
  // before this listing function is called, though.
  CodeStream* code = codeStreams[1];
  if ( code->size() != listing_prev_pc ) {
    fprintf( t_lst, "\n" );
    listing_prev_pc = tcodeDump( t_lst, code->data(), listing_prev_pc, (int) code->size() );
    fprintf( t_lst, "\n" );
  }

  bool print_address_on_source_lines = false;  // lean towards not doing this.  a bit noisy.

  if ( token_accepted && print_address_on_source_lines ) {
    fprintf ( t_lst, "%4d: %s", (int) code->size(), source_line );
  } else {
    /*  "blank" line  */
    fprintf ( t_lst, "      %s", source_line );
  }
}


/*********** S e m a n t i c   O p e r a t i o n s ***********/

void
init_my_operations()
{
  // Set up default code stream
  codeStreams.push_back( nullptr );
  codeStreams.push_back( new CodeStream() );

  codeStreamStack.push_back( 1 );
  currentCodeStream = codeStreams[1];

  // Initialize schema package (schema runtime module)
  SCH_Init();

  labelCount = 0;

  // Note: global scope will be created by the ssl program,
  // as will predefined types and constants.
}


// Allocate size bytes from the given scope.  Returns the address of the allocated memory.
//
long
scopeAlloc( Node scope, int size )
{
  long addr;
  long offset;
  int allocMode = GetValue( scope, qAllocMode );

  switch ( allocMode ) {
    case allocGlobal:
      offset = GetValue( workspace, qGlobalSize );
      addr = offset;
      SetValue( workspace, qGlobalSize, offset + size );
      break;
    case allocDown:
      offset = GetValue( scope, qSize );
      addr = -offset - size;
      SetValue( scope, qSize, offset + size );
      break;
    case allocUp:
      offset = GetValue( scope, qSize );
      addr = offset;
      SetValue( scope, qSize, offset + size );
      break;
  }
  return addr;
}


// Creates some global data that should appear in the output file.
// Given a pointer to the data to be stored, and the number of bytes.
// Returns the address of the data in the output file's global data space.
// 
int
createGlobalData( const char* sourceData, int sourceBytes )
{
  Node globalScope = dScopeStack.front();
  int numInts = (sourceBytes + 3) / 4;

  // allocating as ints, because the strlit data table assumes that (see just below)
  int offset = scopeAlloc( globalScope, numInts * sizeof( int ) );

  // the data currenty resides in the output file's "strlit table"
  // with the format:
  //    <addr> <#ints> <int> <int> <int> ...

  dSL.push_back( offset );
  dSL.push_back( numInts );
  const int* intPtr = (const int*) sourceData;
  for (int i = 0; i < numInts; ++i) {
    dSL.push_back( *intPtr++ );
  }

  return offset;
}


// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

Node dNode;  // temporary for several mechanisms


#include "ssl_begin.h"

    // built-in Emit operation is handled by application

    case oEmit : {
            int outToken = ssl_code_table[ssl_pc++];
            switch (outToken) {
              case tSpace :      currentCodeStream->push_back( 0 );
                                 break;
              default :          currentCodeStream->push_back( outToken );
                                 break;
            }
            continue;
            }


    /* Mechanism node_mech */

    case oNodeNew:      /* (node_type) -- create node */
            ssl_result = (long) NewNode (ssl_param);
            continue;
    case oNodeSet:
            SetAttr ((Node)ssl_argv(0,3), ssl_argv(1,3), (Node)ssl_argv(2,3));
            continue;
    case oNodeSetDouble: {
            long encodedValue = ssl_argv(2,3);
            SetAttrReal8 ((Node)ssl_argv(0,3), ssl_argv(1,3), *(double*)&encodedValue);
            continue;
            }
    case oNodeSetString:
            SetAttr ((Node)ssl_argv(0,3), ssl_argv(1,3), (void*)ssl_argv(2,3));
            continue;
    case oNodeSetInt:
    case oNodeSetBoolean:
    case oNodeSetLabel:
    case oNodeSetCode:
            SetValue ((Node)ssl_argv(0,3), ssl_argv(1,3), ssl_argv(2,3));
            continue;
    case oNodeAddLast:
            NodeAddLast((Node)ssl_argv(0,3), ssl_argv(1,3), (Node)ssl_argv(2,3));
            continue;
    case oNodeGet:
            ssl_result = (long) GetAttr ((Node)ssl_argv(0,2), ssl_argv(1,2));
            continue;
    case oNodeGetDouble: {
            double value = GetAttrReal8 ((Node)ssl_argv(0,2), ssl_argv(1,2));
            // return double value encoded as long
            ssl_result = *(long*) &value;
            continue;
            }
    case oNodeGetString:
            ssl_result = (long) GetAttr ((Node)ssl_argv(0,2), ssl_argv(1,2));
            continue;
    case oNodeGetInt:
    case oNodeGetBoolean:
    case oNodeGetLabel:
    case oNodeGetCode:
            ssl_result = (long) GetValue ((Node)ssl_argv(0,2), ssl_argv(1,2));
            continue;
    case oNodeNull:
            ssl_result = ((Node)ssl_param == NULL);
            continue;
    case oNodeFind: {
            Node N = (Node)ssl_argv(0,4);
            int nodeAttr = ssl_argv(1,4);
            int valueAttr = ssl_argv(2,4);
            long value = ssl_argv(3,4);
            ssl_result = (long) FindValue_NoErrorChecking (N, nodeAttr, valueAttr, value);
            continue;
            }
    case oNodeGetIter: {
            List list = (List) GetAttr((Node)ssl_argv(0,2), ssl_argv(1,2));
            if ( !list ) {
              ssl_result = 0;
            } else {
              ssl_result = (long) FirstItem( list );
            }
            continue;
            }
    case oNodeGetIterLast: {
            List list = (List) GetAttr((Node)ssl_argv(0,2), ssl_argv(1,2));
            if ( !list ) {
              ssl_result = 0;
            } else {
              ssl_result = (long) LastItem( list );
            }
            continue;
            }
    case oNodeIterValue: {
            Item item = (Item)ssl_param;
            if ( item == NULL ) {
              ssl_result = 0;
            } else {
              ssl_result = (long) Value( item );
            }
            continue;
            }
    case oNodeIterNext: {
            Item item = (Item) ssl_var_stack[ssl_param];
            if ( item != NULL ) {
              item = NextItem( item );
              ssl_var_stack[ssl_param] = (long) item;
            }
            continue;
            }
    case oNodeIterPrev: {
            Item item = (Item) ssl_var_stack[ssl_param];
            if ( item != NULL ) {
              item = PrevItem( item );
              ssl_var_stack[ssl_param] = (long) item;
            }
            continue;
            }
    case oNodeType:
            // SARICE TEST: allow for Null, returning type nINVALID.
            //   Maybe Kind should allow this.
            if ((Node)ssl_param == NULL) {
              ssl_result = nINVALID;
            } else {
              ssl_result = Kind((Node)ssl_param);
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
            nv->elements = (Node*) malloc( nv->capacity * sizeof( Node ) );
            memset( nv->elements, nv->capacity * sizeof( Node ), 0 );
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
            Node n = (Node) ssl_argv(1,2);
            if ( nv->size == nv->capacity ) {
              int new_capacity = nv->capacity * 2;
              nv->elements = (Node*) realloc( nv->elements, new_capacity * sizeof( Node ) );
              memset( &(nv->elements[nv->capacity]),
                      (new_capacity - nv->capacity) * sizeof( Node ),
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
            currentCodeStream->push_back( ssl_param );
            continue;
    case oEmitDouble :
            currentCodeStream->push_back( ssl_param & 0xffffffff );
            currentCodeStream->push_back( ssl_param >> 32 );
            continue;
    case oEmitLabel :
            currentCodeStream->push_back( ssl_param );
            continue;
    case oEmitCode : {
            CodeStream* code = codeStreams[ssl_param];
            if ( code ) {
              currentCodeStream->insert( currentCodeStream->end(),
                                         code->begin(), code->end() );
              // blank out the code stream, but don't delete it.
              // I'll leave that cleanup to the owner of codeStreams, for now.
              code->clear();
            }
            }
            continue;
    case Here :
            // This operation is only valid in the default code stream
            ssl_assert( currentCodeStream == codeStreams[1] );
            ssl_result = (*currentCodeStream).size();
            continue;
    case oPatch :
            // This operation is only valid in the default code stream
            ssl_assert( currentCodeStream == codeStreams[1] );
            (*currentCodeStream)[ssl_argv(0,2)] = ssl_argv(1,2);
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
    case add :
            ssl_result = ( ssl_argv( 0, 2 ) + ssl_argv( 1, 2 ) );
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
    case equal_node :
            ssl_result = (ssl_argv(0,2) == ssl_argv(1,2));
            continue;
    case equal_string : {
            const char* str1 = (const char*) ssl_argv(0,2);
            const char* str2 = (const char*) ssl_argv(1,2);
            ssl_result = ( str1 == nullptr && str2 == nullptr ) ||
                         ( str1 != nullptr && str2 != nullptr && strcmp( str1, str2 ) == 0 );
            continue;
            }
    case equal_node_type :
            ssl_result = (ssl_argv(0,2) == ssl_argv(1,2));
            continue;
    case equal_label :
            ssl_result = (ssl_argv(0,2) == ssl_argv(1,2));
            continue;
    case equal_code :
            ssl_result = (ssl_argv(0,2) == ssl_argv(1,2));
            continue;
    case greater :
            ssl_result = (ssl_argv(0,2) > ssl_argv(1,2));
            continue;
    case oMININT :
            ssl_result = -2147483648;  // 0x80000000
            continue;
    case oMAXINT :
            ssl_result = 2147483647;  // 0x7fffffff
            continue;

    /* Mechanism more_builtins */

    case TOKEN_VALUE :
            ssl_result = ssl_token.val;
            continue;
    case TOKEN_VALUE_DOUBLE : {
            double val = ssl_token.double_val;
            // return double value encoded as long
            ssl_result = *(long*) &val;
            }
            continue;
    case LAST_ID :
            ssl_result = ssl_last_id;
            continue;
    case ID_STRING :
            ssl_result = (long) ssl_get_id_string( ssl_param );
            continue;
    case CURRENT_STRLIT :
            ssl_result = (long) ssl_strlit_buffer;
            continue;
    case UNACCEPT_TOKEN :
            ssl_unaccept_token();
            continue;


    /* Mechanism workspace_mech */

    case oWorkspaceNew:
            workspace = NewNode (nWorkspace);
            ssl_result = (long) workspace;
            continue;


    /* Mechanism scope_mech */

    case oScopeBegin:
            dNode = NewNode (nScope);
            SetValue( dNode, qLevel, ssl_argv(0,2) );
            SetValue( dNode, qAllocMode, ssl_argv(1,2) );
            dScopeStack.push_back( dNode );
            continue;
    case oScopeEnter:
            dScopeStack.push_back( (Node) ssl_param );
            continue;
    case oScopeEnd:
            ssl_assert( !dScopeStack.empty() );
            dScopeStack.pop_back();
            continue;
    case oScopeCurrent:
            ssl_assert( !dScopeStack.empty() );
            ssl_result = (long) dScopeStack.back();
            continue;
    case oScopeDeclare: {
            ssl_assert( !dScopeStack.empty() );
            Node scope = dScopeStack.back();
            Node decl = (Node) ssl_param;
            SetAttr( decl, qParentScope, scope );
            NodeAddLast( scope, qDecls, decl );
            continue;
            }
    case oScopeDeclareAlloc: {
            ssl_assert( !dScopeStack.empty() );
            Node scope = dScopeStack.back();
            Node decl = (Node)ssl_param;
            SetAttr( decl, qParentScope, scope );
            NodeAddLast( scope, qDecls, decl );
            Node theType = (Node) GetAttr( decl, qType );
            ssl_assert( theType != NULL );
            int size = GetValue( theType, qSize );
            SetValue( decl, qValue, scopeAlloc( scope, size ) );
            continue;
            }
    case oScopeAllocType: {
            ssl_assert( !dScopeStack.empty() );
            Node scope = dScopeStack.back();
            Node theType = (Node)ssl_param;
            ssl_assert( theType != NULL );
            int size = GetValue( theType, qSize );
            ssl_result = scopeAlloc( scope, size );
            continue;
            }
    case oScopeAlloc: {
            ssl_assert( !dScopeStack.empty() );
            Node scope = dScopeStack.back();
            int size = ssl_argv( 0, 2 );
            // TO DO: use align i.e. ssl_argv( 1, 2 )
            ssl_result = scopeAlloc( scope, size );
            continue;
            }
    case oScopeFind:
            for (int dScopeStackLookup = dScopeStack.size() - 1; dScopeStackLookup >= 0; dScopeStackLookup--)
            {  
                dNode = FindValue_NoErrorChecking (dScopeStack[dScopeStackLookup], qDecls, qIdent, ssl_last_id);
                if (dNode != NULL)
                    break;
            }
            ssl_result = (long) dNode;
            continue;
    case oScopeFindRequire:
            for (int dScopeStackLookup = dScopeStack.size() - 1; dScopeStackLookup >= 0; dScopeStackLookup--)
            {  
                dNode = FindValue_NoErrorChecking (dScopeStack[dScopeStackLookup], qDecls, qIdent, ssl_last_id);
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
            Node scope = dScopeStack.back();
            dNode = FindValue_NoErrorChecking( scope, qDecls, qIdent, ssl_last_id );
            if ( !dNode ) {
              // Look in a scope we extend, too.  Needed to provide impl for procs in unit interface.
              Node extends = (Node) GetAttr( scope, qExtends );
              if ( extends) {
                dNode = FindValue_NoErrorChecking( extends, qDecls, qIdent, ssl_last_id );
              }
            }
            ssl_result = (long) dNode;
            continue;
            }
    case oScopeFindRequireInScope: {
            Node scope = (Node) ssl_param;
            dNode = FindValue_NoErrorChecking( scope, qDecls, qIdent, ssl_last_id );
            if ( !dNode ) {
              // Look in a scope we extend, too.
              Node extends = (Node) GetAttr( scope, qExtends );
              if ( extends) {
                dNode = FindValue_NoErrorChecking( extends, qDecls, qIdent, ssl_last_id );
              }
            }
            ssl_result = (long) dNode;
            if ( !dNode ) {
                ssl_error ("Undefined symbol");
                ssl_pc--;
                ssl_error_recovery ();
            }
            continue;
            }


    /* Mechanism type_mech */

    case oTypeAdd:
            dTypeTable.push_back( (Node) ssl_param );
            continue;


    /* Mechanism type_stack_mech */

    case oTypeSPush:
            dTypeStack.push_back( (Node) ssl_param );
            continue;
    case oTypeSPop:
            ssl_assert( !dTypeStack.empty() );
            dTypeStack.pop_back();
            continue;
    case oTypeSTop:
            ssl_assert( !dTypeStack.empty() );
            ssl_result = (long) dTypeStack.back();
            continue;
    case oTypeSNodeType: {
            ssl_assert( !dTypeStack.empty() );
            Node t = dTypeStack.back();
            // Skip past subranges, to the underlying base type.
            // If a caller wants the raw type including subranges, they can examine
            // the top node themselves using oTypeSTop.
            while ( Kind(t) == nSubrangeType ) {
              t = (Node) GetAttr( t, qBaseType );
            }
            ssl_result = Kind( t );
            continue;
            }


    /* Mechanism id_mech */

    case oId_mysystem:
            ssl_result = ssl_lookup_id( "mysystem", pIdent );
            continue;
    case oId_ShortStringAppendShortString:
            ssl_result = ssl_lookup_id( "ShortStringAppendShortString", pIdent );
            continue;
    case oId_ShortStringAppendChar:
            ssl_result = ssl_lookup_id( "ShortStringAppendChar", pIdent );
            continue;
    case oId_ShortStringCmp:
            ssl_result = ssl_lookup_id( "ShortStringCmp", pIdent );
            continue;
    case oId_File:
            ssl_result = ssl_lookup_id( "file", pIdent );
            continue;
    case oId_Integer:
            ssl_result = ssl_lookup_id( "integer", pIdent );
            continue;
    case oId_Boolean:
            ssl_result = ssl_lookup_id( "boolean", pIdent );
            continue;
    case oId_Char:
            ssl_result = ssl_lookup_id( "char", pIdent );
            continue;
    case oId_Byte:
            ssl_result = ssl_lookup_id( "byte", pIdent );
            continue;
    case oId_Single:
            ssl_result = ssl_lookup_id( "single", pIdent );
            continue;
    case oId_Double:
            ssl_result = ssl_lookup_id( "double", pIdent );
            continue;
    case oId_Pointer:
            ssl_result = ssl_lookup_id( "pointer", pIdent );
            continue;
    case oId_ShortString:
            ssl_result = ssl_lookup_id( "ShortString", pIdent );
            continue;
    case oId_True:
            ssl_result = ssl_lookup_id( "true", pIdent );
            continue;
    case oId_False:
            ssl_result = ssl_lookup_id( "false", pIdent );
            continue;
    case oId_Nil:
            ssl_result = ssl_lookup_id( "nil", pIdent );
            continue;
    case oId_Ord:
            ssl_result = ssl_lookup_id( "ord", pIdent );
            continue;
    case oId_Chr:
            ssl_result = ssl_lookup_id( "chr", pIdent );
            continue;
    case oId_Pred:
            ssl_result = ssl_lookup_id( "pred", pIdent );
            continue;
    case oId_Succ:
            ssl_result = ssl_lookup_id( "succ", pIdent );
            continue;
    case oId_Sizeof:
            ssl_result = ssl_lookup_id( "sizeof", pIdent );
            continue;
    case oChangeIntLitToLabelIdent: {
            // change current token from pIntLit to pIdent "_label_<intlit>"
            ssl_assert( ssl_token.code == pIntLit );   // intlit text is in ssl_token.name
            std::string name = std::string( "_label_" ) + std::string( ssl_token.name );
            // should have a more official way of pushing data into the scanner's current token
            strcpy( ssl_token.name, name.c_str() );
            ssl_token.namelen = name.length();
            ssl_token.code = pIdent;
            ssl_token.val = ssl_lookup_id( ssl_token.name, pIdent );
            // token was already accepted prior to the change,
            // but re-accept it to set some cached data in scanner.
            ssl_accept_token();
            continue;
            }


     /* Mechanism label_mech */

     case oLabelNew :
            ssl_result = ++labelCount;
            continue;


     /* Mechanism code_mech */

     case oCodeNew : {
            CodeStream* code = new CodeStream;
            // For now, SSL/schema don't directly use CodeStream*.
            // They use integer id's.  That's because schema doesn't support
            // client pointers yet.
            int codeId = codeStreams.size();
            codeStreams.push_back( code );
            ssl_result = codeId;
            }
            continue;
    case oCodePush : {
            CodeStream* code = codeStreams[ssl_param];
            ssl_assert( code != nullptr );
            codeStreamStack.push_back( ssl_param );
            currentCodeStream = code;
            }
            continue;
    case oCodePop : {
            codeStreamStack.pop_back();
            currentCodeStream = codeStreams[codeStreamStack.back()];
            }
            continue;
    case oCodeDiscard : {
            CodeStream* code = codeStreams[ssl_param];
            if ( code ) {
              // blank out the code stream, but don't delete it.
              // I'll leave that cleanup to the owner of codeStreams, for now.
              code->clear();
            }
            }
            continue;


     /* Mechanism include_mech */

     case oIncludeUnitFile : {
            // search for unit file <unit>.pas
            const char* unitName = ssl_get_id_string( ssl_param );
            std::string filename = std::string( unitName ) + ".pas";
            std::string fullPath;
            FILE* fp = nullptr;
            for ( auto& dir : search_path ) {
              fullPath = dir + "/" + filename;
              fp = fopen( fullPath.c_str(), "r" );
              if ( fp ) {
                break;
              }
            }
            ssl_result = ( fp != nullptr );
            if ( fp ) {
              printf( "Including %s\n", fullPath.c_str() );
              ssl_include_filename( fullPath.c_str() );
            }
            }
            continue;
     case oIncludeEnd : {
            // Tell ssl to stop including the current file
            // OR, this should happen when explicitly accepting pEof of the include file?
            ssl_end_include();
            }
            continue;

     /* Mechanism value */

     case oValuePush :
            valueStack.emplace_back( (int) ssl_param );
            continue;
     case oValuePushDouble :
            valueStack.emplace_back( *(double*) &ssl_param );
            continue;
     case oValuePushString :
            valueStack.emplace_back( (const char*) ssl_param );
            continue;
     case oValueTop:
            ssl_result = valueStack.top()._int;
            continue;
     case oValueTopDouble: {
            double value = valueStack.top()._double;
            // return double value encoded as long
            ssl_result = *(long*) &value;
            }
            continue;
     case oValueTopString:
            ssl_result = (long) valueStack.top()._string.c_str();
            continue;
     case oValueSwap: {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.push_back( y );
            valueStack.push_back( x );
            }
            continue;
     case oValuePop :
            valueStack.pop();
            continue;
     case oValueCharToString: {
            ValueEntry x = valueStack.top();  valueStack.pop();
            char temp[2];
            temp[0] = (char) x._int;
            temp[1] = '\0';
            valueStack.emplace_back( temp );
            }
            continue;
     case oValueIntToDouble: {
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( (double) x._int );
            }
            continue;

     case oValueNegate :
            valueStack.top()._int *= -1;
            continue;
     case oValueEqual : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int == y._int );
            }
            continue;
     case oValueNotEqual : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int != y._int );
            }
            continue;
     case oValueLess : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int < y._int );
            }
            continue;
     case oValueGreater : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int > y._int );
            }
            continue;
     case oValueLessEqual : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int <= y._int );
            }
            continue;
     case oValueGreaterEqual : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int >= y._int );
            }
            continue;

     case oValueNegateD :
            valueStack.top()._double *= -1;
            continue;
     case oValueEqualD : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._double == y._double );
            }
            continue;
     case oValueNotEqualD : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._double != y._double );
            }
            continue;
     case oValueLessD : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._double < y._double );
            }
            continue;
     case oValueGreaterD : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._double > y._double );
            }
            continue;
     case oValueLessEqualD : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._double <= y._double );
            }
            continue;
     case oValueGreaterEqualD : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._double >= y._double );
            }
            continue;

     case oValueOr : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int || y._int );
            }
            continue;
     case oValueAnd : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int && y._int );
            }
            continue;
     case oValueNot : {
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( !x._int );
            }
            continue;

     case oValueAdd : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int + y._int );
            }
            continue;
     case oValueSub : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int - y._int );
            }
            continue;
     case oValueMult : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int * y._int );
            }
            continue;
     case oValueDiv : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._int / y._int );
            }
            continue;

     case oValueAddD : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._double + y._double );
            }
            continue;
     case oValueSubD : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._double - y._double );
            }
            continue;
     case oValueMultD : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._double * y._double );
            }
            continue;
     case oValueDivD : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._double / y._double );
            }
            continue;

     case oValueStringCmp : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( x._string.compare( y._string ) );
            }
            continue;
     case oValueStringConcat : {
            ValueEntry y = valueStack.top();  valueStack.pop();
            ValueEntry x = valueStack.top();  valueStack.pop();
            valueStack.emplace_back( ( x._string + y._string ).c_str() );
            }
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
            //  char* sourceStr = ssl_strlit_buffer;
            // int sourceLen = ssl_token.namelen + 1;  // +1 for '\0'
            const char* sourceStr = (const char*) ssl_param;
            int sourceLen = strlen( sourceStr ) + 1;  // +1 for '\0'
            ssl_result = createGlobalData( sourceStr, sourceLen );
            continue;
            }
     case oStringAllocShortStringLit : {
            // The input is a C-style null-terminated string.
            // Store it in global memory as a ShortString, which is a length byte followed by the text.
            // In addition, unlike a normal ShortString, we will also null-terminate the string,
            // A strlit may be assigned to either a ShortString or a PChar.
            // Note: the strlit may be longer than 255 characters, but will appear truncated if referenced
            // as a ShortString.
            const char* sourceStr = (const char*) ssl_param;
            int sourceLen = strlen( sourceStr );
            // Set up the data as we want it to appear in global memory.
            int dataSize = sourceLen + 2;
            char* shortStr = new char[dataSize];
            shortStr[0] = (char) std::min( sourceLen, 255 );
            strcpy( &shortStr[1], sourceStr );
            ssl_result = createGlobalData( shortStr, dataSize );
            delete[] shortStr;
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
            DumpNodeShort( (Node) ssl_param );
            continue;
     case oMsgNodeLong :
            DumpNodeLong( (Node) ssl_param );
            continue;
     case oMsgNodeVec : {
            NODE_VEC_PTR nv = (NODE_VEC_PTR) ssl_param;
            printf( "NodeVec: size %d:\n", nv->size );
            for ( int i = 0; i < nv->size; ++i ) {
              Node node = nv->elements[i];
              DumpNodeShort( node );
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
  if ( optionList ) {
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
  if ( optionList ) fclose(t_lst);
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
  printf("\nWriting Files...\n");

  /* code segment */
  // Only the default code stream is written.   Any others should have been
  // assembled into the default stream already.

  fprintf( t_out, "%lu\n", currentCodeStream->size() );
  int col = 0;
  for ( int x : *currentCodeStream ) {
    fprintf( t_out, "%6d ", x );
    if ( ( ++col % 10 ) == 0 ) {
      fprintf( t_out, "\n" );
    }
  }

  /* string literals table */
  for ( int i = 0; i < dSL.size(); ++i ) {
    fprintf( t_out, "%6d ", dSL[i] );
    if ( ( ++col % 10 ) == 0 ) {
      fprintf(t_out,"\n");
    }
  }
}

