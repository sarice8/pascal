/**/ 
static char sccsid[] = "@(#)debug.c	1.5 4/21/94 13:07:52 /files/home/sim/sarice/compilers/debug/1.3.0/SCCS/s.debug.c";
/**/

/*
*****************************************************************************
*
*   SSL Debugger Module
*
*   by Steve Rice
*
*   March 19, 1991
*
*****************************************************************************
*
*   debug.c             SSL Full Screen Debugger
*
*   HISTORY
*----------------------------------------------------------------------------
*   02/09/90 | Steve  | First version of full-screen SSL debugger
*   04/24/91 |        | Adapted debugger for Verilog analyzer
*   06/02/91 |        | Added breakpoints to debugger
*   06/05/91 |        | Added "next" command to debugger
*   06/06/91 |        | Made this a general purpose SSL debugger.
*            |        | Moved from vmain.c to vdebug.c
*   06/08/93 |        | Moved into a generic module for any SSL program.
*   03/11/94 |        | Added step-into-by-line, replaced old step with stepi
*
*****************************************************************************
*/

#include <stdio.h>
#include <string.h>

#include "debug.h"

#define public
#define local  static


/* ----------------------------------------------------------------------- */
/*    SSL walker variables that must be visible to the debugger            */
/* ----------------------------------------------------------------------- */

#include "ssl_rt.h"


/* ----------------------------------------------------------------------- */
/*    Symbol table produced by SSL compiler (ssl 1.3.0 or greater)         */
/* ----------------------------------------------------------------------- */

#define DBG_SYMBOL_CLASS_type       1   /* No need to match schema definitions */
#define DBG_SYMBOL_CLASS_rule       2
#define DBG_SYMBOL_CLASS_global     3
#define DBG_SYMBOL_CLASS_local      4
#define DBG_SYMBOL_CLASS_inparam    5
#define DBG_SYMBOL_CLASS_outparam   6
#define DBG_SYMBOL_CLASS_inoutparam 7

char *dbg_symbol_class_names[] =   /* Same order as above */
{
    "",
    "nType",
    "nRule",
    "nGlobal",
    "nLocal",
    "nInParam",
    "nOutParam",
    "nInOutParam",
    NULL,
};

struct dbg_symbol_struct {
    int      num;       /* symbol reference number */
    char    *name;
    int      class;     /* symbol class (see above) */
    int      type_num;  /* reference number of type */
    struct dbg_symbol_struct *type;   /* type of symbol */
    int      value;     /* value/addr */
};

#define DBG_SYMBOL_TABLE_SIZE 3000
struct dbg_symbol_struct dbg_symbol_table[DBG_SYMBOL_TABLE_SIZE];
int dbg_symbol_table_last;

/* ----------------------------------------------------------------------- */
/*    Local debugger variables                                             */
/* ----------------------------------------------------------------------- */


#define        MAXARGS    10

local short   *dbg_line_table;            /* positions of lines in code     */

local int      dbg_trace;                 /* trace source file?             */

/* dbg_step_mode modes */
#define DBG_STEP_NONE        0
#define DBG_STEP_CONT        1    /* first step past bkpt as part of "cont" */
#define DBG_STEP_INSTRUCTION 2    /* stepi */
#define DBG_STEP_OVER        3    /* next */
#define DBG_STEP_INTO        4    /* step */

local int      dbg_step_mode;
local int      dbg_step_count;            /* steps to take                  */
local int      dbg_line_step_call_level;  /* for "next": orig stack level   */
local int      dbg_line_step_line;        /* for "next": orig line #        */


#define        DBG_MAX_BREAKPOINTS 10

struct dbg_breakpoint_struct {
    short      pc;         /* address of breakpoint */
    short      code;       /* normal code at that address */
    int        line;       /* stop at <line> if line != -1 */
    char      *in_rule;    /* stop in <rule> if rule name != NULL */
    int        input_line; /* stop at i<line> if line != -1 */
};

local struct dbg_breakpoint_struct dbg_breakpoint [DBG_MAX_BREAKPOINTS];

local int      dbg_num_breakpoints;
local int      dbg_breakpoints_installed;

local short    oBreak_opcode;

dbg_variables *dbg_vars;

/* "up", "down" commands can modify display context */
local short    display_pc;
local short    display_sp;   /* call stack */
local long     display_var_fp;

