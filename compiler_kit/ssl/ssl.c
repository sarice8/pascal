/**/
static char sccsid[] = "%W% %G% %U% %P%";
/**/

/*
*****************************************************************************
*
*   Syntax/Semantic Language Compiler
*
*   by Steve Rice
*
*   Aug 27, 1989
*
*****************************************************************************
*
*   ssl.c               Semantic operations for SSL compiler.
*
*   HISTORY
*----------------------------------------------------------------------------

*   08/27/89 | Steve  | First version: hardcoded SSL processor written in C
*   08/31/89 |        | SSL processor rewritten in SSL
*   09/02/89 |        | Wrote table walker shell and SSL instructions
*   09/07/89 |        | Finished implementing semantic operations
*   09/09/89 |        | Produce optional listing file
*   10/18/89 |        | Added 'title' section
*   02/02/90 |        | Added debugger output (line # table for now)
*   02/21/90 |        | Added hex constants (0xABCD)
*   03/20/91 |        | Increased size of string shortform table
*   03/26/91 |        | Fixed bug in handling of statement ">>value".
*            |        | Failed inside choice due to stuff on IS stack
*            |        | above the rule id, put there by choice.
*            |        | Fixed with new mechanism oIdentISPushBottom (kludge)
*   04/24/91 |        | Increased id size from 31 chars to 50 chars
*            |        | Increased patch stack sizes
*            |        | Increased output table size
*            |        | Make SSL source case sensitive, but optionally
*            |        |     insensitive
*   05/05/91 |        | Added "include" feature
*   05/21/91 |        | Added mechanism next_error
*            |        | Added table optimization: reduce chains of jumps
*   06/04/91 |        | Write rule addresses to code file.
*   08/21/93 |        | Repackaged to use generic ssl runtime module,
*            |        | scanner, and debugger (ssl_rt.o, ssl_scan.o, debug.o)
*            |        | 
*
*****************************************************************************
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef AMIGA
#include <dos.h>
#endif // AMIGA

/*  SSL Runtime module definitions  */
#include "ssl_rt.h"

/*  Version of runtime model required by programs compiled with this SSL  */
#define TARGET_RUNTIME_VRS "RT 2.0 - Variable stack - 08/27/93"


/*  Schema database access functions  */
#include "schema_db.h"


/*  Optionally integrate SSL debugger */
#ifdef DEBUG

#include "debug.h"
#define DEBUG_FILE   "ssl.dbg"    /* SSL debug information */
#define PROGRAM_FILE "ssl.lst"    /* SSL program listing */

#endif /* DEBUG */


/*  SSL definitions generated for this program  */
#define  SSL_INCLUDE_ERR_TABLE
#include "ssl.h"

/*  SSL code generated for this program  */
#include "ssl.tbl"


char    input_filename[256];


char    list_out_filename[256];
char    tbl_out_filename[256];
char    hdr_out_filename[256];
char    dbg_out_filename[256];

FILE   *f_lst;
FILE   *f_out; 
FILE   *f_hdr; 
FILE   *f_dbg; 

/* user options */

int    option_list;         /* want listing file? */
int    option_debug_out;    /* generate debugger output? */
int    option_generate_C;   /* generate a table in C code? */
int    option_optimize;     /* perform optimization postprocess? */
int    option_verbose;      /* verbose output */
int    option_debug;        /* run ssl compiler under debugger */
int    option_no_warning;   /* don't report warning messages */

int    option_list_code;    /* write generated code in listing file too */
int    list_code_previous_location;

/*************** Variables for semantic mechanisms ***************/


#define w_outTableSize 100000
int     w_outTable[w_outTableSize];     /* Emit into this table */
int     w_here;                         /* Output position */

#define w_lineTableSize 50000           /* Max # lines for debug table */
int     w_lineTable[w_lineTableSize];   /* Line# of every instruction */


char target_title[SSL_STRLIT_SIZE+1];   /* Title of processor being compiled */


#define dSSsize 400                        /* SS: scope stack               */
Node           dSS[dSSsize];
int            dSSptr;
int            dSSlookup;     /* for lookup in scope stack */

Node           dGlobalScope;  /* Global nScope (only set at end of parsing) */


/*  Table of string aliases for declarations  */

#define dStrTableSize 10000
struct dStrTableType {
  char      *buffer;                   /* text is stored in separately allocated memory */
  Node       decl_ptr;                 /* associated nDeclaration node */
} dStrTable[dStrTableSize];
int dStrTableNext;

/* code patch stacks */

#define dMarkSize 200               /* marks in the following patch stacks */
#define dPatchCTAsize 200           /*     (not all the stacks need marks) */
#define dPatchCTsize 5000
#define dPatchCEsize 3000
#define dPatchCsize 8000
#define dPatchLsize 200
#define dPatchBsize 500


int dPatchCTA[dPatchCTAsize];    /* choice table addr ptrs */
int dPatchCT[dPatchCTsize];      /* choice table built here */
int dPatchCE[dPatchCEsize];      /* choice exits */
int dPatchC[dPatchCsize];        /* calls to undefined rules */
int dPatchL[dPatchLsize];        /* start of loop */
int dPatchB[dPatchBsize];        /* breaks out of loop */

/* array pointing to patch stacks */

struct dPSType {
  int *stack;
  int ptr;
  int size;
  int mark[dMarkSize];
  int markPtr;
} dPS[6];


