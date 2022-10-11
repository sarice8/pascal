/**/ 
static char sccsid[] = "%W% %G% %U% %P%";
/**/

/*
*****************************************************************************
*
*   Syntax/Semantic Language Runtime Support Module
*
*   by Steve Rice
*
*   Aug 18, 1993
*
*****************************************************************************
*
*   ssl_rt.c            Runtime support (SSL interpreter)
*
*   HISTORY
*----------------------------------------------------------------------------
*   08/18/93 | Steve  | Based on SSL interpreter in my Schema compiler
*   08/27/93 | Steve  | RT 2.0, with value stack (parameters, local vars)
*            |        | 
*
*****************************************************************************
*/


#if DOCUMENTATION

/*
 ****************************************************************************
 *
 *   This is a generic module to allow programs to execute code created
 *   by the SSL compiler.
 *
 *
 *   Integration with an application:
 *   --------------------------------
 *
 *   - Application links with ssl_rt.o
 *
 *   - Application defines operations in a file like this:
 *
 *     #include "program.h"  /* Generated by ssl compiler */
 *     #include "ssl_rt.h"
 *
 *     <functions and variables for semantic operations>
 *
 *     #include "ssl_begin.h"
 *
 *         case <operation> :
 *             operation code, referring to ssl_param, ssl_argv(arg,arc), ssl_result,
 *                             ssl_error, ssl_fatal, ssl_assert, ssl_warning
 *             continue;
 *
 *     #include "ssl_end.h"
 *         
 *   - Application should provide code to handle the built-in operation
 *     oEmit, including any data structures required (no output table is
 *     provided by ssl_rt).
 *     
 *   - Application defines ssl code table in either of these ways:
 *
 *         #include "program.tbl"   /* Code table C file generated by
 *                                     SSL compiler */
 *     or
 *         #include "program.h"
 *         short ssl_code_table [SSL_CODE_TABLE_SIZE];  /* Application defines table space */
 *         int   ssl_code_table_size = SSL_CODE_TABLE_SIZE;
 *         char  ssl_title_string [256];                /*   and space for title. */
 *         char  ssl_runtime_vrs  [256];                /*   and space for runtime version. */
 *         ssl_load_program ("program.tbl");            /* ssl_rt loads table */
 *
 *   - In one of the application files, this sequence should be used,
 *     to define the error message table.
 *
 *     #define  SSL_INCLUDE_ERR_TABLE
 *     #include "program.h"
 *
 *
 ****************************************************************************
 */

#endif /* DOCUMENTATION */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef AMIGA
#include <dos.h>
#endif // AMIGA


/*  Trap fatal errors in debug version */
#include <setjmp.h>
jmp_buf  ssl_fatal_jmp_buf;

/*  Catch system error signals  */
#include <signal.h>

/*  For interfacing with SSL debugger  */
#include "debug.h"

/*  Definitions for scanner and this module  */
#include "ssl_rt.h"


/* 
 *
 * Variables for SSL table walker
 *
 */
 

/* Defined in ssl_rt.h for now */
/* #define  ssl_stackSize    200 */

short    ssl_stack [ssl_stackSize];       /* Call stack */
short    ssl_sp;

long     ssl_var_stack [ssl_var_stackSize];  /* Variable stack */
long     ssl_var_sp;                      /* Var stack sp */
long     ssl_var_fp;                      /* Var stack frame pointer, used by Rule calls */

short    ssl_pc;                          /* program counter                */
short    ssl_tmp;                         /* used for error recovery        */
long     ssl_result;                      /* value from oSetResult          */
short    ssl_choice_options;              /* number of choice options       */
char     ssl_error_buffer[256];           /* err message construction area  */
short    ssl_error_count;                 /* number of err signals emitted  */
int      ssl_walking;                     /* currently in walker            */
FILE    *ssl_src_file;                    /* application's input file       */


/*  Options set by application via ssl_set_* functions  */