local display_source_for_pc ();
local display_context_vars ();

/*
*****************************************************************************
*
*  dbg_init
*
*  Description
*      Called to initialize the debugger.
*
*      Create debugger graphics.
*
*      Read the debug file produced by SSL (containing positions of
*      line numbers).
*
*      Initialize debugger variables.
*
*****************************************************************************
*/

public   dbg_init (debug_data_file, source_filename, input_filename, break_opcode,
                   debug_variables)
char              *debug_data_file;
char              *source_filename;
char              *input_filename;
short              break_opcode;
dbg_variables     *debug_variables;
{
    FILE      *fp;
    int        entries;
    int        line;
    int        addr;

    static     session_active = 0;   /* Graphics up? */

    if (session_active)
    {
        /* Graphics are already up, we just need to restart them
           (e.g. for a new run command) */
        dbgui_restart (input_filename);
        return;   /* Leave existing breakpoints, etc., intact */
    }
    else
    {
        session_active = 1;
    }

    /* Get debug information (line numbers) */

    if ((fp = fopen(debug_data_file ,"r")) == NULL)
    {
        printf ("Can't open debug file %s\n", debug_data_file);
        exit (10);
    }

    fscanf (fp, "%d", &entries);

    dbg_line_table = (short *) malloc ((entries+1) * sizeof(short));
    dbg_line_table[0] = entries;
    for (line = 1; line <= entries; line++)
    {
        fscanf (fp, "%d", &addr);
        dbg_line_table[line] = addr;
    }
    dbg_read_symbol_table (fp);
    fclose (fp);

    /*  Init debugger variables  */

    dbg_trace = 1;
    dbg_step_mode = DBG_STEP_INSTRUCTION;
    dbg_step_count = 0;
    dbg_num_breakpoints = 0;
    dbg_breakpoints_installed = 0;

    oBreak_opcode = break_opcode;

    dbg_vars = debug_variables;

    /* Bring up windows, init cmdline, callbacks, etc. */
    dbgui_init(source_filename, input_filename);
}


/*  Called at the beginning of each instruction.
    Returns TRUE if we have reached the end of the current run step-count */

public       dbg_check_step_count()
{
    short     line;

    if ((dbg_step_mode == DBG_STEP_NONE) || 
        ((dbg_step_mode == DBG_STEP_INSTRUCTION) && (dbg_step_count-- > 0)))
        return(0);

    /* step to next line or into call */
    if (dbg_step_mode == DBG_STEP_INTO)
    {
        line = dbg_find_line (ssl_pc);
        if (line == dbg_line_step_line)
            return(0);
        if (--dbg_step_count > 0)
            return(0);
    }

    /* 'cont' command requires initial step past breakpoint location,
       then install breakpoints & run free.  */
    if (dbg_step_mode == DBG_STEP_CONT)
    {
        if (dbg_step_count-- > 0)
            return(0);    /* about to do initial step */

        /* now ready to run free to next breakpoint */
        dbg_step_mode = DBG_STEP_NONE;
        dbg_install_breakpoints();
        return(0);
    }

    if (dbg_step_mode == DBG_STEP_OVER)    /* line step */
    {
        if (ssl_sp > dbg_line_step_call_level)
            return(0);
        line = dbg_find_line (ssl_pc);
        if (line == dbg_line_step_line)
            return(0);
    }

    return(1);
}


/*  Called at the beginning of each input line.
    Returns TRUE if we have hit an input-line breakpoint. */

public       dbg_check_input_breakpoint (input_line, input_col)
short                                    input_line, input_col;
{
    int  i;

    for (i = 0; i < dbg_num_breakpoints; i++)
    {
        if (dbg_breakpoint[i].input_line == input_line)
            return (1);
    }
    return (0);
}


/*
*****************************************************************************
*
*  dbg_command
*
*  Description
*      Main debugger callback.
*      Returns TRUE if the user wishes to quit the debugger.
*
*****************************************************************************
*/

