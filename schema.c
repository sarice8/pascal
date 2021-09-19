/*
*****************************************************************************
*
*   Database Schema Compiler
*
*   by Steve Rice
*
*   May 1, 1991
*
*****************************************************************************
*
*   schema.c            Main program.
*
*   HISTORY
*----------------------------------------------------------------------------
*   05/01/91 | Steve  | Initial version.
*   05/04/91 |        | Generate attribute offset table.
*   05/05/91 |        | Record attribute class.
*            |        | Write SSL code and C code to different files.
*   05/30/91 |        | Collect tags (Pri...) and write to file.
*
*****************************************************************************
*/

/*  Define USE_C_TABLE to compile the ssl code directly into this file.
    Otherwise, the program will read it at run time.  */

#define USE_C_TABLE




#include <stdio.h>
#include <string.h>

#ifdef AMIGA
#include <dos.h>
#endif AMIGA



#define  SSL_INCLUDE_ERR_TABLE
#include "schema.h"

#include "schema_glob.h"



#define TABLE_FILE "schema.tbl"         /* File with SSL code to interpret */

#define DEBUG_FILE "schema.dbg"         /* SSL debug information */


/*
*****************************************************************************

   DESIGN NOTES:

   Table walker variables start with 'w_'.
   Semantic operations start with 'o'.
   Corresponding data structures start with 'd'.

   The code to walk is in w_codeTable (pointer=w_pc).
   The output buffer is w_outTable (pointer=w_here).

   The various stacks here don't use the [0] entry.  The stack variable
   points to the top entry.  Overflow is always checked; underflow is
   never checked.

*****************************************************************************
*/


#ifdef  USE_C_TABLE
#include "schema.tbl"
#else   USE_C_TABLE
short   w_codeTable [w_codeTableSize];    /* SSL code to interpret */
#endif  USE_C_TABLE


FILE    *t_src;                           /* source                         */
FILE    *t_lst;                           /* listing file                   */
FILE    *t_doc;                           /* copy of messages from SSL      */
FILE    *t_out;                           /* C code output                  */
FILE    *t_ssl_out;                       /* SSL code output                */



/* Variables for SSL table walker */
/* ------------------------------ */

#define  w_stackSize 200
short    w_stack [w_stackSize], w_sp;     /* call stack                     */

short    w_pc;                            /* program counter                */
short    w_tmp;                           /* used for error recovery        */
short    w_result;                        /* value from oSetResult          */
short    w_param;                         /* value for oSetParam            */
short    w_options;                       /* number of choice options       */
char     w_errBuffer [256];               /* err message construction area  */
short    w_errorCount;                    /* number of err signals emitted  */

#ifndef USE_C_TABLE
char     w_title_string [256];            /* Title of this SSL processor    */
#endif  USE_C_TABLE

#define  w_outTableSize 50
short    w_outTable [w_outTableSize];     /* emitted output table           */
short    w_here;                          /* position in output table       */

#define  w_assert(expr)  w_assert_fun((expr),__LINE__)

/* Variables for debugger mode */

int      w_debug;                         /* full screen debug mode         */
#define  w_lineTableSize 3000
short    w_lineTable [w_lineTableSize];   /* positions of lines in code     */
int      dbg_trace;                       /* trace source file?             */
int      dbg_count;                       /* instructions to execute        */
int      dbg_nocount;            /* true = no count limit (just breakpoint) */




/* Variables for semantic mechanisms */
/* --------------------------------- */

short    dTemp;                           /* multi-purpose                  */
short    dTemp2;                          /* multi-purpose                  */
short   *dPtr;                            /* multi-purpose                  */
short    dWords;                          /* multi-purpose                  */
short    dSym;                            /* index of symbol in ST          */


#define dNSsize 40                        /* NS: name stack                 */
short          dNS[dNSsize];
short          dNSptr;

#define dOSsize 40                        /* OS: object stack               */
short          dOS[dOSsize];
short          dOSptr;

#define dACsize 2                         /* AC: attribute class stack      */
short          dAC[dACsize];              /* Only expect to need one entry  */
short          dACptr;

#define dTSsize 10                        /* TS: tag stack                  */
short          dTS[dTSsize];
short          dTSptr;


