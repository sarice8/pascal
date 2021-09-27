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
#include <stdlib.h>
#include <string.h>

#include "debug.h"


#define public
#define local  static

/*
*****************************************************************************
*
*  ssltool_init
*
*****************************************************************************
*/

void
ssltool_init( int argc, char** argv, char* source_filename, char* input_filename )
{
}


void
ssltool_at_line ( int line )
{
    printf ("[line %d]\n", line);
}


void
ssltool_at_input_position ( int line, int col )
{
    printf ("[input line %d col %d]\n", line, col);
}


void
ssltool_execution_status ( char* status_string )
{
}


void
ssltool_restart_input_window ( char* input_filename )
{
}


void
ssltool_main_loop ()
{
    cmdline_main_loop ();
}