public        dbg_command (command)
char                      *command;
{
    short     line;
    int       argc;
    char     *argv [MAXARGS];
    int       i;
    int       v;
    long      node_number;

    dbg_step_mode = DBG_STEP_INSTRUCTION;

    split_args (command, &argc, argv);

    if (argc == 0)
        return(0);
    else if ((strcmp(argv[0],"s") == 0) || (strcmp(argv[0],"step") == 0))
    {
        dbg_step_mode = DBG_STEP_INTO;
        dbg_step_count = 1;                 /* single-step */
        dbg_line_step_line = dbg_find_line(ssl_pc);

        dbg_run();
        dbg_display_position();
    }
    else if ((strcmp(argv[0],"n") == 0) || (strcmp(argv[0],"next") == 0))
    {
        dbg_step_mode = DBG_STEP_OVER;      /* next line */
        dbg_line_step_call_level = ssl_sp;
        dbg_line_step_line = dbg_find_line(ssl_pc);

        dbg_run();
        dbg_display_position();
    }
    else if ((strcmp(argv[0],"si") == 0) || (strcmp(argv[0],"stepi") == 0))
    {
        dbg_step_mode = DBG_STEP_INSTRUCTION;
        dbg_step_count = 1;                 /* single-step */

        dbg_run();
        dbg_display_position();
    }
    else if (strcmp(argv[0],"run") == 0)
    {
        ssl_cleanup();
        ssl_restart();    /* Restart session with same source, breakpoints */

        dbg_step_mode = DBG_STEP_NONE;   /* Restart & run automatically */

        dbg_install_breakpoints();
        dbg_run();
        dbg_remove_breakpoints();
        dbg_display_position();
    }
    else if ((strcmp(argv[0],"c") == 0) || (strcmp(argv[0],"cont") == 0))
    {
        dbg_step_mode = DBG_STEP_CONT;
        dbg_step_count = 1;   /* Take one step before installing bkpts */
                              /* Bkpts installed by dbg_check_step_count() */
        dbg_run();
        dbg_remove_breakpoints();
        dbg_display_position();
    }
    else if (strcmp(argv[0],"trace") == 0)
    {
        if (argc < 2)
        {
            dbg_trace = !dbg_trace;
            printf ("trace is now %s\n", dbg_trace ? "on" : "off");
        }
        else if (strcmp(argv[1],"on") == 0)
            dbg_trace = 1;
        else if (strcmp(argv[1],"off") == 0)
            dbg_trace = 0;
        else
            printf ("invalid argument\n");
    }
    else if (strcmp(argv[0],"quit") == 0)
    {
        return(1);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        if (argc != 3)
        {
            printf ("Invalid argument(s)\n");
            return(0);
        }

        if (dbg_num_breakpoints >= DBG_MAX_BREAKPOINTS)
        {
            printf ("Breakpoint table full\n");
            return(0);
        }

        if (strcmp(argv[1], "at") == 0)
        {
            if (argv[2][0] == '#')         /* stop at #<address> */
            {
                i = atoi(argv[2]+1);
                if (i < 0 || i >= ssl_code_table_size)
                {
                    printf ("Illegal address %d\n", i);
                    return(0);
                }
                dbg_breakpoint[dbg_num_breakpoints].in_rule = NULL;
                dbg_breakpoint[dbg_num_breakpoints].line = -1;
                dbg_breakpoint[dbg_num_breakpoints].pc = i;
                dbg_breakpoint[dbg_num_breakpoints].code = ssl_code_table[i];
                dbg_breakpoint[dbg_num_breakpoints].input_line = -1;
                dbg_num_breakpoints++;
            }
            else if (argv[2][0] == 'i')         /* stop at i<input_line> */
            {
                i = atoi(argv[2]+1);
                if (i < 1)
                {
                    printf ("Illegal input line %d\n", i);
                    return(0);
                }
                dbg_breakpoint[dbg_num_breakpoints].in_rule = NULL;
                dbg_breakpoint[dbg_num_breakpoints].line = -1;
                dbg_breakpoint[dbg_num_breakpoints].pc = -1;
                dbg_breakpoint[dbg_num_breakpoints].code = 0;
                dbg_breakpoint[dbg_num_breakpoints].input_line = i;
                dbg_num_breakpoints++;
            }
            else                           /* stop at <line> */
            {
                line = atoi(argv[2]);
                if (line < 1 || line > dbg_line_table[0])
                {
                    printf ("Illegal line number %d\n", line);
                    return(0);
                }
                i = dbg_line_table[line];
                dbg_breakpoint[dbg_num_breakpoints].pc = i;
                dbg_breakpoint[dbg_num_breakpoints].line = line;
                dbg_breakpoint[dbg_num_breakpoints].in_rule = NULL;
                dbg_breakpoint[dbg_num_breakpoints].code = ssl_code_table[i];
                dbg_breakpoint[dbg_num_breakpoints].input_line = -1;
                dbg_num_breakpoints++;
            }
        }
        else if (strcmp(argv[1], "in") == 0)
        {
            i = ssl_rule_addr (argv[2]);
            if (i < 0)
                return(0);
            dbg_breakpoint[dbg_num_breakpoints].in_rule = ssl_rule_name(i);
            dbg_breakpoint[dbg_num_breakpoints].pc = i;
            dbg_breakpoint[dbg_num_breakpoints].code = ssl_code_table[i];
                dbg_breakpoint[dbg_num_breakpoints].input_line = -1;
            dbg_num_breakpoints++;
        }
        else
        {
            printf ("Invalid argument '%s' (expecting 'at' or 'in')\n", argv[1]);
            return(0);
        }
        
        for (i = 0; i < dbg_num_breakpoints; i++)
        {
            if (dbg_breakpoint[i].in_rule != NULL)
                printf ("(%d) stop in %s\n", i+1, dbg_breakpoint[i].in_rule);
            else if (dbg_breakpoint[i].line != -1)
                printf ("(%d) stop at %d\n", i+1, dbg_breakpoint[i].line);
            else if (dbg_breakpoint[i].input_line != -1)
                printf ("(%d) stop at i%d [input line]\n", i+1, dbg_breakpoint[i].input_line);
            else
                printf ("(%d) stop at #%d\n", i+1, dbg_breakpoint[i].pc);
        }
    }
    else if ((strcmp(argv[0], "delete") == 0) || (strcmp(argv[0], "clear") == 0))
    {
        if (argc != 2)
        {
            printf ("Invalid argument(S)\n");
            return(0);
        }
        i = atoi(argv[1]) - 1;
        if (i >= dbg_num_breakpoints || i < 0)
        {
            printf ("Invalid breakpoint number\n");
            return(0);
        }
        dbg_num_breakpoints--;
        dbg_breakpoint[i].pc = dbg_breakpoint[dbg_num_breakpoints].pc;
        dbg_breakpoint[i].code = dbg_breakpoint[dbg_num_breakpoints].code;
        dbg_breakpoint[i].in_rule = dbg_breakpoint[dbg_num_breakpoints].in_rule;
        dbg_breakpoint[i].line = dbg_breakpoint[dbg_num_breakpoints].line;
        dbg_breakpoint[i].input_line = dbg_breakpoint[dbg_num_breakpoints].input_line;
    }
    else if (strcmp(argv[0], "status") == 0)
    {
        for (i = 0; i < dbg_num_breakpoints; i++)
        {
            if (dbg_breakpoint[i].in_rule != NULL)
                printf ("(%d) stop in %s\n", i+1, dbg_breakpoint[i].in_rule);
            else if (dbg_breakpoint[i].line != -1)
                printf ("(%d) stop at %d\n", i+1, dbg_breakpoint[i].line);
            else if (dbg_breakpoint[i].input_line != -1)
                printf ("(%d) stop at i%d [input line]\n", i+1, dbg_breakpoint[i].input_line);
            else
                printf ("(%d) stop at #%d\n", i+1, dbg_breakpoint[i].pc);
        }
    }
    else if (strcmp(argv[0], "where") == 0)
    {
        printf("%d\t%s", ssl_pc, ssl_rule_name(ssl_pc));
        for (i = ssl_sp; i > 0; i--)
        {
            printf("\tcalled from\n%d\t%s",
                   ssl_stack[i]-2, ssl_rule_name(ssl_stack[i]-2));
        }
        printf ("\n");
    }
    else if (strcmp(argv[0], "display") == 0)
    {
        for (v = 0; dbg_vars[v].display_method != NULL; v++)
        {
            if (dbg_vars[v].address == DBG_TYPE_DISPLAY)
                continue;   /* This is a type display routine, not a variable */
            printf ("%s:\n", dbg_vars[v].name);
            (*dbg_vars[v].display_method) (dbg_vars[v].address, dbg_vars[v].udata);
        }
        printf ("--------------------\n");
        display_context_vars (display_pc, display_var_fp, 1, NULL);
        printf ("--------------------\n");
    }
    else if (strcmp(argv[0], "dump") == 0)
    {
        display_context_vars (display_pc, display_var_fp, 0, NULL);
        printf ("--------------------\n");
    }
    else if (strcmp(argv[0], "up") == 0)
    {
        if (display_sp > 0)
        {
            display_pc = ssl_stack[display_sp]-2;
            display_sp--;
            display_var_fp = ssl_var_stack[display_var_fp];
            display_source_for_pc (display_pc);
        }
    }
    else if (strcmp(argv[0], "down") == 0)
    {
        if (display_sp < ssl_sp)
        {
            display_sp++;
            if (display_sp == ssl_sp)
                display_pc = ssl_pc;
            else
                display_pc = ssl_stack[display_sp+1]-2;

            /* Can only find lower frame by searching from last frame */
            display_var_fp = ssl_var_fp;
            for (i = ssl_sp - display_sp; i > 0; i--)
                display_var_fp = ssl_var_stack[display_var_fp];

            display_source_for_pc (display_pc);
        }
    }
    else if (strcmp(argv[0], "p") == 0)
    {
        if ((argv[1][0] >= '0') && (argv[1][0] <= '9'))
        {
            node_number = atoi(argv[1]);
            nodeDumpTreeNum (node_number);  /* TO DO, remove from debugger */
            printf ("_________________________________________________________________________\n");
        }
        else
        {
            /* Display any internal variable by that name */
            for (v = 0; dbg_vars[v].display_method != NULL; v++)
            {
                if (dbg_vars[v].address == DBG_TYPE_DISPLAY)
                    continue;   /* This is a type display routine, not a variable */
                if (strcmp(dbg_vars[v].name, argv[1]) == 0)
                {
                    printf ("%s:\n", dbg_vars[v].name);
                    (*dbg_vars[v].display_method) (dbg_vars[v].address, dbg_vars[v].udata);
                }
            }

            /* Display any variable in SSL program by that name */
            display_context_vars (display_pc, display_var_fp, 1, argv[1]);

            printf ("--------------------\n");
        }
    }
    else
    {
        printf ("-------------- commands: ---------------\n");
        printf ("s               (step)\n");
        printf ("n               (next line)\n");
        printf ("stop at <line>  (set breakpoint at program line)\n");
        printf ("stop at i<line> (set breakpoint at input line)\n");
        printf ("stop at #<addr> (set breakpoint at code address)\n");
        printf ("stop in <rule>  (set breakpoint in program rule)\n");
        printf ("delete n        (delete a breakpoint)\n");
        printf ("status          (display all breakpoints)\n");
        printf ("run n           (execute n instructions)\n");
        printf ("c               (continue until breakpoint)\n");
        printf ("where           (display call stack)\n");
        printf ("trace on|off    (trace SSL code in TxEd1 window)\n");
        printf ("display         (display all variables)\n");
        printf ("p n             (print node <n>)\n");
        printf ("quit            (quit program)\n");
    }

    return (0);
}