/*  Maintained by w_define_obj(), w_define_attr() to hold definitions  */

struct attr_struct {
       short   attr;                      /* attr # */
       struct  attr_struct *next;
};

#define dOLsize 200
struct dOL_struct {                       /* OL: object list */
       short   id;                        /*     -- id# of object */
       struct  attr_struct *attr;         /*     -- list of attributes */
}              dOL[dOLsize];
short          dOLptr;

/*  Entry in attribute list is common for all uses of attribute  */

#define dALsize 200                       /* AL: attribute list             */
struct dAL_struct {
       short   id;                        /*     -- id# of attribute        */
       char    class;                     /* 4=Integer4, 100=NODE, 150=LIST */
       char    tags;                      /* bits for Pri, Alt, Opt         */
       short   type;                      /* object #, if NODE or LIST      */
}              dAL[dALsize];
short          dALptr;


short w_define_obj ();                    /* Define an object given an id   */
short w_define_attr ();                   /* Define an attribute given id   */
w_define_attr_of_obj ();                  /* ...given attr # and object #   */
w_derive_obj ();                          /* obj1 derives from obj2         */
w_dump_table ();


char           C_out_filename[100];
char           SSL_out_filename[100];

/*
*****************************************************************************
*
* main
*
* Description:
*   Get the command line arguments, read the SSL program into memory,
*   and execute the SSL table walker (interpreter).
*
*****************************************************************************
*/

main (argc, argv)
int   argc;
char *argv[];
{
    int     t_hitBreak();
    short   arg;
    char    tmp_buffer [256];
    int     i;
    int     entries;
    int     temp;                /* having trouble reading "short" */


    /* check command line arguments */

    t_listing = 0;
    w_debug   = 0;

    for (arg = 1; arg<argc && argv[arg][0]=='-'; arg++)
    {
        for (i = 1; argv[arg][i]; i++)
        {
            if (argv[arg][i]=='L' || argv[arg][i]=='l')
                t_listing = 1;
            else if (argv[arg][i]=='D' || argv[arg][i]=='d')
#ifdef AMIGA
                w_debug = 1;
#else  AMIGA
                printf ("The debug option is only available on the Amiga.\n")
#endif AMIGA
        }
    }

    if (arg >= argc)
    {
        printf("Usage:  schema [-l] [-d] <file>\n");
        printf("        -l : produce listing file\n");
        printf("        -d : invoke full screen debugger\n");
        exit(10);
    }


#ifndef USE_C_TABLE
    read_table (argv[0]);
#endif  USE_C_TABLE


    /* Get debug information (line numbers) */

    if (w_debug)
    {
        if ((t_src=fopen(DEBUG_FILE,"r"))==NULL)
        {
            printf ("Can't open debug file %s\n", TABLE_FILE);
            exit (10);
        }
        fscanf (t_src,"%d",&entries);           /* size of SSL code */
        if (entries > w_lineTableSize)
        {
            printf ("Debug file too big; must recompile %s.c\n",argv[0]);
            exit (10);
        }
        w_lineTable[0] = entries;
        for (w_pc=1; w_pc<entries; w_pc++)
        {
            fscanf (t_src,"%d",&temp);
            w_lineTable[w_pc] = temp;
        }
        fclose (t_src);
    }


    /* open other files */

    strcpy (tmp_buffer, argv[arg]);
    if ((strlen(argv[arg]) < 7) ||
        (strcmp(tmp_buffer+strlen(argv[arg])-7,".schema") != 0))
        strcpy(tmp_buffer+strlen(argv[arg]),".schema");

    if ((t_src=fopen(tmp_buffer,"r"))==NULL)
    {
        printf ("Can't open source file %s\n",tmp_buffer);
        exit (10);
    }

    sprintf (C_out_filename, "%s_schema.h", argv[arg]);
    if ((t_out=fopen(C_out_filename,"w"))==NULL)
    {
        printf ("Can't open output file %s\n", C_out_filename);
        exit (10);
    }

    sprintf (SSL_out_filename, "%s_schema.ssl", argv[arg]);
    if ((t_ssl_out=fopen(SSL_out_filename,"w"))==NULL)
    {
        printf ("Can't open output file %s\n", SSL_out_filename);
        exit (10);
    }

    if ((t_doc=fopen("a.doc","w"))==NULL)
    {
        printf ("Can't open output file a.doc\n");
        exit (10);
    }

    if (t_listing)
    {
        if ((t_lst=fopen("a.lst","w"))==NULL)
        {
            printf ("Can't open listing file a.lst\n");
            exit (10);
        }
    }

    printf ("%s\n", w_title_string);        /* print title */


#ifdef AMIGA
    onbreak(&t_hitBreak);
#endif AMIGA


    /* initialize scanner */

    t_initScanner();


    /* initialize table walker */

    w_pc = 0;
    w_sp = 0;
    w_here = 0;
    w_errorCount = 0;

    /* initialize debugger */

    if (w_debug)
        w_initDebugger();

    /* initialize semantic operations */

    w_initSemanticOperations();


    /* execute SSL program */

    w_walkTable();


    /* program has completed */

    if (w_errorCount)
        printf ("\n%d error(s)\n", w_errorCount);

    t_cleanup();

    if (w_errorCount)
        exit(5);
}