#ifdef DEBUG

void dump_tree_short( void* var, void* udata );
void dump_int_stack( void* var, void* udata );
void dump_int( void* var, void* udata );
void dump_node_short( void* var, void* udata );
void dump_var_stack( void* var, void* udata );

dbg_variables debug_variables[] =
{
    /* "Name",     address,             udata,           function */

    "SS",          (char *)dSS,         (char *)&dSSptr, dump_tree_short,
    "Here",        (char *)&w_here,       NULL,          dump_int,
#if 0
    "vars",        NULL,                  NULL,          dump_var_stack,
#endif

    /*  Type displayers  */
    "Node",        DBG_TYPE_DISPLAY,      NULL,          dump_node_short,

    "",            NULL,                0,               NULL,
};

#endif /* DEBUG */


// Forward declarations

void open_my_files();
void init_my_scanner();
void close_my_files();
int hit_break_key();
void w_optimize_table();
void w_dump_tables();
void dump_debug_symbols();
void list_generated_code ();
void set_global_scope ( Node scope_ptr );

/*  Callbacks  */
void my_listing_function( char* source_line, int token_accepted );
void init_my_operations();




/*
*****************************************************************************
*
* main
*
*****************************************************************************
*/


void
main( int argc, char* argv[] )
{
    int   arg;
    char* p;
    int   status;

    /* Prepare Files */

    option_list = 0;
    option_debug_out = 0;
    option_generate_C = 0;
    option_optimize = 0;
    option_verbose = 0;
    option_debug = 0;
    option_no_warning = 0;

    option_list_code = 0;
    list_code_previous_location = 0;

    for (arg=1; arg<argc; arg++)
    {
        if (*argv[arg] != '-')
            break;
        for (p=argv[arg]+1; *p; p++)
        {
            if (*p=='l')
                option_list = 1;
            else if (*p=='d')
                option_debug_out = 1;
            else if (*p=='c')
                option_generate_C = 1;
            else if (*p=='o')
                option_optimize = 1;
            else if (*p=='v')
                option_verbose = 1;
            else if (*p=='q')
                option_no_warning = 1;
            else if (*p=='r')
            {
                option_list_code = 1;
                option_list = 1;
            }
            else if (*p=='s')
            {
#ifdef DEBUG
                option_debug = 1;
#else  /* DEBUG */
                printf("The -s option is not available in this binary\n");
#endif /* DEBUG */
            }
            else
            {
                printf ("Unknown option '%c'\n", *p);
                exit (10);
            }
        }
    }

    if (arg>=argc)
    {
        printf("Usage:  ssl [-l] [-d] [-o] [-c] [-v] [-q] [-r] [-s] file\n");
        printf("        -l: produce listing file\n");
        printf("        -d: produce debugging file\n");
        printf("        -o: optimize code table\n");
        printf("        -c: produce C code for table\n");
        printf("        -v: verbose\n");
        printf("        -q: quiet (no warning messages reported)\n");
        printf("        -r: report generated code in listing file\n");
        printf("        -s: run ssl compiler under symbolic debugger\n");
#ifndef DEBUG
        printf("            [The -s option is not available in this binary]\n");
#endif  /* DEBUG */

        exit(10);
    }

    sprintf (input_filename, "%s.ssl", argv[arg]);

    sprintf (list_out_filename, "ram_%s.lst", argv[arg]);
    sprintf (tbl_out_filename,  "ram_%s.tbl", argv[arg]);
    sprintf (hdr_out_filename,  "ram_%s.h",   argv[arg]);
    sprintf (dbg_out_filename,  "ram_%s.dbg", argv[arg]);

    open_my_files ();

    ssl_init ();

    init_my_scanner ();

#ifdef DEBUG
    ssl_set_debug (option_debug);
    ssl_set_debug_info (DEBUG_FILE, PROGRAM_FILE, oBreak, debug_variables);
#endif /* DEBUG */

    ssl_set_input_filename (input_filename);

    ssl_set_recovery_token (pSemiColon);


    /* Listing callback will be made by oStartListing at start of second pass.  */


    ssl_set_init_operations_callback (init_my_operations);


#ifdef AMIGA
    onbreak(&hit_break_key);
#endif // AMIGA


    /*  Execute SSL program  */

    status = ssl_run_program ();


    close_my_files ();

    if (status != 0)
        exit (-1);

    exit (0);
}


/*  Callback when first token of each source line accepted.
 *
 *  Normally used to list source file, but here also used to create
 *  debug line table.
 *
 *  Line supplied to us containing '\n' for now.
 */

void
my_listing_function ( char* source_line, int token_accepted )
{
    int     line_number;
    int     addr;

#if 0
    if (option_list_code)
    {
        /*  List generated code in listing file  */

        if (w_here != list_code_previous_location)
        {
            fprintf (f_lst, "====  ");
            for (addr = list_code_previous_location; addr < w_here; addr++)
            {
                fprintf (f_lst, "%4d", w_outTable[addr]);
            }
            fprintf (f_lst, "\n");
            list_code_previous_location = w_here;
        }
    }
#endif /* 0 */

    /*  Collect debug line table  */

    if (token_accepted)
    {
        line_number = ssl_token.lineNumber;
        addr = w_here;
    }
    else
    {
        /*  In this case have to use scanner's "ssl_line_number"
            because token lineNumber not set for blank lines */
        line_number = ssl_line_number;
        addr = -1;
    }

    if (line_number > w_lineTableSize)
        ssl_fatal ("Too many source lines for debug line table");
    else
        w_lineTable[line_number] = addr;


    /*  Produce listing file  */

    if (option_list)
    {
        if (token_accepted)
        {
            fprintf (f_lst, "%4d: %s", w_here, source_line);
        }
        else
        {
            /*  "blank" line  */
            fprintf (f_lst, "      %s", source_line);
        }
    }
}