/*  Run up to end of program, step count, or breakpoint.
    Returns 1 = program finished  */

dbg_run ()
{
    int   status;
    int   done;

    done = 0;

    status = ssl_walkTable();     /* walk table until breakpoint, step-count, or end */

    if (status == 0)
    {
        dbg_execution_complete();  /* done program */
        done = 1;
    }
    else if (status == 1)
    {
        dbg_hit_breakpoint();   /* hit breakpoint */
    }
    else if (status == 2)       /* reached end of step-count */
    {
    }
    else if (status == 3)       /* hit input-line breakpoint */
    {
        dbg_hit_input_breakpoint();
    }
    else
    {
        printf ("debug: unknown run status %d\n", status);
    }

    return (done);
}

/* Display current position */
dbg_display_position ()
{
    short     line;
    char      buffer[50];
    short     input_line, input_col;

    /* Reset displayed context for up/down commands */
    display_pc = ssl_pc;
    display_sp = ssl_sp;
    display_var_fp = ssl_var_fp;

    if (dbg_trace)
    {
        /* Move source window to current position */

        display_source_for_pc (ssl_pc);

        ssl_get_input_position (&input_line, &input_col);
        dbgui_at_input_position (input_line, input_col);

        sprintf (buffer, "pc: %d", ssl_pc);
        dbgui_execution_status (buffer);
    }
}