/* get SSL code -- if not using USE_C_TABLE option */

read_table (progname)
char       *progname;
{
    int     entries;
    int     temp;                /* having trouble reading "short" */

    if ((t_src=fopen(TABLE_FILE,"r"))==NULL)
    {
        printf ("Can't open table file %s\n", TABLE_FILE);
        exit (10);
    }

    fgets (w_title_string, 256, t_src);         /* title string */

    fscanf (t_src,"%d",&entries);           /* size of SSL code */
    if (entries > w_codeTableSize)
    {
        printf ("Table file too big; must recompile %s.c\n", progname);
        exit (10);
    }

    for (w_pc=0; w_pc<entries; w_pc++)
    {
        fscanf (t_src,"%d",&temp);
        w_codeTable[w_pc] = temp;
    }
    fclose (t_src);
}

/*********** S e m a n t i c   O p e r a t i o n s ***********/

w_errorSignal(err)
short err;
{
  short i;
  i = err;
  if (w_errTable[i].val != i)
    for (i=0; i<w_errTableSize && w_errTable[i].val != err; i++);
  sprintf(w_errBuffer,"Error: %s",w_errTable[i].msg);
  t_error(w_errBuffer);
}


w_initSemanticOperations ()
{
    dNSptr = 0;
    dOSptr = 0;
    dOLptr = 0;   dOL[dOLptr].attr = NULL;
    dALptr = 0;
    dACptr = 0;
    dTSptr = 0;
}


