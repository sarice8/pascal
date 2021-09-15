
/*
*  Test of SSL with variables
*/

#include <stdio.h>
#include <string.h>

#include "node.h"
#include "ssl_rt.h"
#include "debug.h"

#define  SSL_INCLUDE_ERR_TABLE
#include "test.h"

#include "test.tbl"

#define  DEBUG_FILE "test.dbg"
#define  PROGRAM_FILE "test.lst"

char     input_filename[256];

char     list_filename[256];

FILE    *f_lst;


/* Variables for semantic mechanisms */

dbg_variables debug_variables[] = {
    /* "Name", address,    udata,    function */
    "",        NULL,       0,        NULL,
};


/* Callbacks */
int my_listing_function ();
int init_my_operations ();


main (argc, argv)
int   argc;
char *argv[];
{
    int     arg = 1;
    int     status;

    if (argc < 2)
    {
        printf ("Usage: test <prog>.t\n");
        exit (1);
    }

    sprintf (input_filename, "%s.t", argv[arg]);
    sprintf (list_filename, "%s.lst", argv[arg]);

    open_my_files();
    ssl_init();
    init_my_scanner();

    ssl_set_debug(0);
    ssl_set_debug_info (DEBUG_FILE, PROGRAM_FILE, oBreak, debug_variables);

    ssl_set_input_filename (input_filename);

    ssl_set_listing_callback (my_listing_function);
    ssl_set_init_operations_callback (init_my_operations);

    status = ssl_run_program();

    close_my_files();
    exit (status);
}

open_my_files()
{
    if ((f_lst = fopen(list_filename, "w")) == NULL)
    {
        printf ("Can't open listing file %s\n", list_filename);
        exit (1);
    }
}

close_my_files()
{
    fclose(f_lst);
}

int my_listing_function (source_line, token_accepted)
char      *source_line;
int        token_accepted;
{
    if (token_accepted)
        fprintf (f_lst, "=== %s", source_line);
    else
        fprintf (f_lst, "    %s", source_line);
}

init_my_scanner()
{
    static struct ssl_token_table_struct keywords[] =
    {
        NULL,  0
    };

    static struct ssl_token_table_struct operators[] =
    {
        NULL,  0
    };

    static struct ssl_special_codes_struct special_codes;

    special_codes.invalid = pInvalid;
    special_codes.eof     = pEof;
    special_codes.ident   = pIdent;
    special_codes.intlit  = pIntLit;
    special_codes.strlit  = pStrLit;

    ssl_init_scanner (keywords, operators, &special_codes);
}

init_my_operations ()
{
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

#include "ssl_begin.h"

    /* Mechanism math */

    case oAdd:
        ssl_var_stack[ssl_argv(0,2)] += ssl_argv(1,2);
        continue;
    case print:
        printf ("%d ", ssl_param);
        continue;
    case TOKEN_VAL:
        ssl_result = ssl_token.val;
        continue;

#include "ssl_end.h"

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */


