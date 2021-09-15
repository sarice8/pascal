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

#ifdef AMIGA
#include <dos.h>
#endif AMIGA

/*  SSL Runtime module definitions  */
#include "ssl_rt.h"

/*  Version of runtime model required by programs compiled with this SSL  */
#define TARGET_RUNTIME_VRS "RT 1.1 - Original SSL model, generic ssl_rt - 08/18/93"


#ifdef DEBUG

/*  Optionally integrate SSL debugger */

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


/*************** Variables for semantic mechanisms ***************/

/*
 * Identifier attributes table.
 *
 * This table is parallel to the ssl scanner's ssl_id_table.
 * i.e. they are both indexed by the identifier value.
 * The identifier name and code (pIDENTIFIER, keyword code, etc)
 * are stored in the ssl_id_table.
 *
 * (This table should be as large as the ssl_id_table.)
 */

struct id_table_struct
{
    short   kind;        /* kind of identifier in SSL program */
    char    declared;
    char    choice;
    char    param;
    short   val;         /* code #, or rule address */
    short   type;        /* identifier's user defined type */
    short   paramType;
} id_table[ssl_id_table_size];


#define w_outTableSize 5000
short   w_outTable[w_outTableSize];     /* Emit into this table */
short   w_here;                         /* Output position */

#define w_lineTableSize 3000            /* Max # lines for debug table */
short   w_lineTable[w_lineTableSize];   /* Line# of every instruction */


char target_title[SSL_STRLIT_SIZE+1];   /* Title of processor being compiled */


short dTemp;                            /* multi-purpose */

#define dCSsize 30
short dCS[dCSsize], dCSptr;        /* count stack */

short dNextError;                  /* for mechanism next_error */

#define dVSsize 30
short dVS[dVSsize], dVSptr;        /* value stack */

#define dISsize 30
short dIS[dISsize], dISptr;

#define dStrTableSize 100
struct dStrTableType {
  char *buffer;                   /* text is stored in separately allocated memory */
  short val;
} dStrTable[dStrTableSize];
short dStrTableNext;

/* code patch stacks */

#define dMarkSize 20               /* marks in the following patch stacks */
#define dPatchCTAsize 20           /*     (not all the stacks need marks) */
#define dPatchCTsize 500
#define dPatchCEsize 300
#define dPatchCsize 800
#define dPatchLsize 20
#define dPatchBsize 50


short dPatchCTA[dPatchCTAsize];    /* choice table addr ptrs */
short dPatchCT[dPatchCTsize];      /* choice table built here */
short dPatchCE[dPatchCEsize];      /* choice exits */
short dPatchC[dPatchCsize];        /* calls to undefined rules */
short dPatchL[dPatchLsize];        /* start of loop */
short dPatchB[dPatchBsize];        /* breaks out of loop */

/* array pointing to patch stacks */

struct dPSType {
  short *stack;
  short ptr;
  short size;
  short mark[dMarkSize];
  short markPtr;
} dPS[6];


#ifdef DEBUG

dbg_variables debug_variables[] =
{
    /* "Name", address,    udata,    function */
    "",        NULL,       0,        NULL,
};

#endif /* DEBUG */


/* ----------------------------------------------------------------- */

/*  Callbacks  */
int  my_listing_function ();
int  init_my_operations ();


/* ----------------------------------------------------------------- */


/*
*****************************************************************************
*
* main
*
*****************************************************************************
*/