w_walkTable()
{
   char  tmp_buffer [256];

   while (1) {

#ifdef AMIGA

     if (w_debug)
         w_debugCommand();

#endif AMIGA


     switch (w_codeTable[w_pc++]) {

       /* SSL Instructions */

       case oJumpForward :
              w_pc += w_codeTable[w_pc];
              continue;
       case oJumpBack :
              w_pc -= w_codeTable[w_pc];
              continue;
       case oInput :
              t_getToken();
              if (t_token.code != w_codeTable[w_pc])
              {
                  sprintf (tmp_buffer, "Syntax error: expecting \"%s\"",
                           t_getCodeName(w_codeTable[w_pc]));
                  t_error (tmp_buffer);
                  w_pc--;
                  w_error_recovery ();
                  continue;
              }
              w_pc++;
              t_token.accepted = 1;
              if (t_listing && !t_lineListed) {
                fprintf(t_lst,"%4d: %s",w_here,t_lineBuffer);
                t_lineListed = 1;
              }
              if (t_token.code == pIDENTIFIER)
                t_lastId = t_token.val;
              continue;
       case oInputAny :
              t_getToken();
              t_token.accepted = 1;
              if (t_listing && !t_lineListed) {
                fprintf(t_lst,"%4d: %s",w_here,t_lineBuffer);
                t_lineListed = 1;
              }
              if (t_token.code == pIDENTIFIER)
                t_lastId = t_token.val;
              continue;
       case oEmit :
              dTemp = w_codeTable[w_pc++];
              w_outTable[w_here++] = dTemp;
              continue;
       case oError :
              w_errorSignal(w_codeTable[w_pc]);
              w_pc--;
              w_error_recovery ();
              continue;
       case oInputChoice :
              t_getToken();
              w_tmp = w_pc - 1;              /* start of choice instruction */
              w_pc += w_codeTable[w_pc];
              w_options = w_codeTable[w_pc++];
              while (w_options--) {
                if (w_codeTable[w_pc++] == t_token.code) {
                  w_pc -= w_codeTable[w_pc];
                  t_token.accepted = 1;
                  if (t_listing && !t_lineListed) {
                    fprintf(t_lst,"%4d: %s",w_here,t_lineBuffer);
                    t_lineListed = 1;
                  }
                  if (t_token.code == pIDENTIFIER)
                    t_lastId = t_token.val;
                  break;
                }
                w_pc++;
              }
              continue;
       case oCall :
              if (++w_sp==w_stackSize) t_fatal ("SSL: call stack overflow");
              w_stack[w_sp] = w_pc+1;
              w_pc = w_codeTable[w_pc];
              continue;
       case oReturn :
              if (w_sp) {
                w_pc = w_stack[w_sp--];
                continue;
              } else
                return;            /* done walking table */
       case oSetResult :
              w_result = w_codeTable[w_pc++];
              continue;
       case oChoice :
              w_tmp = w_pc - 1;              /* start of choice instruction */
              w_pc += w_codeTable[w_pc];
              w_options = w_codeTable[w_pc++];
              while (w_options--) {
                if (w_codeTable[w_pc++] == w_result) {
                  w_pc -= w_codeTable[w_pc];
                  break;
                }
                w_pc++;
              }
              continue;
       case oEndChoice :           /* choice or inputChoice didn't match */
              t_error ("Syntax error");
              /* t_token.accepted = 1; */ /* actually, only if input choice err*/
              w_pc = w_tmp;        /* back to start of choice instruction */
              w_error_recovery ();
              continue;
       case oSetParameter :
              w_param = w_codeTable[w_pc++];
              continue;


       /* Semantic Mechanisms */


       /* mechanism name_stack */

       case oNSPush :
              if (++dNSptr == dNSsize) t_fatal ("NS overflow");
              dNS[dNSptr] = t_token.val;
              continue;


       /* mechanism object */

       case oObjectDefine :
              if (++dOSptr == dOSsize) t_fatal ("OS overflow");
              dOS[dOSptr] = w_define_obj (t_token.val);
              continue;
       case oObjectDefineNS :
              if (++dOSptr == dOSsize) t_fatal ("OS overflow");
              dOS[dOSptr] = w_define_obj (dNS[dNSptr--]);
              continue;
       case oObjectDerive :
              w_derive_obj (dOS[dOSptr], dOS[dOSptr-1]);
              dOSptr -= 2;
              continue;
       case oObjectPop :
              dOSptr--;
              continue;


       /* mechanism attr */

       case oAttrDefineNS :
              dTemp2 = 0;          /* collect tag bits */
              while (dTSptr > 0)
                  dTemp2 = dTemp2 | dTS[dTSptr--];
              dTemp = w_define_attr (dNS[dNSptr--], dAC[dACptr--], dTemp2);
              w_define_attr_of_obj (dTemp, dOS[dOSptr]);
              continue;

       case oAttrClass :
              if (++dACptr == dACsize) t_fatal ("AC overflow");
              dAC[dACptr] = w_param;
              continue;


       /* mechanism tag */

       case oTagPush :
              if (++dTSptr == dTSsize) t_fatal ("TS overflow");
              dTS[dTSptr] = w_param;
              continue;


       /* mechanism doc */

       case oDocDumpTable :
              w_dump_table ();
              continue;


       default:
              w_pc--;
              sprintf(w_errBuffer,"SSL: Bad instruction #%d at %d",
                      w_codeTable[w_pc],w_pc);
              t_fatal(w_errBuffer);
     } /* switch */

   }  /* while */
}


/* --------- FOR SEMANTIC MECHANISMS --------- */
short w_define_obj (id_val)
short               id_val;
{
    short o;

    for (o = 1; o <= dOLptr; o++)
        if (dOL[o].id == id_val)
            return (o);

    if (++dOLptr == dOLsize) t_fatal ("OL overflow");
    dOL[dOLptr].id = id_val;
    dOL[dOLptr].attr = NULL;
    return (dOLptr);
}