local display_source_for_pc (pc)
short                        pc;
{
    short     line;

    line = dbg_find_line (pc);
    dbgui_at_line (line);
}


local    dbg_install_breakpoints()
{
    int i;
    int pc;

    for (i = 0; i < dbg_num_breakpoints; i++)
    {
        pc = dbg_breakpoint[i].pc;
        if (pc == -1)
            continue;     /* No breakpoint address (e.g. break at input position) */

        if (ssl_code_table[pc] != oBreak_opcode)
        {
            dbg_breakpoint[i].code = ssl_code_table[pc];
            ssl_code_table[pc] = oBreak_opcode;
        }
    }
    dbg_breakpoints_installed = 1;
}

local   dbg_remove_breakpoints()
{
    int i;

    for (i = 0; i < dbg_num_breakpoints; i++)
    {
        if (dbg_breakpoint[i].pc == -1)
            continue;     /* No breakpoint address (e.g. break at input position) */

        ssl_code_table[dbg_breakpoint[i].pc] = dbg_breakpoint[i].code;
    }
    dbg_breakpoints_installed = 0;
}


local   split_args (cmdbuf, argc, argv)
char               *cmdbuf;
int                *argc;
char              **argv;
{
  char *p;

  *argc = 0;

  p = cmdbuf;
  while (1)
  {
    while (*p == ' ' || *p == '\t' || *p == '\n')
      p++;
    if (*p == '\0')
      break;
    argv[*argc] = p;
    (*argc)++;
    while (*p && *p != ' ' && *p != '\t' && *p != '\n')
      p++;
    if (*p == '\0')
      break;
    *p = '\0';
    p++;
  }

}