void
open_my_files ()
{
    if ((f_out = fopen(tbl_out_filename, "w")) == NULL)
    {
        printf ("Can't open output file %s\n", tbl_out_filename);
        exit (10);
    }

    if ((f_hdr = fopen(hdr_out_filename, "w")) == NULL)
    {
        printf ("Can't open output file %s\n", hdr_out_filename);
        exit (10);
    }

    if (option_list)
    {
        if ((f_lst = fopen(list_out_filename, "w")) == NULL)
        {
            printf ("Can't open output file %s\n", list_out_filename);
            exit (10);
        }
    }

    if (option_debug_out)
    {
        if ((f_dbg = fopen(dbg_out_filename, "w")) == NULL)
        {
            printf ("Can't open output file %s\n", dbg_out_filename);
            exit (10);
        }
    }

}


void
close_my_files ()
{
    fclose (f_out);

    fclose (f_hdr);

    if (option_list)
        fclose (f_lst);

    if (option_debug_out)
        fclose (f_dbg);
}


int
hit_break_key()
{
  printf("Breaking...\n");
  close_my_files ();       /* Also need way to clean up SSL */
  return(1);
}


/* --------------------------- Semantic Operations ----------------------- */


void
init_my_operations()
{
    w_here = 0;

    /* Initialize node package (schema runtime module) */
    SCH_Init();

    dSSptr = 0;
    dGlobalScope = NULL;

    /* link up patch stacks */

    dPS[patchChoiceTableAddr].stack = dPatchCTA;
    dPS[patchChoiceTableAddr].size = dPatchCTAsize;
    dPS[patchChoiceTable].stack = dPatchCT;
    dPS[patchChoiceTable].size = dPatchCTsize;
    dPS[patchChoiceExit].stack = dPatchCE;
    dPS[patchChoiceExit].size = dPatchCEsize;
    dPS[patchCall].stack = dPatchC;
    dPS[patchCall].size = dPatchCsize;
    dPS[patchLoop].stack = dPatchL;
    dPS[patchLoop].size = dPatchLsize;
    dPS[patchBreak].stack = dPatchB;
    dPS[patchBreak].size = dPatchBsize;

    strcpy(target_title, "Compiling");    /* default title */
}


/* Pre-define some operation identifiers.  Want to do this in SSL code
   eventually.  */

void
install_system_operations ( long* next_operation )
{
    int        id;
    int        index;
    Node       node_ptr;

    /* This list must match the list of output tokens defined in ssl.ssl
       with the exception of token values that are not emitted in final
       table (e.g. iSpace) */

    static char *system_operations[] =
    {
        "oJumpForward",
        "oJumpBack",
        "oInput",
        "oInputAny",
        "oEmit",
        "oError",
        "oInputChoice",
        "oCall",
        "oReturn",
        "oSetResult",
        "oChoice",
        "oEndChoice",
        "oPushResult",
        "oPop",
        "oBreak",
        "oGlobalSpace",
        "oLocalSpace",
        "oGetParam",
        "oGetFromParam",
        "oGetLocal",
        "oGetGlobal",
        "oGetAddrParam",
        "oGetAddrLocal",
        "oGetAddrGlobal",
        "oAssign",

        NULL,
    };

    for (index = 0; system_operations[index] != NULL; index++)
    {
        id = ssl_add_id (system_operations[index], pIdent);

        node_ptr = NewNode( nOperation );
        SetValue( node_ptr, qIdent, id );
        SetValue( node_ptr, qValue, (*next_operation)++ );
        NodeAddLast( dSS[dSSptr], qDecls, node_ptr );
    }
}


/* Pre-define some type identifiers.  Want to do this in SSL code
   eventually.  */

void
install_system_types ( Node* int_type, Node* token_type )
{
    int        id;
    Node       node_ptr;

    id = ssl_add_id ("int", pIdent);
    node_ptr = NewNode( nType );
    SetValue( node_ptr, qIdent, id );
    NodeAddLast( dSS[dSSptr], qDecls, node_ptr );
    *int_type = node_ptr;

    id = ssl_add_id ("token", pIdent);
    node_ptr = NewNode( nType );
    SetValue( node_ptr, qIdent, id );
    NodeAddLast( dSS[dSSptr], qDecls, node_ptr );
    *token_type = node_ptr;

}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

Node     dNode;
int      dTemp;                            /* multi-purpose */