short w_define_attr (id_val, attr_class, tags)
short                id_val;
short                attr_class;
short                tags;
{
    short a;

    for (a = 1; a <= dALptr; a++)
        if (dAL[a].id == id_val)
        {
            if (dAL[a].class != attr_class)
                t_fatal ("Inconsistent class of attribute");
            if (dAL[a].tags != tags)
                t_fatal ("Attribute tags inconsistent with previous definition");
            return (a);
        }

    if (++dALptr == dALsize) t_fatal ("AL overflow");
    dAL[dALptr].id = id_val;
    dAL[dALptr].class = attr_class;
    dAL[dALptr].tags = tags;
    return (dALptr);
}

w_define_attr_of_obj (attr_num, obj_num)
short          attr_num;
short          obj_num;
{
    struct attr_struct *as, *p;

    as = (struct attr_struct *) malloc (sizeof (struct attr_struct));
    as->attr = attr_num;
    as->next = NULL;

    if (dOL[obj_num].attr == NULL)
        dOL[obj_num].attr = as;
    else
    {
        for (p = dOL[obj_num].attr; p->next != NULL; p = p->next);
        p->next = as;
    }

}


/*  when deriving object, copy all the old object's attributes  */
/*  STILL TO DO:  record in a table that this object is derived */

w_derive_obj (new_obj, old_obj)
short         new_obj, old_obj;
{
    struct attr_struct *attr;

    for (attr = dOL[old_obj].attr; attr != NULL; attr = attr->next)
    {
        w_define_attr_of_obj (attr->attr, new_obj);
    }

/*
    printf ("%s derives from %s\n",
            t_idTable[dOL[new_obj].id].name,
            t_idTable[dOL[old_obj].id].name);
*/
}

w_dump_table ()
{
    struct attr_struct  *attr;
    short                o;
    short                a;
    short               *attr_offsets;
    short               *obj_sizes;
    short                next_offset;

    printf ("------- Writing SSL code to %s --------\n", SSL_out_filename);

    fprintf (t_ssl_out, "% Generated automatically by schema\n\n");

    fprintf (t_ssl_out, "type node_type:\n");
    fprintf (t_ssl_out, "\tnINVALID = 0\n");
    for (o = 1; o <= dOLptr; o++)
        fprintf (t_ssl_out, "\t%s\n", t_idTable[dOL[o].id].name);
    fprintf (t_ssl_out, "\t;\n\n");

    fprintf (t_ssl_out, "type node_attribute:\n");
    fprintf (t_ssl_out, "\tqINVALID = 0\n");
    for (a = 1; a <= dALptr; a++)
        fprintf (t_ssl_out, "\t%s\n", t_idTable[dAL[a].id].name);
    fprintf (t_ssl_out, "\t;\n\n");

    printf ("------- Writing C code to %s --------\n", C_out_filename);

    fprintf (t_out, "/* Generated automatically by schema */\n\n");

    attr_offsets = (short *) malloc (sizeof(short) * (dALptr+1));
    obj_sizes = (short *) malloc (sizeof(short) * (dOLptr+1));

    fprintf (t_out, "short dAttributeOffset [%d][%d] = {\n", dOLptr+1, dALptr+1);
    for (o = 0; o <= dOLptr; o++)
    {
        next_offset = 0;
        for (a = 0; a <= dALptr; a++)
            attr_offsets[a] = -1;

        for (attr = dOL[o].attr; attr != NULL; attr = attr->next)
        {
            attr_offsets[attr->attr] = next_offset;
            next_offset += 4;
        }

        obj_sizes[o] = next_offset;

        fprintf (t_out, "\t");
        for (a = 0; a <= dALptr; a++)
            fprintf (t_out, "%3d, ", attr_offsets[a]);
        fprintf (t_out, "\n");
    }
    fprintf (t_out, "};\n\n");

    fprintf (t_out, "short dObjectSize [%d] = {\n\t", dOLptr+1);
    for (o = 0; o <= dOLptr; o++)
    {
        fprintf (t_out, "%d, ", obj_sizes[o]);
        if ((o & 15) == 15)
            fprintf (t_out, "\n\t");
    }
    fprintf (t_out, "\n};\n\n");

    fprintf (t_out, "int   dObjects = %d;\n", dOLptr+1);
    fprintf (t_out, "char *dObjectName [%d] = {\n", dOLptr+1);
    for (o = 0; o <= dOLptr; o++)
    {
        fprintf (t_out, "\t\"%s\",\n", t_idTable[dOL[o].id].name);
    }
    fprintf (t_out, "};\n\n");

    fprintf (t_out, "int   dAttributes = %d;\n", dALptr+1);
    fprintf (t_out, "char *dAttributeName [%d] = {\n", dALptr+1);
    for (a = 0; a <= dALptr; a++)
    {
        fprintf (t_out, "\t\"%s\",\n", t_idTable[dAL[a].id].name);
    }
    fprintf (t_out, "};\n\n");

    fprintf (t_out, "short dAttributeClass [%d] = {\n\t", dALptr+1);
    for (a = 0; a <= dALptr; a++)
    {
        fprintf (t_out, "%d, ", dAL[a].class);
        if ((a & 15) == 15)
            fprintf (t_out, "\n\t");
    }
    fprintf (t_out, "\n};\n\n");

    fprintf (t_out, "short dAttributeTags [%d] = {\n\t", dALptr+1);
    for (a = 0; a <= dALptr; a++)
    {
        fprintf (t_out, "%d, ", dAL[a].tags);
        if ((a & 15) == 15)
            fprintf (t_out, "\n\t");
    }
    fprintf (t_out, "\n};\n\n");

}