/*
*****************************************************************************
*
*  dbg_hit_breakpoint
*
*  Description
*      Called by the SSL walker when an oBreak instruction is hit.
*      This function removes all breakpoints, then returns to the
*      walker main loop in single step mode, thus entering dbg_command().
*
*****************************************************************************
*/

public    dbg_hit_breakpoint()
{
    short    i;

    ssl_pc--;

    for (i = 0; i < dbg_num_breakpoints; i++)
    {
        if (dbg_breakpoint[i].pc == -1)
            continue;   /* a watch breakpoint */

        if (ssl_pc == dbg_breakpoint[i].pc)
        {
            if (dbg_breakpoint[i].in_rule != NULL)
                printf ("Hit breakpoint in %s\n", dbg_breakpoint[i].in_rule);
            else if (dbg_breakpoint[i].line != -1)
                printf ("Hit breakpoint at %d\n", dbg_breakpoint[i].line);
            else
                printf ("Hit breakpoint at #%d\n", ssl_pc);
            break;
        }
    }

    if (i >= dbg_num_breakpoints)
        printf ("Hit unexpected breakpoint at %d\n", ssl_pc);

    dbg_remove_breakpoints();

    dbg_step_mode = DBG_STEP_INSTRUCTION;     /* Go into single step mode */
    dbg_step_count = 1;
}

/* Similar to above, but hit breakpoint on input line */
public    dbg_hit_input_breakpoint ()
{
    short    i;
    short    input_line, input_col;

    ssl_get_input_position (&input_line, &input_col);

    printf ("Hit breakpoint at input line %d\n", input_line);

    dbg_remove_breakpoints();

    dbg_step_mode = DBG_STEP_INSTRUCTION;     /* Go into single step mode */
    dbg_step_count = 1;
}

dbg_execution_complete()
{
    printf ("Execution complete.\n");
}


local int    dbg_find_line (pc)
int              pc;
{
    short        line;
    int          max_line = dbg_line_table[0];

    /* Skip to last source line at that pc address */
    /* -1 means a source line with no code generated */

    for (line = 1;
         (dbg_line_table[line] <= pc) && (line <= max_line);
         line++);

    if (dbg_line_table[line] > pc)
      for (line--;
           line > 0 && dbg_line_table[line] < 0;
           line--);

#if 0
    /* Original code ... probably still good */

    for (line = 1;
         dbg_line_table[line] < pc &&
         line <= dbg_line_table[0];
         line++);

    if (dbg_line_table[line] > pc)
      for (line--;
           line > 0 && dbg_line_table[line] < 0;
           line--);
#endif /* 0 */


    return ((int) line);
}