main (argc, argv)
int   argc;
char *argv[];
{
    int     hit_break_key();
    short   arg;
    char   *p;
    int     status;

  /* Prepare Files */

    option_list = 0;
    option_debug_out = 0;
    option_generate_C = 0;
    option_optimize = 0;
    option_verbose = 0;
    option_debug = 0;

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
        printf("Usage:  ssl [-l] [-d] [-o] [-c] [-v] [-s] file\n");
        printf("        -l: produce listing file\n");
        printf("        -d: produce debugging file\n");
        printf("        -o: optimize code table\n");
        printf("        -c: produce C code for table\n");
        printf("        -v: verbose\n");
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


    /* Need to call listing callback for debug_out too, in order to
       collect line table data */

    if (option_list || option_debug_out)
        ssl_set_listing_callback (my_listing_function);


    ssl_set_init_operations_callback (init_my_operations);


#ifdef AMIGA
    onbreak(&hit_break_key);
#endif AMIGA


    /*  Execute SSL program  */

    status = ssl_run_program ();


    if (status == 0)
    {
        if (option_optimize)
            w_optimize_table();

        w_dump_tables();
    }
    
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

int my_listing_function (source_line, token_accepted)
char                    *source_line;
int                      token_accepted;
{
    int     line_number;
    int     addr;

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

close_my_files ()
{
    fclose (f_out);

    fclose (f_hdr);

    if (option_list)
        fclose (f_lst);

    if (option_debug_out)
        fclose (f_dbg);
}

hit_break_key()
{
  printf("Breaking...\n");
  close_my_files ();       /* Also need way to clean up SSL */
  return(1);
}


/* --------------------------- Semantic Operations ----------------------- */


init_my_operations()
{
    register short i;
    short    firstOp,lastOp,op,opVal;

    w_here = 0;

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


    /* Pre-define some operation identifiers */

    firstOp = ssl_add_id ("oJumpForward",  pIdent);
              ssl_add_id ("oJumpBack",     pIdent);
              ssl_add_id ("oInput",        pIdent);
              ssl_add_id ("oInputAny",     pIdent);
              ssl_add_id ("oEmit",         pIdent);
              ssl_add_id ("oError",        pIdent);
              ssl_add_id ("oInputChoice",  pIdent);
              ssl_add_id ("oCall",         pIdent);
              ssl_add_id ("oReturn",       pIdent);
              ssl_add_id ("oSetResult",    pIdent);
              ssl_add_id ("oChoice",       pIdent);
              ssl_add_id ("oEndChoice",    pIdent);
              ssl_add_id ("oSetParameter", pIdent);
    lastOp =  ssl_add_id ("oBreak",        pIdent);

    opVal = 0;

    for (op = firstOp; op <= lastOp; op++)
    {
        id_table[op].kind     = kOp;
        id_table[op].val      = opVal++;
        id_table[op].declared = 1;
        /* other fields 0 */
    }
}


/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

#include "ssl_begin.h"

       /* built-in Emit operation is handled by application */

       case oEmit :
              dTemp = ssl_code_table[ssl_pc++];
              switch (dTemp) {
                case iConstant :   w_outTable[w_here++] = dVS[dVSptr];
                                   continue;
                case iIdentVal :   w_outTable[w_here++] =
                                       id_table[ssl_last_id].val;
                                   continue;
                case iIdentISVal : w_outTable[w_here++] =
                                       id_table[dIS[dISptr]].val;
                                   continue;
                default :          w_outTable[w_here++] = dTemp;
                                   continue;
              }

       /* Mechanism count */

       case oCountPush :
              if (++dCSptr==dCSsize) ssl_fatal("CS overflow");
              dCS[dCSptr] = ssl_param;
              continue;
       case oCountPushIntLit :
              if (++dCSptr==dCSsize) ssl_fatal("CS overflow");
              dCS[dCSptr] = ssl_token.val;
              continue;
       case oCountPop :
              dCSptr--;
              continue;
       case oCountInc :
              dCS[dCSptr]++;
              continue;
       case oCountDec :
              dCS[dCSptr]--;
              continue;
       case oCountNegate :
              dCS[dCSptr] = -dCS[dCSptr];
              continue;
       case oCountZero :
              ssl_result = !dCS[dCSptr];
              continue;

       /* Mechanism next_error */

       case oNextErrorPushCount :
              if (++dCSptr==dCSsize) ssl_fatal("CS overflow");
              dCS[dCSptr] = dNextError;
              continue;
       case oNextErrorPopCount :
              dNextError = dCS[dCSptr--];
              continue;

       /* Mechanism value */

       case oValuePushKind :
              if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
              dVS[dVSptr] = ssl_param;
              continue;
       case oValuePushVal :
              if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
              dVS[dVSptr] = id_table[ssl_last_id].val;
              continue;
       case oValuePushISVal :
              if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
              dVS[dVSptr] = id_table[dIS[dISptr]].val;
              continue;
       case oValuePushIdent :
              if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
              dVS[dVSptr] = ssl_last_id;
              continue;
       case oValuePushType :
              if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
              dVS[dVSptr] = id_table[ssl_last_id].type;
              continue;
       case oValueChooseKind :
              ssl_result = dVS[dVSptr];
              continue;
       case oValuePushCount :
              if (++dVSptr==dVSsize) ssl_fatal("VS overflow");
              dVS[dVSptr] = dCS[dCSptr];
              continue;
       case oValueSwap :
              dTemp = dVS[dVSptr];
              dVS[dVSptr] = dVS[dVSptr-1];
              dVS[dVSptr-1] = dTemp;
              continue;
       case oValuePop :
              dVSptr--;
              continue;

       /* Mechanism ident */

       case oIdentSetDeclared :
              if (id_table[ssl_last_id].declared)
                ssl_error("Name already declared");
              id_table[ssl_last_id].declared = 1;
              continue;
       case oIdentSetKind :
              id_table[ssl_last_id].kind = ssl_param;
              continue;
       case oIdentSetKindVS :
              id_table[ssl_last_id].kind = dVS[dVSptr];
              continue;
       case oIdentSetType :
              id_table[ssl_last_id].type = dVS[dVSptr];
              continue;
       case oIdentSetValCount :
              id_table[ssl_last_id].val = dCS[dCSptr];
              continue;
       case oIdentSetValHere :
              id_table[ssl_last_id].val = w_here;
              continue;
       case oIdentSetChoice :
              id_table[ssl_last_id].choice = ssl_param;
              continue;
       case oIdentChooseKind :
              ssl_result = id_table[ssl_last_id].kind;
              continue;
       case oIdentChooseParam :
              ssl_result = id_table[ssl_last_id].param;
              continue;
       case oIdentChooseChoice :
              ssl_result = id_table[ssl_last_id].choice;
              continue;
       case oIdentChooseDeclared :
              ssl_result = id_table[ssl_last_id].declared;
              continue;
       case oIdentMatchType :
              ssl_result = id_table[ssl_last_id].type ==
                         id_table[dIS[dISptr]].type;
              continue;
       case oIdentMatchParamType :
              ssl_result = id_table[ssl_last_id].type ==
                         id_table[dIS[dISptr]].paramType;
              continue;
       case oIdentISPush :
              if (++dISptr==dISsize) ssl_fatal("IS overflow");
              dIS[dISptr] = ssl_last_id;
              continue;
       case oIdentISPushBottom :
              if (++dISptr==dISsize) ssl_fatal("IS overflow");
              dIS[dISptr] = dIS[1];
              continue;
       case oIdentISPop :
              dISptr--;
              continue;
       case oIdentSetISParamType :
              id_table[dIS[dISptr]].paramType = dVS[dVSptr];
              continue;
       case oIdentSetISParam :
              id_table[dIS[dISptr]].param = 1;
              continue;
       case oIdentSetISChoice :
              id_table[dIS[dISptr]].choice = ssl_param;
              continue;
       case oIdentSetISType :
              id_table[dIS[dISptr]].type = dVS[dVSptr];
              continue;
       case oIdentChooseISKind :
              ssl_result = id_table[dIS[dISptr]].kind;
              continue;
       case oIdentChooseISChoice :
              ssl_result = id_table[dIS[dISptr]].choice;
              continue;
       case oIdentChooseISParam :
              ssl_result = id_table[dIS[dISptr]].param;
              continue;
       case oIdentMatchISType :
              ssl_result = id_table[dIS[dISptr-1]].type ==
                         id_table[dIS[dISptr]].type;
              continue;

       /* Mechanism shortForm */

       case oShortFormAdd :
              if (dStrTableNext==dStrTableSize)
                ssl_fatal("String table overflow");
              dStrTable[dStrTableNext].buffer = strdup(ssl_strlit_buffer);
              dStrTable[dStrTableNext++].val = ssl_last_id;
              continue;
       case oShortFormLookup :
              if (++dISptr==dISsize) ssl_fatal("IS overflow");
              dIS[dISptr] = 0;
              for (dTemp=0; dTemp<dStrTableNext; dTemp++)
                if (!strcmp(ssl_strlit_buffer, dStrTable[dTemp].buffer)) {
                  dIS[dISptr] = dStrTable[dTemp].val;
                  break;
                }
              if (!dIS[dISptr])
                ssl_error("String shortform not defined");
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
       case oPatchPushIdent :
              if (++dPS[ssl_param].ptr==dPS[ssl_param].size)
                    ssl_fatal("patch overflow");
              dPS[ssl_param].stack[dPS[ssl_param].ptr] = ssl_last_id;
              continue;
       case oPatchPushValue :
              if (++dPS[ssl_param].ptr==dPS[ssl_param].size)
                    ssl_fatal("patch overflow");
              dPS[ssl_param].stack[dPS[ssl_param].ptr] = dVS[dVSptr];
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
              w_outTable[dPS[ssl_param].stack[dPS[ssl_param].ptr--]] =
                    id_table[dTemp].val;
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
       case oDocCheckpoint :
              printf("Checkpoint %d (sp=%d) ",ssl_pc-1, ssl_sp);
              /* t_printToken(); */
              continue;

       /* Mechanism include_mech */

       case oInclude :
              ssl_include_filename (ssl_strlit_buffer);
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


init_my_scanner ()
{
    my_special_codes.invalid = pInvalid;
    my_special_codes.eof     = pEof;
    my_special_codes.ident   = pIdent;
    my_special_codes.intlit  = pIntLit;
    my_special_codes.strlit  = pStrLit;

    ssl_init_scanner (my_keyword_table, my_operator_table, &my_special_codes);
}


w_dump_tables()
{
    short             i;
    short             count;
    struct rule_addr
    {
        char      *name;
        short      addr;
        struct rule_addr *next;
    };
    struct rule_addr *head, *curr, *prev, *new_rule;
    int               rule_table_size;
  
    printf ("\nWriting Files...\n");

    /* Build list of rule addresses, sorted by address (insertion sort) */

    rule_table_size = 0;
    head = NULL;
    for (i = 1; i < ssl_id_table_next; i++)
    {
        if (ssl_id_table[i].code == pIdent &&
            id_table[i].kind == kRule)
        {
            new_rule = (struct rule_addr *) malloc (sizeof(struct rule_addr));
            new_rule->name = ssl_id_table[i].name;
            new_rule->addr = id_table[i].val;
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
        fprintf (f_out, "short ssl_code_table[] = {\n\t");
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
    }

    /*  Write header file  */

    fprintf(f_hdr,"#define SSL_CODE_TABLE_SIZE %d\n\n", w_here+50);

    for (i=1; i<ssl_id_table_next; i++)
    {
        if (ssl_id_table[i].code == pIdent &&
            id_table[i].kind != kType &&
            id_table[i].kind != kMech &&
            id_table[i].kind != kRule)
        {
            fprintf(f_hdr,"#define %s %d\n", ssl_id_table[i].name,
                                             id_table[i].val);
        }
    }

    count = 0;
    fprintf(f_hdr,"\n#ifdef SSL_INCLUDE_ERR_TABLE\n");

    fprintf(f_hdr,"\nstruct ssl_error_table_struct ssl_error_table[] = {\n");
    for (i=1; i<ssl_id_table_next; i++)
    {
        if (ssl_id_table[i].code == pIdent &&
            id_table[i].kind == kError)
        {
            fprintf(f_hdr,"   \"%s\", %d,\n",ssl_id_table[i].name,
                                             id_table[i].val);
            count++;
        }
    }
    fprintf(f_hdr,"   \"\", 0\n};\nint ssl_error_table_size = %d;\n",count);
    fprintf(f_hdr,"\n#endif SSL_INCLUDE_ERR_TABLE\n");

}


/* Optimize generated table */

/* NOTE:  This function does something risky:  it uses definition of
 *        oJumpForward, etc., from the SSL compiler that built the
 *        SSL compiler, not from the definition of the current SSL
 *        compiler that generated the output table.
 */

w_optimize_table ()
{
    short  pc, target, final_target;
    short  options;
    short  improved_count;
    short  improved_case_jump_count;
    short  find_ultimate_destination();

    /*  keep track of when next choice table is coming up  */
    short  choice_table [dPatchCTAsize], choice_table_sp;

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
            case oJumpForward :
                    target = pc + w_outTable[pc];
                    final_target = find_ultimate_destination (target);
                    if (final_target != target)
                    {
                        improved_count++;
                        if (final_target > pc)
                        {
                            w_outTable [pc-1] = oJumpForward;
                            w_outTable [pc] = final_target - pc;
                        }
                        else
                        {
                            w_outTable [pc-1] = oJumpBack;
                            w_outTable [pc] = pc - final_target;
                        }
                    }
                    pc++;
                    continue;

            case oJumpBack :
                    target = pc - w_outTable[pc];
                    final_target = find_ultimate_destination (target);
                    if (final_target != target)
                    {
                        improved_count++;
                        if (final_target > pc)
                        {
                            w_outTable [pc-1] = oJumpForward;
                            w_outTable [pc] = final_target - pc;
                        }
                        else
                        {
                            w_outTable [pc-1] = oJumpBack;
                            w_outTable [pc] = pc - final_target;
                        }
                    }
                    pc++;
                    continue;

            case oInputChoice :
            case oChoice :
                    choice_table [++choice_table_sp] = pc + w_outTable[pc];
                    pc++;
                    continue;

            case oInput :
            case oEmit :
            case oError :
            case oCall :
            case oSetResult :
            case oSetParameter :
                    pc++;
                    continue;

            case oInputAny :
            case oReturn :
            case oEndChoice :
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

short find_ultimate_destination (pc)
short                      pc;
{
    while (1)
    {
        if (w_outTable[pc] == oJumpForward)
        {
            pc++;
            pc += w_outTable[pc];
        }
        else if (w_outTable[pc] == oJumpBack)
        {
            pc++;
            pc -= w_outTable[pc];
        }
        else
            break;
    }

    return (pc);
}


/*  Need this stub function for now, if integrating with debugger
    but not schema database  */

nodeDumpTreeNum ()
{
    printf ("Schema database not used in this program\n");
}