/* ------------------------------------------- */


/*  On entry, w_pc will sit at start of oInput, oError, oInputChoice or
    oChoice instruction, which caused the mismatch.
    t_token will contain the mismatched token (not yet accepted).
    On exit, w_pc will sit at an instruction just after accepting a ';', and
    t_token will contain a just-accepted ';' (or pEOF).
    No semantic mechanisms will be called; no output will be generated.
    The t_lastId index will not be maintained.
    Note, currently Semantic/Rule choices will follow the same algorithm as
    input choices: take default, or first choice if no default.    */

w_error_recovery ()
{
    int done_loop = 0;
    int steps;

    t_abort();  /* Error recovery not supported in SCHEMA. */
                /* There's no good 'required' token. */
                /* (I can't recover to a choice token) */


steps = 0;  /* for debugging */

    /* First, walk table until we accept a ';'.  Do not call semantic mechanisms.
       Do not actually accept input tokens. */


    /* If original failure in a choice table, start recovery by taking the first
       choice.  (By definition, a failure in a choice will only happen if there
       is no default) */
    /* NOTE: here I assume that the first choice given by the user will be the
       code immediately following the oChoice instruction.  Alternatively, could
       follow the table, where the user's first choice is currently the LAST entry. */
/* NOTE: should check if the first choice in the input choice table is for ; */


printf ("recovery: error with %s at location %d\n", t_getCodeName(t_token.code), w_pc);

    if (w_codeTable[w_pc] == oInputChoice || w_codeTable[w_pc] == oChoice)
    {
        w_pc += 2;
    }

   while (!done_loop)
   {
     switch (w_codeTable[w_pc++]) {

       case oJumpForward :
              w_pc += w_codeTable[w_pc];
              continue;
       case oJumpBack :
              w_pc -= w_codeTable[w_pc];
              continue;
       case oInput :
              if (w_codeTable[w_pc++] == pDERIVES)
                  done_loop = 1;
              continue;
       case oInputAny :
              continue;
       case oEmit :
              continue;
       case oError :
              continue;
       case oInputChoice :
              w_tmp = w_pc + w_codeTable[w_pc];                /* position of #options */
              w_tmp = (w_codeTable[w_tmp] * 2) + w_tmp + 1;    /* position of default code */
              if (w_codeTable[w_tmp] == oEndChoice)            /* no default */
              {
                  w_pc = w_pc + 1;                             /* go to first choice */
                  if (w_codeTable[w_tmp-1] == pDERIVES)        /* ASSUMING first choice is */
                      done_loop = 1;                           /*     last in table        */
              }
              else
                  w_pc = w_tmp;                                /* go to default code */
              continue;
       case oCall :
              if (++w_sp==w_stackSize) t_fatal ("SSL: call stack overflow");
              w_stack[w_sp] = w_pc+1;
              w_pc = w_codeTable[w_pc];
              continue;
       case oReturn :
              if (w_sp) {
                w_pc = w_stack[w_sp--];
                continue;
              } else
                t_fatal ("SSL: parse tree completed during recovery");
       case oSetResult :
              w_result = w_codeTable[w_pc++];
              continue;
       case oChoice :
              /* NOTE: maybe we should obey actual choice value here, but
                       can't now because not calling semantic mechanisms. */
              w_tmp = w_pc + w_codeTable[w_pc];                /* position of #options */
              w_tmp = (w_codeTable[w_tmp] * 2) + w_tmp + 1;    /* position of default code */
              if (w_codeTable[w_tmp] == oEndChoice)            /* no default */
                  w_pc = w_pc + 1;                             /* go to first choice */
              else
                  w_pc = w_tmp;                                /* go to default code */
              continue;
       case oEndChoice :           /* choice or inputChoice didn't match */
              continue;            /* shouldn't occur during recovery */
       case oSetParameter :
              w_param = w_codeTable[w_pc++];
              continue;

       default :                   /* all semantic mechanisms ignored */
              continue;
       }

       if (steps++ > 100)  /*DEBUGGING -- LET CTRL-C HAVE AN EFFECT*/
       { printf ("-"); steps = 0; }
    }

printf ("recovery: w_pc advanced to %d\n", w_pc);

    /* Now advance token stream to accept a ';' */

    /* first check if mismatch token is ';' */

    if (t_token.code == pDERIVES || t_token.code == pEOF)
    {
        t_token.accepted = 1;
        if (t_listing && !t_lineListed)
        {
            fprintf(t_lst, "%4d: %s", w_here, t_lineBuffer);
            t_lineListed = 1;
        }
        return;
    }

    do
    {
        t_getToken();
/* printf ("recovery: accepting %s\n", t_getCodeName(t_token.code)); */
        t_token.accepted = 1;
        if (t_listing && !t_lineListed)
        {
            fprintf(t_lst, "%4d: %s", w_here, t_lineBuffer);
            t_lineListed = 1;
        }
printf ("+");  /* FOR DEBUGGING */
    }
    while (t_token.code != pDERIVES && t_token.code != pEOF);

}


