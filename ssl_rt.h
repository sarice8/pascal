/*
static char sccsid[] = "%W% %G% %U% %P%";
*/

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
*   ssl_rt.h            Runtime support module - Public header
*
*   HISTORY
*----------------------------------------------------------------------------
*   08/18/93 | Steve  | Based on SSL interpreter in my Schema compiler
*            |        |  
*
*****************************************************************************
*/

#ifdef AMIGA
#include <string.h>
#define bzero(p,n)   memset(p,'\0',n)
#endif AMIGA

#define SSL_RUNTIME_VRS "RT 1.1 - Original SSL model, generic ssl_rt - 08/18/93"
#if 0
#define SSL_RUNTIME_VRS "RT 1.0 - Original SSL Runtime Model - 09/07/89"
#endif /* 0 */



/*
 * --------------------------------------------------------------------------
 *                    Variables supplied by application
 * --------------------------------------------------------------------------
 */

extern short  ssl_code_table [];
extern int    ssl_code_table_size;
extern char   ssl_title_string [];
extern char   ssl_runtime_vrs [];

struct ssl_rule_table_struct
{
    char  *name;
    short  addr;
};
extern  struct ssl_rule_table_struct ssl_rule_table [];
extern  int    ssl_rule_table_size;

struct ssl_error_table_struct
{
    char  *msg;
    short  val;
};
extern  struct ssl_error_table_struct ssl_error_table [];
extern  int    ssl_error_table_size;


/***  Variables supplied to scanner by application  ***/

/*  Used to define keywords and operators  */
struct ssl_token_table_struct
{
    char  *name;
    short  code;
};

/*  Used to define special token codes  */
struct ssl_special_codes_struct
{
    short  invalid;    /* invalid token */
    short  eof;        /* end of file */
    short  ident;      /* identifier */
    short  intlit;     /* integer literal */
    short  strlit;     /* string literal */
};

/*
 * --------------------------------------------------------------------------
 *                            Public Functions            
 * --------------------------------------------------------------------------
 */

/*
 *  This should be called once at the start of the application
 *  before making any other ssl_* calls.
 */
ssl_init ();

/*
 *  Load a compiled SSL program.  The application only needs this call
 *  if it does not directly include the C version of the program table.
 *
 *  The application must define the table variable in its own file:
 *
 *  short ssl_code_table [ssl_code_table_size];
 *  char  ssl_title_string [256];
 *  char  ssl_runtime_vrs [256];
 *
 *  Returns 0 on success.
 */
int ssl_load_program (/* char *program_filename */);

/*
 *  Turn debugger on/off, any time during run.
 */
ssl_set_debug (/* int debug_flag */);

/*
 *  Debugging info must be provided before the run, if
 *  debug mode is turned on.
 *
 *      debug_file      - File containing debug data
 *      program_file    - File containing application program source
 *      break_code      - Value of oBreak opcode
 *      debug_variables - Operation variables and display functions
 */
ssl_set_debug_info (/* char *debug_file, char *program_file,
                       int break_code, dbg_variables *debug_variables */);

/*
 *  Provide the application's input file.
 *  Takes effect at the next ssl_run_program() or ssl_restart().
 */
ssl_set_input_filename (/* char *input_filename */);

/*
 *  Determine whether the scanner is case sensitive when matching
 *  identifiers and keywords.  The default is true.
 *
 *  Currently this function can be called any time, even after
 *  ssl_init_scanner().  However in the future we might require
 *  the call to be made before ssl_init_scanner(). (Or actually,
 *  changing this to a flag passed to ssl_init_scanner).
 */
ssl_set_case_sensitive (/* int yes_no */);

/*
 *  Register a callback to list a source line when the first token
 *  in the line is accepted.  The function will be called as:
 *       (*listing_callback) (char *line, int token_accepted);
 *  Note, token_accepted == FALSE for blank/comment lines.
 */
ssl_set_listing_callback (/* int (*listing_callback)() */);

/*
 *  Register a function to initialize application semantic operations
 *  at the begining of program execution or ssl_restart().
 *  NOTE, guaranteed to be called -after- ssl_restart_scanner()
 *  so this function may define additional identifiers, etc.
 */
ssl_set_init_operations_callback (/* int (*init_operations_callback)() */);
 