char     ssl_input_filename [256];        /* application's input file       */
int      ssl_debug;                       /* debug mode                     */
int      ssl_case_sensitive;              /* scanner is case sensitive      */
void   (*ssl_listing_callback)();         /* func provided to list source   */
void   (*ssl_init_operations_callback)(); /* func provided to init operations */

short    ssl_recovery_token;              /* synch token for error recovery */
short    ssl_eof_token;                   /* EOF token for error recovery   */


/*  Information for the debugger  */

char             ssl_debug_file [256];    /* File containing debug data */
char             ssl_program_file [256];  /* File containing application listing */
dbg_variables   *ssl_debug_variables;     /* Operation variables and display funcs */
int              ssl_break_code;          /* oBreak opcode */


// Forward declarations

int ssl_validate_version ();
void ssl_init_handle_signals ();


/*  ----------------------------- Application Functions ----------------------- */

/*  Initialize ssl_rt module  */
/*  Should be called once at start of application  */

void
ssl_init ()
{
    strcpy (ssl_input_filename, "default_input");

    ssl_listing_callback = NULL;
    ssl_init_operations_callback = NULL;

    ssl_src_file = NULL;

    ssl_debug = 0;
    ssl_case_sensitive = 1;

    /* Debug info */
    strcpy (ssl_debug_file, "");
    strcpy (ssl_program_file, "");
    ssl_debug_variables = NULL;
    ssl_break_code = 0;

    /* Catch system error signals */
    ssl_init_handle_signals ();
}



/* --------------- Set runtime options ------------- */

/*  SSL Debugger on/off  */
/*  Can be called any time during run, to enable/disable debugging. */

void
ssl_set_debug ( int debug_flag )
{
    ssl_debug = debug_flag;
}


/*  Debugging info must be provided before run if debug mode enabled
 *
 *      debug_file      - File containing debug data
 *      program_file    - File containing application program source
 *      break_code      - Value of oBreak opcode
 *      debug_variables - Operation variables and display functions
 */

void
ssl_set_debug_info ( const char* debug_file, const char* program_file, int break_code, dbg_variables* debug_variables )
{
    strcpy (ssl_debug_file, debug_file);
    strcpy (ssl_program_file, program_file);
    ssl_break_code = break_code;
    ssl_debug_variables = debug_variables;
}


/*  Provide application input file, for scanner and debugger.
 *  Takes effect at next ssl_restart()
 */
void
ssl_set_input_filename ( const char* input_filename )
{
    strcpy (ssl_input_filename, input_filename);
}


void
ssl_set_case_sensitive ( int case_sensitive )
{
    ssl_case_sensitive = case_sensitive;
}


void
ssl_set_listing_callback ( void (*listing_callback)() )
{
    ssl_listing_callback = listing_callback;
}


void
ssl_set_init_operations_callback ( void (*init_operations_callback)() )
{
    ssl_init_operations_callback = init_operations_callback;
}


void
ssl_set_recovery_token ( short recovery_token )
{
    ssl_recovery_token = recovery_token;
}


/* --------------- Execute program ----------------- */

/*  Begin execution of ssl program.
 *  Returns 0 if the program completed successfully.
 */

int ssl_run_program ()
{
    int         status;

    status = ssl_validate_version ();
    if (status != 0)
        return (-1);

    status = ssl_restart ();    /* Initialize inputs, operations */
    if (status != 0)
    {
        printf ("SSL: error starting session\n");   /* Dumb message */
        return (-1);
    }

    if (ssl_debug)
        dbg_walkTable();
    else
        ssl_walkTable();


    /* program has completed */

    if (ssl_error_count)
        printf ("\n%d error(s)\n", ssl_error_count);

    ssl_cleanup();

    if (ssl_error_count)
        return (-1);

    return (0);
}


/*  Returns 0 if successful */