w_assert_fun (expr, line_num)
int       expr;
int       line_num;
{
    char   buffer[100];

    if (!expr)
    {
        sprintf (buffer, "Assertion error in walker, line %d", line_num);
        t_fatal (buffer);
    }
}


w_traceback()
{
  short i;
  printf("SSL Traceback:\n");
  fprintf(t_doc,"SSL Traceback:\n");
  printf("  %d",w_pc);
  fprintf(t_doc,"  %d",w_pc);
  for (i=w_sp;i>0;i--) {
    printf("  called from\n  %d",w_stack[i]-2);
    fprintf(t_doc,"  called from\n  %d",w_stack[i]-2);
  }
  printf("\n");
  fprintf(t_doc,"\n");
}


t_hitBreak()
{
  printf("Breaking...\n");
  t_cleanup();
  return(1);
}


t_cleanup()
{
  t_dumpTables();
  fclose(t_src);
  fclose(t_out);
  fclose(t_ssl_out);
  fclose(t_doc);
  if (t_listing) fclose(t_lst);
}


t_fatal(msg)
char *msg;
{
  char tmpbuf[256];

  sprintf (tmpbuf, "[FATAL] %s", msg);
  t_error (tmpbuf);
  t_abort();
}


t_abort()
{
    w_traceback();
    t_cleanup();
    exit(5);
}


t_printToken()
{
  printf(" (Token=");
  fprintf(t_doc," (Token=");
  if (t_token.code==pIDENTIFIER) {
    printf("%s)\n",t_token.name);
    fprintf(t_doc,"%s)\n",t_token.name);
  } else if (t_token.code==pINVALID) {
    printf("'%s')\n",t_token.name);
    fprintf(t_doc,"'%s')\n",t_token.name);
  } else {
    printf("<%d>)\n",t_token.code);
    fprintf(t_doc,"<%d>)\n",t_token.code);
  }
}