#include "ssl_begin.h"

    /* built-in Emit operation is handled by application */

    case oEmit :
            if (w_here >= w_outTableSize)
                ssl_fatal ("SSL code table overflow");
            w_outTable[w_here++] = ssl_code_table[ssl_pc++];
            continue;


    /* Mechanism scanner_mech */

    case oResetInput :
            ssl_reset_input();
            continue;
    case oStartListing :
            /* Just turn on listing during second pass, to get code addresses. */
            /* Need to call listing callback for debug_out too, in order to
               collect line table data */
            if (option_list || option_debug_out)
                ssl_set_listing_callback (my_listing_function);
            continue;


    /* Mechanism warning_mech */

    case oWarning :
            if (option_no_warning)
                continue;
            switch (ssl_param)
            {
                case wRuleMissingAtSign:
                    ssl_warning ("Warning: Rule call is missing '@'");
                    break;
                case wReturnValueIgnored:
                    ssl_warning ("Warning: Return value ignored");
                    break;
                default:
                    sprintf (ssl_error_buffer, "Warning #%ld (no message)", ssl_param);
                    ssl_warning (ssl_error_buffer);
                    break;
            }
            continue;


    /* Mechanism more_errors_mech */

    case oUndeclaredRule :
            sprintf (ssl_error_buffer, "Rule '%s' was referenced but never declared",
                     ssl_get_id_string(ssl_param));
            ssl_error (ssl_error_buffer);
            break;
            continue;

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


    /* Mechanism shortForm */

    case oShortFormAdd :
            if (dStrTableNext==dStrTableSize)
                ssl_fatal("String table overflow");
            dStrTable[dStrTableNext].buffer = strdup(ssl_strlit_buffer);
            dStrTable[dStrTableNext++].decl_ptr = (Node)ssl_param;
            continue;
    case oShortFormLookup :
            dNode = NULL;
            for (dTemp = 0; dTemp < dStrTableNext; dTemp++)
            {
                if (strcmp(ssl_strlit_buffer, dStrTable[dTemp].buffer) == 0)
                {
                    dNode = dStrTable[dTemp].decl_ptr;
                    break;
                }
            }
            if (dNode == Null)
                ssl_error("String shortform is not defined");
            ssl_var_stack[ssl_param] = (long) dNode;
            continue;


    /* Mechanism patch */

    case oPatchMark :
            if (++dPS[ssl_param].markPtr==dMarkSize)
                ssl_fatal("mark overflow");
            dPS[ssl_param].mark[dPS[ssl_param].markPtr] = dPS[ssl_param].ptr;
            continue;
    case oPatchAtMark :
            ssl_result = dPS[ssl_param].mark[dPS[ssl_param].markPtr] ==
                         dPS[ssl_param].ptr;
            if (ssl_result)
                dPS[ssl_param].markPtr--;
            continue;
    case oPatchPushHere :
            if (++dPS[ssl_param].ptr==dPS[ssl_param].size)
                ssl_fatal("patch overflow");
            dPS[ssl_param].stack[dPS[ssl_param].ptr] = w_here;
            continue;
    case oPatchPushInt :
            dTemp = ssl_argv(0,2);   /* Patch stack */
            if (++dPS[dTemp].ptr==dPS[dTemp].size)
                ssl_fatal("patch overflow");
            dPS[dTemp].stack[dPS[dTemp].ptr] = ssl_argv(1,2);
            continue;
    case oPatchPushIdent :
            if (++dPS[ssl_param].ptr==dPS[ssl_param].size)
                ssl_fatal("patch overflow");
            dPS[ssl_param].stack[dPS[ssl_param].ptr] = ssl_last_id;
            continue;
    case oPatchAnyEntries :
            ssl_result = dPS[ssl_param].ptr > 0;
            continue;              
    case oPatchPopFwd :
            dTemp = dPS[ssl_param].stack[dPS[ssl_param].ptr--];
            w_outTable[dTemp] = w_here - dTemp;
            continue;
    case oPatchPopBack :
            w_outTable[w_here] = w_here -
                dPS[ssl_param].stack[dPS[ssl_param].ptr--];
            w_here++;
            continue;
    case oPatchPopValue :
            w_outTable[w_here++] = dPS[ssl_param].stack[dPS[ssl_param].ptr--];
            continue;
    case oPatchPopCall :
            dTemp = dPS[ssl_param].stack[dPS[ssl_param].ptr--];
            /* Find identifier 'dTemp' in scope */
            for (dSSlookup = dSSptr; dSSlookup > 0; dSSlookup--)
            {  
                dNode = FindValue_NoErrorChecking (dSS[dSSlookup], qDecls, qIdent, dTemp);
                if (dNode != NULL)
                    break;
            }  
            w_outTable[dPS[ssl_param].stack[dPS[ssl_param].ptr--]] = GetValue( dNode, qValue );
            continue;


    /* Mechanism titleMech */

    case oTitleSet :
            strcpy(target_title, ssl_strlit_buffer);
            continue;


    /* Mechanism doc */

    case oDocNewRule :
            if (option_verbose)
                printf("Rule %s\n", ssl_token.name);
            continue;


    /* Mechanism include_mech */

    case oInclude :
            ssl_include_filename (ssl_strlit_buffer);
            continue;


    /* mechanism Node */

    case oNodeNew:      /* (node_type) -- create node */
            ssl_result = (long) NewNode( ssl_param );
            continue;
    case oNodeSet:
            SetAttr( (Node)ssl_argv(0,3), ssl_argv(1,3), (Node)ssl_argv(2,3) );
            continue;
    case oNodeSetInt:
    case oNodeSetBoolean:
            SetValue( (Node)ssl_argv(0,3), ssl_argv(1,3), ssl_argv(2,3) );
            continue;
    case oNodeGet:
            ssl_result = (long) GetAttr( (Node)ssl_argv(0,2), ssl_argv(1,2) );
            continue;
    case oNodeGetInt:
    case oNodeGetBoolean:
            ssl_result = (long) GetValue( (Node)ssl_argv(0,2), ssl_argv(1,2) );
            continue;
    case oNodeNull:
            ssl_result = ((Node)ssl_param == NULL);
            continue;

    case oNodeGetIter: {
            List list = (List) GetAttr( (Node)ssl_argv(0,2), ssl_argv(1,2) );
            if ( !list ) {
              ssl_result = 0;
            } else {
              ssl_result = (long) FirstItem( list );
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

    case oNodeType:
            ssl_result = Kind( (Node)ssl_param );
            continue;
    case oNodeEqual:
            ssl_result = (ssl_argv(0,2) == ssl_argv(1,2));
            continue;


    /* mechanism Scope */

    case oScopeBegin:
            dNode = NewNode( nScope );
            if (dSSptr > 0)
                SetAttr( dNode, qParentScope, dSS[dSSptr] );
            ssl_var_stack[ssl_param] = (long) dNode;
            if (++dSSptr == dSSsize) ssl_fatal ("SS overflow");
            dSS[dSSptr] = dNode;
            continue;
    case oScopeOpen:
            if (++dSSptr == dSSsize) ssl_fatal ("SS overflow");
            dSS[dSSptr] = (Node) ssl_param;
            continue;
    case oScopeEnd:
            ssl_assert (dSSptr >= 1);
            dSSptr--;
            continue;
    case oScopeDeclare:
            ssl_assert (dSSptr >= 1);
            NodeAddLast( dSS[dSSptr], qDecls, (Node)ssl_param );
            continue;
    case oScopeFind:
            for (dSSlookup = dSSptr; dSSlookup > 0; dSSlookup--)
            {  
                dNode = FindValue_NoErrorChecking( dSS[dSSlookup], qDecls, qIdent, ssl_last_id );
                if (dNode != NULL)
                    break;
            }
            ssl_var_stack[ssl_param] = (long) dNode;
            ssl_result = (dNode != NULL);
            continue;
    case oScopeFindInCurrentScope: {
                dNode = FindValue_NoErrorChecking( dSS[dSSptr], qDecls, qIdent, ssl_last_id );
                ssl_result = (long) dNode;
            }
            continue;
    case oScopeFindRequire:
            for (dSSlookup = dSSptr; dSSlookup > 0; dSSlookup--)
            {  
                dNode = FindValue_NoErrorChecking( dSS[dSSlookup], qDecls, qIdent, ssl_last_id );
                if (dNode != NULL)
                    break;
            }  
            ssl_var_stack[ssl_param] = (long) dNode;
            if (dNode == NULL)
            {
                ssl_error ("Undefined symbol");
                ssl_pc--;
                ssl_error_recovery ();
            }
            continue;


    /* mechanism Install */

    case oInstallSystemOperations:
            install_system_operations (&ssl_var_stack[ssl_param]);
            continue;
    case oInstallSystemTypes:
            install_system_types ((Node*)&(ssl_var_stack[ssl_argv(0,2)]),
                                  (Node*)&(ssl_var_stack[ssl_argv(1,2)]));
            continue;


    /* mechanism WriteTables */

    case oWriteTables:
            set_global_scope ((Node)ssl_param);
            if (option_optimize)
                w_optimize_table();
            if (option_list_code)
                list_generated_code ();
            w_dump_tables ();
            continue;


#include "ssl_end.h"

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */



/************************ S c a n n e r *********************/

struct ssl_token_table_struct my_keyword_table[] =
{
    "title",         pTitle,
    "input",         pInput,
    "output",        pOutput,
    "type",          pType,
    "error",         pError,
    "include",       pInclude,
    "mechanism",     pMechanism,
    "rules",         pRules,
    "end",           pEnd,
    "in",            pIn,
    "out",           pOut,
    "inout",         pInOut,

    NULL,            0
};

struct ssl_token_table_struct my_operator_table[] =
{
    "{",             pLCurly,
    "}",             pRCurly,
    "[",             pLSquare,
    "]",             pRSquare,
    "|",             pBar,
    "@",             pCall,
    "*",             pStar,
    "?",             pQuestion,
    ".",             pEmit,
    "-",             pMinus,
    "=",             pEquals,
    ":",             pColon,
    ";",             pSemiColon,
    ",",             pComma,
    "(",             pLParen,
    ")",             pRParen,
    "#",             pErr,
    ">",             pBreak,
    ">>",            pReturn,

    NULL,            0
};

struct ssl_special_codes_struct my_special_codes;


void
init_my_scanner ()
{
    my_special_codes.invalid = pInvalid;
    my_special_codes.eof     = pEof;
    my_special_codes.ident   = pIdent;
    my_special_codes.intlit  = pIntLit;
    my_special_codes.strlit  = pStrLit;

    ssl_init_scanner (my_keyword_table, my_operator_table, &my_special_codes);
}


void
set_global_scope( Node scope_ptr )
{
    if ( !IsA( Kind( scope_ptr ), nScope ) )
    {
        ssl_error ("Internal error: global scope out of place");
        return;
    }
    dGlobalScope = scope_ptr;
}


void
w_dump_tables()
{
    int               i;
    int               count;
    struct rule_addr {
        const char* name;
        int         addr;
        struct rule_addr* next;
    };
    struct rule_addr *head, *curr, *prev, *new_rule;
    int               rule_table_size;
  
    if (ssl_error_count > 0)
    {
        printf ("\nNot writing files due to errors\n");
        return;
    }

    printf ("\nWriting Files...\n");

    /* Build list of rule addresses, sorted by address (insertion sort) */

    rule_table_size = 0;
    head = NULL;

    Item it;
    FOR_EACH_ITEM( it, (List) GetAttr( dGlobalScope, qDecls ) ) {
        Node node_ptr = Value( it );
        if ( Kind(node_ptr) == nRule )
        {
            new_rule = (struct rule_addr *) malloc (sizeof(struct rule_addr));
            new_rule->name = ssl_get_id_string( GetValue( node_ptr, qIdent ) );
            new_rule->addr = GetValue( node_ptr, qValue );
            new_rule->next = NULL;
            rule_table_size++;
            if (head == NULL)
                head = new_rule;
            else
            {
                curr = head;
                prev = NULL;
                while (curr != NULL && curr->addr < new_rule->addr)
                {
                    prev = curr;
                    curr = curr->next;
                }
                if (prev == NULL)
                    head = new_rule;
                else
                    prev->next = new_rule;
                new_rule->next = curr;
            }
        }
    }

    /*  Write table  */

    if (option_generate_C)
    {
        fprintf (f_out, "/* Automatically generated by ssl */\n\n");
        fprintf (f_out, "char ssl_title_string[] = \"%s\";\n", target_title);
        fprintf (f_out, "char ssl_runtime_vrs[]  = \"%s\";\n\n", TARGET_RUNTIME_VRS);
        fprintf (f_out, "int ssl_code_table[] = {\n\t");
        for (i = 0; i < w_here; i++)
        {
            fprintf (f_out, "%d, ", w_outTable[i]);
            if ((i & 15) == 15)
                fprintf (f_out, "\n\t");
        }
        fprintf (f_out, "\n};\n\n");
        fprintf (f_out, "int ssl_code_table_size = %d;\n", w_here);

        /* output rule addresses */
        fprintf (f_out, "int ssl_rule_table_size = %d;\n", rule_table_size);
        fprintf (f_out, "struct ssl_rule_table_struct ssl_rule_table[] = {\n");
        for (curr = head; curr != NULL; curr = curr->next)
        {
            fprintf (f_out, "\t{\"%s\",\t%d},\n", curr->name, curr->addr);
        }
        fprintf (f_out, "};\n");
    }
    else
    {
        /* output code */
        fprintf (f_out, "%s\n", target_title);
        fprintf (f_out, "%s\n", TARGET_RUNTIME_VRS);
        fprintf (f_out, "%d\n\n", w_here);
        for (i=0; i<w_here; i++)
            fprintf (f_out, "%d\n", w_outTable[i]);

        /* output rule addresses */
        fprintf (f_out, "%d\n", rule_table_size);
        for (curr = head; curr != NULL; curr = curr->next)
        {
            fprintf (f_out, "%s\t%d\n", curr->name, curr->addr);
        }
    }

    /*  Write debug file  */

    if (option_debug_out)
    {
        fprintf(f_dbg,"%d\n", ssl_line_number);  /* # lines */
        for (i=1; i <= ssl_line_number; i++)
          fprintf(f_dbg,"%d\n",w_lineTable[i]);
        dump_debug_symbols();
    }

    /*  Write header file  */

    /*  Round up the code table size so we only need to recompile the
        application C code occasionally.  */
    fprintf(f_hdr,"#define SSL_CODE_TABLE_SIZE %d\n\n",
                  ((w_here/100)+2)*100);

    FOR_EACH_ITEM( it, (List) GetAttr( dGlobalScope, qDecls ) ) {
        Node node_ptr = Value( it );
        switch ( Kind(node_ptr) )
        {
            /* case nInput: */
            case nOutput:
            case nError:
            case nValue:
            case nOperation:
                fprintf( f_hdr, "#define %s %ld\n",
                         ssl_get_id_string( GetValue( node_ptr, qIdent ) ),
                         GetValue( node_ptr, qValue ) );
                break;

#if 0
  /* This would allow operations to access global variables declared in SSL.
   * But I don't want to do it, because it would force a recompile of the C code
   * if any globals were added.
   * I've been trying to avoid recompilation if only the stuff in "rules" changes.
   * (Of course, that's assuming that the user didn't generate C code for the table).
   */
            case nGlobal:
                fprintf( f_hdr, "#define var_%s (ssl_var_stack[%ld])\n",
                         ssl_get_id_string( GetValue( node_ptr, qIdent ) ),
                         GetValue( node_ptr, qAddr ) );
                break;
#endif /* 0 */

            default:
                break;

        }
    }

    count = 0;
    fprintf(f_hdr,"\n#ifdef SSL_INCLUDE_ERR_TABLE\n");

    fprintf(f_hdr,"\nstruct ssl_error_table_struct ssl_error_table[] = {\n");

    FOR_EACH_ITEM( it, (List) GetAttr( dGlobalScope, qDecls ) ) {
        Node node_ptr = Value( it );
        if ( Kind(node_ptr) == nError )
        {
            fprintf( f_hdr,"   \"%s\", %ld,\n",
                     ssl_get_id_string( GetValue( node_ptr, qIdent ) ),
                     GetValue( node_ptr, qValue ) );
            count++;
        }
    }
    fprintf(f_hdr,"   \"\", 0\n};\nint ssl_error_table_size = %d;\n",count);
    fprintf(f_hdr,"\n#endif // SSL_INCLUDE_ERR_TABLE\n");

}


void
dump_debug_symbols ()
{
    Item it;
    FOR_EACH_ITEM( it, (List) GetAttr( dGlobalScope, qDecls ) ) {
        Node np = Value( it );
        switch ( Kind( np ) )
        {
            case nType:
                fprintf( f_dbg, "%ld %s %s\n", NodeNum(np),
                         ssl_get_id_string( GetValue( np,qIdent ) ),
                         SCH_GetTypeName( Kind( np ) ) );
                break;
            case nGlobal:
                fprintf( f_dbg, "%ld %s %s %ld %ld\n", NodeNum(np),
                         ssl_get_id_string( GetValue( np, qIdent ) ),
                         SCH_GetTypeName( Kind( np ) ),
                         NodeNum( GetAttr( np, qType ) ),
                         GetValue( np, qAddr ) );
                break;
            case nRule:
                fprintf( f_dbg, "%ld %s %s %ld\n", NodeNum(np),
                         ssl_get_id_string( GetValue( np, qIdent ) ),
                         SCH_GetTypeName( Kind( np ) ),
                         GetValue( np, qValue ) );

                Item it2;
                FOR_EACH_ITEM( it2, (List) GetAttr( GetAttr( np, qParamScope ), qDecls ) ) {
                    Node p = Value( it2 );
                    fprintf( f_dbg, "%ld %s %s %ld %ld\n", NodeNum(p),
                             ssl_get_id_string( GetValue( p, qIdent ) ),
                             SCH_GetTypeName( Kind( p ) ),
                             NodeNum( GetAttr( p, qType ) ),
                             GetValue( p, qAddr ) );
                }
                FOR_EACH_ITEM( it2, (List) GetAttr( GetAttr( np, qScope ), qDecls ) ) {
                    Node p = Value( it2 );
                    fprintf( f_dbg, "%ld %s %s %ld %ld\n", NodeNum(p),
                             ssl_get_id_string( GetValue( p, qIdent ) ),
                             SCH_GetTypeName( Kind( p ) ),
                             NodeNum( GetAttr( p, qType ) ),
                             GetValue( p, qAddr ) );
                }
                break;

            default:
                break;
        }
    }
}


/*  Dump a listing of generated code to the listing file  */

void
list_generated_code ()
{
    int    pc, target, final_target;
    int    option_val, options;
    int    addr;
    int    op;
    int    arg;
    int    has_arg;
    const char* get_op_name();

    /*  keep track of when next choice table is coming up  */
    int    choice_table [dPatchCTAsize], choice_table_sp;

    fprintf (f_lst, "\nGenerated code:\n\n");

    choice_table_sp = 0;

    pc = 0;
    while (pc < w_here)
    {
        addr = pc;

        if (choice_table_sp > 0  &&
            choice_table [choice_table_sp] == pc)
        {
            options = w_outTable [pc++];
            fprintf (f_lst, "%4d: Choice Lookup Table\n", addr);

            while (options > 0)
            {
                option_val = w_outTable[pc++];
                target = pc - w_outTable[pc];
                pc++;
                fprintf (f_lst, "        %4d   %4d\n", option_val, target);
                options--;
            }

            choice_table_sp--;
            continue;
        }

        op = w_outTable[pc++];
        has_arg = 0;
        switch (op)
        {
            case iJumpForward :
                arg = pc + w_outTable[pc];
                pc++;
                has_arg = 1;
                break;

            case iJumpBack :
                arg = pc - w_outTable[pc];
                pc++;
                has_arg = 1;
                break;

            case iInputChoice :
            case iChoice :
                choice_table [++choice_table_sp] = pc + w_outTable[pc];
                pc++;
                arg = choice_table[choice_table_sp];
                has_arg = 1;
                break;

            /* Multi-value instructions */

            case iInput :
            case iEmit :
            case iError :
            case iCall :
            case iSetResult :
            case iPop :
            case iGlobalSpace :
            case iLocalSpace :
            case iGetParam :
            case iGetFromParam :
            case iGetLocal :
            case iGetGlobal :
            case iGetAddrParam :
            case iGetAddrLocal :
            case iGetAddrGlobal :
                arg = w_outTable[pc++];
                has_arg = 1;
                break;
                

            /* Single-value instructions */

            case iInputAny :
            case iReturn :
            case iEndChoice :
            case iPushResult :
            case iAssign :
                break;

            default :
                break;
        }
        if (has_arg)
            fprintf (f_lst, "%4d: %s %d\n", addr, get_op_name(op), arg);
        else
            fprintf (f_lst, "%4d: %s\n", addr, get_op_name(op));
    }
}


const char*
get_op_name( int op )
{
    Item it;
    FOR_EACH_ITEM( it, (List) GetAttr( dGlobalScope, qDecls ) ) {
        Node node_ptr = Value( it );
        if ( ( Kind( node_ptr ) == nOperation ) &&
             ( GetValue( node_ptr, qValue ) == op ) )
        {
            return ssl_get_id_string( GetValue( node_ptr, qIdent ) );
        }
    }
    return ("<UnnamedOperation>");
}


/* Optimize generated table */

void
w_optimize_table ()
{
    int    pc, target, final_target;
    int    options;
    int    improved_count;
    int    improved_case_jump_count;
    int    find_ultimate_destination();

    /*  keep track of when next choice table is coming up  */
    int    choice_table[dPatchCTAsize], choice_table_sp;

    printf ("\nOptimizing table...\n");

    choice_table_sp = 0;

    /* Convert a chain of jumps into a single jump */

    improved_count = 0;
    improved_case_jump_count = 0;

    pc = 0;
    while (pc < w_here)
    {
        if (choice_table_sp > 0  &&
            choice_table [choice_table_sp] == pc)
        {
            /* skip past choice table ... maybe optimize it too */

            options = w_outTable [pc++];
            /* pc += 2 * options; */

            while (options > 0)
            {
                pc++;
                target = pc - w_outTable[pc];
                final_target = find_ultimate_destination (target);
                if ((final_target != target) &&
                    (final_target < pc))
                {
                    w_outTable[pc] = pc - final_target;
                    improved_case_jump_count++;
                    improved_count++;
                }
                pc++;
                options--;
            }

            choice_table_sp--;
            continue;
        }

        switch (w_outTable[pc++])
        {
            case iJumpForward :
                    target = pc + w_outTable[pc];
                    final_target = find_ultimate_destination (target);
                    if (final_target != target)
                    {
                        improved_count++;
                        if (final_target > pc)
                        {
                            w_outTable [pc-1] = iJumpForward;
                            w_outTable [pc] = final_target - pc;
                        }
                        else
                        {
                            w_outTable [pc-1] = iJumpBack;
                            w_outTable [pc] = pc - final_target;
                        }
                    }
                    pc++;
                    continue;

            case iJumpBack :
                    target = pc - w_outTable[pc];
                    final_target = find_ultimate_destination (target);
                    if (final_target != target)
                    {
                        improved_count++;
                        if (final_target > pc)
                        {
                            w_outTable [pc-1] = iJumpForward;
                            w_outTable [pc] = final_target - pc;
                        }
                        else
                        {
                            w_outTable [pc-1] = iJumpBack;
                            w_outTable [pc] = pc - final_target;
                        }
                    }
                    pc++;
                    continue;

            case iInputChoice :
            case iChoice :
                    choice_table [++choice_table_sp] = pc + w_outTable[pc];
                    pc++;
                    continue;


            /* Instructions with argument */

            case iInput :
            case iEmit :
            case iError :
            case iCall :
            case iSetResult :
            case iPop :
            case iGlobalSpace :
            case iLocalSpace :
            case iGetParam :
            case iGetFromParam :
            case iGetLocal :
            case iGetGlobal :
            case iGetAddrParam :
            case iGetAddrLocal :
            case iGetAddrGlobal :
                    pc++;
                    continue;

            /* Single-value instructions */

            case iInputAny :
            case iReturn :
            case iEndChoice :
            case iPushResult :
            case iAssign :
                    continue;

            default :
                    continue;
        }
    }
    printf ("Reduced %d jump instructions\n", improved_count);
    if (improved_case_jump_count)
        printf ("including %d jumps directly from a case.\n",
                improved_case_jump_count);
}


int
find_ultimate_destination( int pc )
{
    while (1)
    {
        if (w_outTable[pc] == iJumpForward)
        {
            pc++;
            pc += w_outTable[pc];
        }
        else if (w_outTable[pc] == iJumpBack)
        {
            pc++;
            pc -= w_outTable[pc];
        }
        else
            break;
    }

    return (pc);
}



/*  For debugger display of program variables  */


void
dump_tree_short ( void* variable, void* udata )
{
    int         stack_size;
    Node*       stack;

    stack = (Node*) variable;

    for (stack_size = *((int*) udata);
         stack_size > 0;
         stack_size--)
    {
        DumpNodeShort (stack[stack_size]);
    }
}


void
dump_node_short ( void* variable, void* udata )
{
    Node        node_ptr;

    node_ptr = *((Node*) variable);

    DumpNodeShort (node_ptr);
}


void
dump_int_stack ( void* variable, void* udata )
{
    int  stack_size;
    int* stack;

    stack = (int*) variable;

    for (stack_size = *((int*) udata);
         stack_size > 0;
         stack_size--)
    {
        printf ("\t%d\n", stack[stack_size]);
    }
}


void
dump_int ( void* variable, void* udata )
{
    int    *var;

    var = (int*) variable;

    printf ("\t%d\n", *var);
}


void
dump_var_stack ( void* variable, void* udata )
{
    int    addr;
    int    fp;

    fp = ssl_var_fp;

    for (addr = ssl_var_sp; addr > 0; addr--)
    {
        printf ("%4d: %5ld", addr, ssl_var_stack[addr]);
        if (addr == fp)
        {
            printf ("  <-- fp");
            fp = ssl_var_stack[addr];
        }
        if (addr == ssl_var_sp)
            printf ("  <-- sp");
        printf ("\n");
    }
}

