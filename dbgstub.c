/**/
static char sccsid[] = "%W% %G% %U% %P%";
/**/

/*
*****************************************************************************
*
*   Verilog Analyzer
*
*   by Steve Rice
*
*   March 19, 1991
*
*****************************************************************************
*
*   dbgstub.c            SSL Full Screen Debugger
*
*                        -- stubs to replace graphical user interface
*                           with a text interface
*
*   HISTORY
*----------------------------------------------------------------------------
*   12/21/92 | Steve  | First version
*
*****************************************************************************
*/

#include <stdio.h>

#define public
#define local  static

/*
*****************************************************************************
*
*  ssltool_init
*
*****************************************************************************
*/

public   ssltool_init (source_filename)
char                  *source_filename;
{
}

ssltool_at_line (line)
int              line;
{
    printf ("[line %d]\n", line);
}

ssltool_at_input_position (line, col)
int                        line;
int                        col;
{
    printf ("[input line %d col %d]\n", line, col);
}

ssltool_execution_status (status_string)
char                     *status_string;
{
}

ssltool_restart_input_window (input_filename)
char                         *input_filename;
{
}

ssltool_main_loop ()
{
    cmdline_main_loop ();
}

