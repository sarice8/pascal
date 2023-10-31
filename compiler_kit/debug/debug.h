#ifndef SSL_DBG_H
#define SSL_DBG_H

/**/
/* static char sccsid[] = "%W% %G% %U% %P%"; */
/**/

/*  vdebug.h  */

/*  Application Interface to debugger  */

#ifdef AMIGA
#include <string.h>
#define bzero(p,n)   memset(p,'\0',n)
#endif // AMIGA

typedef struct dbg_variables_struct {
    const char* name;    /* Name of variable for debug user */
    char  *address;      /* Location of variable */
    char  *udata;        /* User data passed to display func */
    void (*display_method)( void*, void* );  /* display func */
} dbg_variables;
 

/*  Set address field in dbg_variables to this, to register a
 *  generic display routine for a type (defined in SSL).
 *  The routine will be called with the address of the variable
 *  and the udata (just like a variable display method).
 */
#define DBG_TYPE_DISPLAY  ((char *)0x1)

void dbg_init ( const char* debug_data_file, const char* source_filename, const char* input_filename,
                int break_opcode, dbg_variables* debug_variables );
int dbg_command( char* command );
void dbg_hit_breakpoint();
int dbg_check_step_count();
int dbg_check_input_breakpoint( int input_line, int input_col );
void dbg_walkTable();

/*  Application Interface to generic debugger.
 *  This might be implemented by a command-line debugger, or a graphical debugger.
 *  Don't really want to put this interface in ssltool, since that's also just one implementation
 *  of the debugger (and also isn't migrated yet).  Should be renamed.
 */
void ssltool_init( int argc, char** argv, const char* source_filename, const char* input_filename );
void ssltool_at_line( int line );
void ssltool_at_input_position( int line, int col );
void ssltool_execution_status( const char* status_string );
void ssltool_restart_input_window( const char* input_filename );
void ssltool_main_loop();

// Internal
void cmdline_main_loop ();

#endif