dbg_walkTable()
{
    dbgui_main_loop();    /* Enter main loop, with callbacks to dbg_command */

    /* Program has terminated, should remember it is no longer valid to execute */
#if 0
    /*  Taken from 'quit' command ... maybe I don't always want to do this...? */
    ssl_cleanup();
    exit (1);
#endif /* 0 */
}


/*  Bring up windows, init cmdline, callbacks, etc  */

dbgui_init (source_filename, input_filename)
char       *source_filename;
char       *input_filename;
{
    dbgui_create_debug_window(source_filename, input_filename);
}

/*  Rerun same program with a (possibly) new input file */
dbgui_restart (input_filename)
char          *input_filename;
{
    dbgui_restart_input_window(input_filename);
}


/*  Enter main event loop for debugger  */

dbgui_main_loop ()
{
    dbg_display_position();   /* initial display of position.  Subsequent update after each run. */
    ssltool_main_loop ();
}


/*  Command-line version of main loop, rather than graphical event loop */

cmdline_main_loop ()
{
    char      cmdbuf [100];
    int       done;

    done = 0;

    while (!done)
    {
        printf ("ssl.%d> ",ssl_pc);
        gets (cmdbuf);

        done = dbg_command (cmdbuf);
    }

}


dbgui_create_debug_window (source_filename, input_filename)
char                      *source_filename;
char                      *input_filename;
{
    int   my_argc = 1;         /* Hardcode these for now */
    static char *my_argv[] = { "ssltool", NULL };

    ssltool_init(my_argc, my_argv, source_filename, input_filename);

}

/* Restart input window with (possibly) new file */

dbgui_restart_input_window (input_filename)
char                       *input_filename;
{
    ssltool_restart_input_window (input_filename);
}

dbgui_at_line (line)
short          line;
{
#if 0
    /*  This is how I used to do it with TxEd on the Amiga */
    sprintf (cmdbuf, "rx 'address \"TxEd Plus1/c\" jump %d 0'", line);
    Execute (cmdbuf, 0,0);
#endif /* 0 */

    ssltool_at_line ((int) line);
}


dbgui_at_input_position (line, col)
int                      line;
int                      col;
{
    ssltool_at_input_position (line, col);
}


dbgui_execution_status (status_string)
char                   *status_string;
{
    ssltool_execution_status (status_string);
}

dbg_read_symbol_table (fp)
FILE *fp;
{
    int   num, class, type_num;
    char  name_buf[256];
    int   index, t;

    dbg_symbol_table_last = 0;

    while (fscanf (fp, "%d", &num) != EOF)
    {
        dbg_symbol_table_last++;
        if (dbg_symbol_table_last == DBG_SYMBOL_TABLE_SIZE)
        {
            printf ("Debug warning: symbol table full\n");
            dbg_symbol_table_last--;
            break;
        }

        dbg_symbol_table[dbg_symbol_table_last].num = num;

        fscanf (fp, "%s", name_buf);
        dbg_symbol_table[dbg_symbol_table_last].name = strdup(name_buf);

        fscanf (fp, "%s", name_buf);
        for (class = 1; dbg_symbol_class_names[class] != NULL; class++)
            if (strcmp(name_buf, dbg_symbol_class_names[class]) == 0)
                break;
        dbg_symbol_table[dbg_symbol_table_last].class = class;

        switch (class)
        {
            case DBG_SYMBOL_CLASS_global:
            case DBG_SYMBOL_CLASS_local:
            case DBG_SYMBOL_CLASS_inparam:
            case DBG_SYMBOL_CLASS_outparam:
            case DBG_SYMBOL_CLASS_inoutparam:
                fscanf (fp, "%d", &(dbg_symbol_table[dbg_symbol_table_last].type_num));
                break;
        }

        switch (class)
        {
            case DBG_SYMBOL_CLASS_rule:
            case DBG_SYMBOL_CLASS_global:
            case DBG_SYMBOL_CLASS_local:
            case DBG_SYMBOL_CLASS_inparam:
            case DBG_SYMBOL_CLASS_outparam:
            case DBG_SYMBOL_CLASS_inoutparam:
                fscanf (fp, "%d", &(dbg_symbol_table[dbg_symbol_table_last].value));
                break;
        }
    }

    /*  Now link up type references  */
    for (index = 1; index <= dbg_symbol_table_last; index++)
    {
        type_num = dbg_symbol_table[index].type_num;
        if (type_num != 0)
        {
            for (t = 1; t <= dbg_symbol_table_last; t++)
            {
                if (dbg_symbol_table[t].num == type_num)
                {
                    dbg_symbol_table[index].type = &(dbg_symbol_table[t]);
                    break;
                }
            }
        }
    }
}