/*
 *  Set the synchronization token used in error recovery
 *  (for example, pSemicolon)
 */
ssl_set_recovery_token (/* short recovery_token */);


/*
 *  Initialize the scanner module
 */
ssl_init_scanner (/* struct ssl_token_table_struct *keyword_table,
                     struct ssl_token_table_struct *operator_table,
                     struct ssl_special_codes_struct *special_codes */);

/*
 *  Include another source file
 */
ssl_include_filename (/* char *filename */);


/*
 *
 *  Execute the SSL program.
 *
 *  Returns 0 if the program completed successfully.
 *
 */
int ssl_run_program ();

/*
 * --------------------------------------------------------------------------
 *               Functions for use in semantic operations
 * --------------------------------------------------------------------------
 */

/*
 *  Report a fatal message and abort.
 */
ssl_fatal (/* char *msg */);

/*
 *  Abort.
 */
ssl_abort ();

/*
 *  Report an error message, increment the error count, and
 *  continue normally (without entering error recovery mode).
 */
ssl_error (/* char *msg */);

/*
 *  Abort if assertion fails.
 */
#define  ssl_assert(expr)  ssl_assert_fun((expr),__LINE__)

/*
 *  Scanner functions which may be used by operations.
 */

/*  Add an identifier name to the scanner's identifier table.
 *  Code is the token code (typically pIDENTIFIER).
 *  Returns the identifier value.
 */
short ssl_add_id (/* char *name, short code */);

/*
 *  Return the name of a given identifier.
 */
char *ssl_get_id_string (/* long id */);

/*
 *  Return the current line, col in the input source.
 *  (Used by the debugger)
 */
ssl_get_input_position (/* short *line_ptr, short *col_ptr */);

/*
 * --------------------------------------------------------------------------
 *                           Private functions         
 * --------------------------------------------------------------------------
 */

/*  I think these are only required by the debugger module.
 *  In which case they can be moved to a private header.
 */

ssl_restart ();
ssl_cleanup ();

char *ssl_rule_name (/* short addr */);
short ssl_rule_addr (/* char *name */);

/*  In scanner */
char *ssl_get_code_name (/* short code */);
ssl_accept_token ();

/*
 * --------------------------------------------------------------------------
 *                        Private types & variables
 * --------------------------------------------------------------------------
 */

/*  These definitions are provided for the built-in SSL operations included
    in the application code.  The application itself should not use these.  */

/*  --- ssl_rt ---  */

extern short    ssl_recovery_token;
extern short    ssl_eof_token;

extern int      ssl_debug;

extern short    ssl_pc;
extern short    ssl_tmp;
extern short    ssl_result;
extern short    ssl_param;
extern short    ssl_choice_options;
extern char     ssl_error_buffer[];
extern int      ssl_walking;

extern short    ssl_sp;
extern short    ssl_stack [];
#define         ssl_stackSize  200     /* Need to define elsewhere */

/*  Should set this in ssl_rt.c to avoid need for <setjmp.h> in application */
#include <setjmp.h>
extern jmp_buf  ssl_fatal_jmp_buf;

extern int    (*ssl_listing_callback)();

extern FILE    *ssl_src_file;
extern int      ssl_case_sensitive;


/*  --- Scanner --- */

extern char     ssl_line_buffer[];

#define SSL_NAME_SIZE     256
#define SSL_STRLIT_SIZE   256

struct ssl_token_struct
{
    char  accepted;
    char  name[SSL_NAME_SIZE+1];
    short namelen;
    short val;       /* id #, int lit, etc */
    short code;      /* input token code (pIdent, pIntLit, etc) */
    short lineNumber;
    short colNumber;
};
extern struct ssl_token_struct    ssl_token;

/* strlit token values are returned here rather than in ssl_token.name */
extern char   ssl_strlit_buffer [];

#define ssl_get_token()  { if (ssl_token.accepted) ssl_get_next_token(); }

extern short ssl_line_number;  /* Current line of scanner */
extern char  ssl_line_listed;  /* Should be walker var, not scanner */
extern short ssl_last_id;      /* Should be walker var, not scanner */

#define ssl_id_table_size      4000
struct ssl_id_table_struct
{
    char  *name;
    short  namelen;
    short  code;
};
extern struct ssl_id_table_struct ssl_id_table[];
extern short  ssl_id_table_next;