int ssl_restart ()
{

    if ((ssl_src_file=fopen(ssl_input_filename, "r"))==NULL)
    {
        printf ("SSL: Can't open source file %s\n", ssl_input_filename);
        return (-1);
    }

    /* restart scanner */

    ssl_restart_scanner();


    /* initialize table walker */

    ssl_pc = 0;
    ssl_sp = 0;
    ssl_error_count = 0;
    ssl_walking = 0;


    /* initialize semantic operations */

    if (ssl_init_operations_callback)
        (*ssl_init_operations_callback)();


    /* initialize debugger */

    if (ssl_debug)
    {
        dbg_init (ssl_debug_file, ssl_program_file, ssl_input_filename, ssl_break_code,
                  ssl_debug_variables);
    }

    printf ("%s\n", ssl_title_string);        /* print title */

    return (0);
}


void
ssl_cleanup()
{
    if (ssl_src_file != NULL)
        fclose (ssl_src_file);
}


/*  Read generated code of ssl program into ssl_code_table.
 *
 *  Called by application that does not directly include C version of
 *  generated code table.
 *
 *  Returns 0 on success.
 */

int
ssl_load_program ( const char* program_filename )
{
    FILE   *program_file;
    int     entries;
    int     temp;
    int     addr;
    char    tmp_buffer[256];

    if ((program_file = fopen(program_filename, "r"))==NULL)
    {
        printf ("SSL: Can't open program file %s\n", program_filename);
        return (-1);
    }

    char* ok = fgets (ssl_title_string, 256, program_file);
    assert( ok );

    ok = fgets (ssl_runtime_vrs,  256, program_file);
    assert( ok );
    ssl_runtime_vrs[strlen(ssl_runtime_vrs)-1] = '\0';   /* Remove trailing \n */

    int read = fscanf (program_file,"%d",&entries);       /* size of SSL code */
    assert( read == 1 );

    /*  NEED A PROTOCOL FOR THIS  */
    ssl_code_table_size = entries;

    for (addr=0; addr < entries; addr++)
    {
        read = fscanf (program_file, "%d", &temp);
        assert( read == 1 );
        ssl_code_table[addr] = temp;
    }

    /*  Get rule addresses  */

    /*  Note, at the moment the application must also provide the
        ssl_rule_table[] global array.  The C table form is a global array
        and we need to be consistent. */

    read = fscanf (program_file, "%d", &entries);
    assert( read == 1 );

    for (ssl_rule_table_size = 0; ssl_rule_table_size < entries; ssl_rule_table_size++)
    {   
        read = fscanf (program_file, "%s %d", tmp_buffer, &temp);
        assert( read == 2 );
        ssl_rule_table[ssl_rule_table_size].addr = temp;
        ssl_rule_table[ssl_rule_table_size].name = (char *) strdup (tmp_buffer);
    }

    fclose (program_file);
}


/*  ------------ Internal functions to support execution ------------- */

void
ssl_error_signal ( short error_code )
{
    short i;

    i = error_code;
    if (ssl_error_table[i].val != i)
    {
        for (i=0; i < ssl_error_table_size && ssl_error_table[i].val != error_code; i++);
    }

    sprintf (ssl_error_buffer, "Error: %s", ssl_error_table[i].msg);
    ssl_error(ssl_error_buffer);
}


void
ssl_assert_fun ( int expr, int line_num )
{
    char   buffer[100];

    if (!expr)
    {
        sprintf (buffer, "SSL: Assertion error in semantic operation at line %d", line_num);
        ssl_fatal (buffer);
    }
}


void
ssl_traceback()
{
    short i;

    printf("SSL Traceback:\n");
    printf("%d\t%s\n", ssl_pc, ssl_rule_name(ssl_pc));
    for (i=ssl_sp; i>0; i--)
    {
        printf("%d\t%s\n", ssl_stack[i]-2, ssl_rule_name(ssl_stack[i]-2));
    }
    printf("\n");
}


/*  Verify that version of runtime model supported by this runtime module
 *  (SSL_RUNTIME_VRS) matches that assumed by the SSL compiler (ssl_runtime_vrs).
 *
 *  Aspects of the runtime model include:
 *      system instruction set,
 *      method of variable and parameter addressing
 *
 *  If any of these change, then a new version of the runtime model is
 *  created, and must be supported by new matching versions of the SSL compiler
 *  and runtime module (ssl_rt).
 *
 *  Return 0 for success.
 */