local display_value (type_name, symbol_name, value)
char                *type_name;
char                *symbol_name;
long                 value;
{
    int  v;

    printf ("%s %s = ", type_name, symbol_name);

    for (v = 0; dbg_vars[v].display_method != NULL; v++)
    {
        if (dbg_vars[v].address != DBG_TYPE_DISPLAY)
            continue;

        if (strcmp(dbg_vars[v].name, type_name) == 0)
        {
            (*dbg_vars[v].display_method) (&value, dbg_vars[v].udata);
            return;
        }
    }

    /* Default display as int */
    printf ("\t%d\n", value);
}

/* If a var_name given, only variable(s) with matching name
 * will be displayed.
 */
local display_context_vars (pc, fp, show_globals, var_name)
short                       pc;
long                        fp;
int                         show_globals;
char                       *var_name;
{
    int    symbol;
    char  *rule;
    long   value;
    char  *type_name;
    char  *symbol_name;


    /*  Globals  */

    if (show_globals)
    {
        for (symbol = 1; symbol <= dbg_symbol_table_last; symbol++)
        {
            if (dbg_symbol_table[symbol].class == DBG_SYMBOL_CLASS_global)
            {
                symbol_name = dbg_symbol_table[symbol].name;
                if ((var_name == NULL) || (strcmp(var_name, symbol_name) == 0))
                {
                    type_name = (dbg_symbol_table[symbol].type)->name;
                    value = ssl_var_stack[dbg_symbol_table[symbol].value];

                    display_value (type_name, symbol_name, value);
                }
            }
        }

        if (var_name == NULL)
            printf ("--------------------\n");
    }


    /*  Parameters & Locals */

    /*  First, find rule symbol.  Param/local symbols follow it in table.  */

    rule = ssl_rule_name(pc);
    for (symbol = 1; symbol <= dbg_symbol_table_last; symbol++)
    {
        if ((dbg_symbol_table[symbol].class == DBG_SYMBOL_CLASS_rule) &&
            (strcmp(dbg_symbol_table[symbol].name, rule) == 0))
        {
            break;
        }
    }

    while ((++symbol <= dbg_symbol_table_last) &&
           ((dbg_symbol_table[symbol].class == DBG_SYMBOL_CLASS_local) ||
            (dbg_symbol_table[symbol].class == DBG_SYMBOL_CLASS_inparam) ||
            (dbg_symbol_table[symbol].class == DBG_SYMBOL_CLASS_outparam) ||
            (dbg_symbol_table[symbol].class == DBG_SYMBOL_CLASS_inoutparam)))
    {
        symbol_name = dbg_symbol_table[symbol].name;
        if ((var_name != NULL) && (strcmp(var_name, symbol_name) != 0))
            continue;

        type_name = (dbg_symbol_table[symbol].type)->name;

        switch (dbg_symbol_table[symbol].class)
        {
            case DBG_SYMBOL_CLASS_local:
                if (fp == ssl_var_sp)   /* Local space not allocated for this rule yet */
                    value = 0;          /* Display it as if it will be cleared on allocation */
                else
                    value = ssl_var_stack[fp + dbg_symbol_table[symbol].value];
                break;

            case DBG_SYMBOL_CLASS_inparam:
                value = ssl_var_stack[fp - dbg_symbol_table[symbol].value];
                break;

            case DBG_SYMBOL_CLASS_outparam:
            case DBG_SYMBOL_CLASS_inoutparam:
                value = ssl_var_stack[ssl_var_stack[fp - dbg_symbol_table[symbol].value]];
                break;
        }

        display_value (type_name, symbol_name, value);

    }
}