t_error(msg)
char *msg;
{
  char *p,spaceBuf[256];
  short col;

  w_errorCount++;
  printf("%s",t_lineBuffer);
  fprintf(t_doc,"%s",t_lineBuffer);
  for (p = spaceBuf, col = 0; col < t_token.colNumber; col++)
  {
    if (t_lineBuffer[col] == '\t')
        *p++ = '\t';
    else
        *p++ = ' ';
  }
  *p = '\0';
  printf("%s^\n",spaceBuf);
  fprintf(t_doc,"%s^\n",spaceBuf);
  printf("%s on line %d\n",msg,t_token.lineNumber);
  fprintf(t_doc,"%s on line %d\n",msg,t_token.lineNumber);
  /* t_printToken(); */
}


t_dumpTables()
{
#if 0
  short i,col;
  printf("\nWriting Files...\n");

  /* code segment */
  fprintf(t_out,"%d\n",w_here);
  col = 0;
  for (i=0; i<w_here; i++) {
    fprintf(t_out,"%6d",w_outTable[i]);
    if ((++col % 10) == 0)
      fprintf(t_out,"\n");
  }
#endif 0
}


/* ---------------------------- debugger -------------------------- */


#define MAXARGS 10


w_debugCommand ()
{
  short line;
  char cmdbuf[100];
  int argc;
  char *argv[MAXARGS];

  if (dbg_nocount || --dbg_count > 0)     /* time to halt execution? */
    return;

  /* check for breakpoints */


  dbg_nocount = 0;    /* halt non-stop execution */

  if (dbg_trace)
  {
    /* Move source window to current position */

    for (line = 1;
         w_lineTable[line] < w_pc &&
         line <= w_lineTable[0];
         line++);
    if (w_lineTable[line] > w_pc)
      for (line--;
           line > 0 && w_lineTable[line] < 0;
           line--);

#ifdef AMIGA
    sprintf (cmdbuf, "rx 'address \"TxEd Plus1/c\" jump %d 0'", line);
    Execute (cmdbuf, 0,0);
#else  AMIGA
    printf ("[line %d]\n", line);
#endif AMIGA

  }

  while (1)
  {
    printf ("ssl.%d>",w_pc);
    gets (cmdbuf);
    split_args (cmdbuf, &argc, argv);

    if (argc == 0)
      continue;
    else if (strcmp(argv[0],"s") == 0)
    {
      dbg_count = 1;                 /* single-step */
      return;
    }
    else if (strcmp(argv[0],"run") == 0)
    {
      if (argc < 2)
        dbg_count = 100;   /* some default count */
      else
        dbg_count = atoi(argv[1]);
      return;
    }
    else if (strcmp(argv[0],"cont") == 0)
    {
      dbg_nocount = 1;
      return;
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
      t_cleanup();
      exit (1);
    }
    else if (strcmp(argv[0],"p") == 0)
    {
      dbg_print();
    }
    else
    {
      printf ("-------------- commands: ---------------\n");
      printf ("s              (step)\n");
      printf ("p              (print variables)\n");
      printf ("run n          (execute n instructions)\n");
      printf ("cont           (execute until breakpoint)\n");
      printf ("trace on|off   (trace SSL code in TxEd1 window)\n");
      printf ("quit           (quit program)\n");
    }
  }
}


split_args (cmdbuf, argc, argv)
char *cmdbuf;
int *argc;
char **argv;
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

w_initDebugger ()
{
  dbg_trace = 1;    /* trace source */
  dbg_count = 0;
  dbg_nocount = 0;
}


/*  debugger "p" command -- dump interesting variables  */

dbg_print ()
{
    int i;

    printf ("OS:\t"); for (i = dOSptr; i>0; i--) printf (" %d", dOS[i]);
    printf ("\n");

    printf ("NS:\t"); for (i = dNSptr; i>0; i--) printf (" %d", dNS[i]);
    printf ("\n");

    printf ("Token:\t%s", t_getCodeName(t_token.code));
    if (t_token.code == pIDENTIFIER)
        printf (":  %s", t_token.name);
    printf ("  [%saccepted]\n", t_token.accepted ? "" : "not ");
}