int
ssl_validate_version ()
{
    int     status = 0;
    char   *ssl_rt_runtime_vrs = SSL_RUNTIME_VRS;

    if (ssl_runtime_vrs[0] == '\0')
    {
        /* User didn't provide a runtime version,
         * in the case they used ssl_load_program().
         * Assume its okay.
         */
        status = 0;
    }
    else if (strcmp (ssl_runtime_vrs, ssl_rt_runtime_vrs) != 0)
    {
        printf ("SSL: This program has been linked with the wrong runtime module.\n");
        printf ("     SSL program requires runtime version:  %s\n", ssl_runtime_vrs);
        printf ("     Runtime module is for runtime version: %s\n", ssl_rt_runtime_vrs);
        status = -1;
    }
    else
    {
        status = 0;
    }

    return (status);
}


/*  ------------------- Can be called from semantic operations ----------- */

void
ssl_fatal ( const char* msg )
{
    char tmpbuf[256];

    sprintf (tmpbuf, "[FATAL] %s", msg);
    ssl_error (tmpbuf);
    ssl_abort ();
}


void
ssl_abort ()
{
    ssl_traceback();
    ssl_cleanup();

    if (ssl_debug && ssl_walking)
        longjmp (ssl_fatal_jmp_buf, 1);

    exit(5);
}


void
ssl_print_input_position ()
{
  char *p,spaceBuf[256];
  short col;

  printf("%s",ssl_line_buffer);
  for (p = spaceBuf, col = 0; col < ssl_token.colNumber; col++)
  {
    if (ssl_line_buffer[col] == '\t')
        *p++ = '\t';
    else
        *p++ = ' ';
  }
  *p = '\0';
  printf("%s^\n",spaceBuf);
}


void
ssl_error ( const char* msg )
{
    ssl_error_count++;

    ssl_print_input_position ();

    printf("%s on line %d\n",msg,ssl_token.lineNumber);
    /* ssl_print_token(); */
}


void
ssl_warning ( const char* msg )
{
    ssl_print_input_position ();
    printf ("%s on line %d\n", msg, ssl_token.lineNumber);
}


void
ssl_print_token()
{
#if 0
  printf(" (Token=");
  if (ssl_token.code==pIDENTIFIER) {
    printf("%s)\n",ssl_token.name);
  } else if (ssl_token.code==pINVALID) {
    printf("'%s')\n",ssl_token.name);
  } else if (ssl_token.code == pSTRLIT) {
    printf("'%s')\n", ssl_strlit_buffer);
  } else {
    printf("<%d>)\n",ssl_token.code);
  }
#endif /* 0 */
}



/*  ------------------- For use by debugger ------------------- */

const char*
ssl_rule_name ( short pc )
{
    short    i;

    for (i = 1; i < ssl_rule_table_size; i++)
    {
        if (pc < ssl_rule_table[i].addr)
            return (ssl_rule_table[i-1].name);
    }
    return (ssl_rule_table[i-1].name);
}


short
ssl_rule_addr ( const char* name )
{
    short    i;

    for (i = 0; i < ssl_rule_table_size; i++)
    {
        if (strcmp (name, ssl_rule_table[i].name) == 0)
            return (ssl_rule_table[i].addr);
    }
    printf ("Unknown rule name: '%s'\n", name);
    return ((short) -1);
}


/*  ------------------- For use by debugger ------------------- */

/*  Catch system errors possibly caused by semantic operations  */

void
ssl_handle_signal_segv ( int sig )
{
    ssl_fatal ("System segmentation violation");
}


void
ssl_handle_signal_bus ( int sig )
{
    ssl_fatal ("System bus error");
}


void
ssl_init_handle_signals ()
{
    signal (SIGSEGV, ssl_handle_signal_segv);
    signal (SIGBUS,  ssl_handle_signal_bus);
}

