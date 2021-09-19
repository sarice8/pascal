/**/ 
static char sccsid[] = "@(#)schema.c	1.4 8/23/93 20:10:04 /files/home/sim/sarice/compilers/schema/SCCS/s.schema.c";
/**/

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


/*  Trap fatal errors in debug version */
#include <setjmp.h>
jmp_buf  t_fatal_jmp_buf;


/* Schema database access functions */
#include "node.h"


#define  SSL_INCLUDE_ERR_TABLE
#include "schema.h"

#include "schema_glob.h"

#include "debug.h"


#define TABLE_FILE "schema.tbl"         /* File with SSL code to interpret */

#define DEBUG_FILE "schema.dbg"         /* SSL debug information */
#define DEBUG_SOURCE "schema.lst"


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

short    w_codeTable_size;

char     input_filename[256];             /* schema source */

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


int      w_debug;                         /* full screen debug mode         */
int      w_walking;                       /* currently in walker            */



/* Variables for semantic mechanisms */
/* --------------------------------- */

short    dTemp;                           /* multi-purpose                  */
short    dTemp2;                          /* multi-purpose                  */
short   *dPtr;                            /* multi-purpose                  */
short    dWords;                          /* multi-purpose                  */
short    dSym;                            /* index of symbol in ST          */
long     dLong;                           /* multi-purpose                  */
NODE_PTR dNode;

NODE_PTR SchemaRoot;                      /* Root of schema database */
NODE_PTR CurrentClass;                    /* class found with oFindClass */
NODE_PTR GetsAttrsClass;                  /* class to get attributes */
NODE_PTR CurrentAttr;                     /* current attribute */
NODE_PTR CurrentAttrSym;                  /* current attribute symbol */
long     NumAttrSyms;                     /* count of attribute symbols */
long     NumClasses;                      /* count of created/derived classes */

dbg_variables debug_variables[] = {
    /* "Name", address,    udata,    function */
    "",        NULL,       0,        NULL,
};


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
                w_debug = 1;
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


    /* open other files */

    strcpy (input_filename, argv[arg]);
    if ((strlen(argv[arg]) < 7) ||
        (strcmp(input_filename+strlen(argv[arg])-7,".schema") != 0))
        strcpy(input_filename+strlen(argv[arg]),".schema");

    sprintf (C_out_filename, "%s_schema.c", argv[arg]);
    sprintf (SSL_out_filename, "%s_schema.ssl", argv[arg]);

    w_restart ();

#ifdef AMIGA
    onbreak(&t_hitBreak);
#endif AMIGA


    /* execute SSL program */

    if (w_debug)
        dbg_walkTable();
    else
        w_walkTable();


    /* program has completed */

    if (w_errorCount)
        printf ("\n%d error(s)\n", w_errorCount);

    t_cleanup();

    if (w_errorCount)
        exit(5);

    exit(0);
}


w_restart ()
{

    if ((t_src=fopen(input_filename,"r"))==NULL)
    {
        printf ("Can't open source file %s\n", input_filename);
        exit (10);
    }

    if ((t_out=fopen(C_out_filename,"w"))==NULL)
    {
        printf ("Can't open output file %s\n", C_out_filename);
        exit (10);
    }

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

    /* initialize scanner */

    t_initScanner();


    /* initialize table walker */

    w_pc = 0;
    w_sp = 0;
    w_here = 0;
    w_errorCount = 0;
    w_walking = 0;


    /* initialize semantic operations */

    w_initSemanticOperations();


    /* initialize debugger */

    if (w_debug)
    {
        dbg_init (DEBUG_FILE, DEBUG_SOURCE, input_filename, oBreak,
                  debug_variables);
    }

    printf ("%s\n", w_title_string);        /* print title */
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
    w_codeTable_size = entries;
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
    nodeInit ();   /* Init node package */

    SchemaRoot = NULL;
    CurrentClass = NULL;
    GetsAttrsClass = NULL;
    CurrentAttr = NULL;
    CurrentAttrSym = NULL;
    NumAttrSyms = 0;
    NumClasses = 0;
}


w_walkTable()
{
   char  tmp_buffer [256];
   short  t_addId();   /* For one of the mechanisms. From scanner.  Should make public. */
   NODE_PTR  w_find_class();  /* For a mechanism. Code below. */

   w_walking = 1;

   if (w_debug)
   {
       if (setjmp (t_fatal_jmp_buf))   /* Return here from calls to t_fatal() during run */
       { 
           w_walking = 0;
           t_error ("Stopped at fatal error");
           return (0);   /* I guess we say that the program is complete: hit fatal error */
       } 
   }


   while (1) {

     if (w_debug)
     {
        if (dbg_check_step_count ())
        {
            w_walking = 0;
            return (2);
        }
     }


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
              } else {
                w_pc--;
                w_walking = 0;
                return(0);            /* done walking table */
              }
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
        case oBreak :
              w_walking = 0;
              return(1);   /* hit breakpoint */


       /* Semantic Mechanisms */


       /* mechanism Class */

       case oCreateEmptySchema :
              SchemaRoot = nodeNew ((short) nSchema);

              dTemp = t_addId ("nINVALID", pIDENTIFIER);   /* Add to ident table */
              dNode = nodeNew ((short) nClass);
              nodeSetValue (dNode, qIdent, (long)dTemp);
              nodeAppend (SchemaRoot, qClassTree, dNode);
              NumClasses++;

              dTemp = t_addId ("Object", pIDENTIFIER);   /* Add to ident table */
              dNode = nodeNew ((short) nClass);
              nodeSetValue (dNode, qIdent, (long)dTemp);
              nodeAppend (SchemaRoot, qClassTree, dNode);
              NumClasses++;

              dTemp = t_addId ("qINVALID", pIDENTIFIER);   /* Add to ident table */
              dNode = nodeNew ((short) nAttrSym);
              nodeSetValue (dNode, qIdent, (long)dTemp);
              nodeSetValue (dNode, qCode, (long) NumAttrSyms);
              nodeAppend (SchemaRoot, qAttrSyms, dNode);
              NumAttrSyms++;

              continue;
       case oFindClass :
              CurrentClass = w_find_class (nodeGet(SchemaRoot, qClassTree), (long) t_lastId);
              if (CurrentClass == NULL)
                  t_fatal("Undefined class");
              continue;
       case oDeriveClass :
              /* Verify class not already defined */
              if (w_find_class (nodeGet(SchemaRoot, qClassTree), (long) t_lastId) != NULL)
                  t_fatal("Multiply defined class");
              dNode = nodeNew ((short) nClass);
              nodeSetValue (dNode, qIdent, (long) t_lastId);
              nodeAppend (CurrentClass, qDerived, dNode);
              NumClasses++;
              continue;
       case oThisClassWillGetAttrs :
              GetsAttrsClass = CurrentClass;
              continue;
       case oNoClassWillGetAttrs :
              GetsAttrsClass = NULL;
              continue;


       /* mechanism Attr */

       case oCreateAttrSym :
              for (dNode = nodeGet(SchemaRoot, qAttrSyms); dNode != NULL; dNode = nodeNext(dNode))
                  if (nodeGetValue(dNode, qIdent) == t_lastId)
                      break;
              if (dNode == NULL)
              {
                  dNode = nodeNew ((short) nAttrSym);
                  nodeSetValue (dNode, qIdent, (long) t_lastId);
                  nodeSetValue (dNode, qCode, (long) NumAttrSyms);
                  nodeAppend (SchemaRoot, qAttrSyms, dNode);
                  NumAttrSyms++;
              }
              CurrentAttrSym = dNode;
              continue;
       case oCreateAttr :
              CurrentAttr = nodeNew ((short) nAttr);
              nodeLink (CurrentAttr, qAttrSym, CurrentAttrSym);
              nodeAppend (GetsAttrsClass, qAttrs, CurrentAttr);
              continue;
       case oAttrType :
              nodeSetValue (CurrentAttr, qType, (long) w_param);
              continue;
       case oAttrTag :
              dLong = nodeGetValue (CurrentAttr, qTags);
              nodeSetValue (CurrentAttr, qTags, (long) (dLong | w_param));
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

NODE_PTR  w_find_class (root, ident)
NODE_PTR                root;
long                    ident;
{
    NODE_PTR   derived;
    NODE_PTR   find_in_derived;

    while (root != NULL)
    {
        if (nodeGetValue (root, (short) qIdent) == ident)
            return (root);

        derived = nodeGet (root, (short) qDerived);
        find_in_derived = w_find_class (derived, ident);
        if (find_in_derived != NULL)
            return (find_in_derived);

        root = nodeNext(root);
    }
    return (NULL);

}


w_dump_table ()
{
    NODE_PTR             class;
    short               *obj_sizes;
    char                *obj_isa;
    short               *attr_offsets;
    short               *attr_types;
    short               *attr_tags;
    short                next_offset;
    short                a, c;
    int                  current_class_code;

    printf ("------- Writing SSL code to %s --------\n", SSL_out_filename);

    fprintf (t_ssl_out, "\n%% Generated automatically by schema\n\n");

    fprintf (t_ssl_out, "type node_type:\n");

    w_dump_ssl_class (nodeGet(SchemaRoot, qClassTree));

    fprintf (t_ssl_out, "\t;\n\n");

    fprintf (t_ssl_out, "type node_attribute:\n");

    w_dump_ssl_attr ();

    fprintf (t_ssl_out, "\t;\n\n");

    printf ("------- Writing C code to %s --------\n", C_out_filename);

    fprintf (t_out, "\n/* Generated automatically by schema */\n\n");

    fprintf (t_out, "/* Private Data */\n\n");



    /* Initialize memory layout assignments before any classes created */

    attr_offsets = (short *) malloc (sizeof(short) * NumAttrSyms);
    obj_sizes    = (short *) malloc (sizeof(short) * NumClasses);
    obj_isa      = (char  *) malloc (sizeof(char)  * NumClasses);

    for (a = 0; a < NumAttrSyms; a++)
        attr_offsets[a] = -1;

    for (c = 0; c < NumClasses; c++)
        obj_sizes[c] = 0;

    for (c = 0; c < NumClasses; c++)
        obj_isa[c] = 0;


    /* Print entries for class tree */

    next_offset = 0;
    current_class_code = -1;

    fprintf (t_out, "static short dAttributeOffset [%d][%d] = {\n", NumClasses, NumAttrSyms);

    w_dump_c_attr_offsets (nodeGet(SchemaRoot, qClassTree), attr_offsets, next_offset,
                           &current_class_code, obj_sizes);
    fprintf (t_out, "};\n\n");


    fprintf (t_out, "static short dAttributeType [%d][%d] = {\n", NumClasses, NumAttrSyms);

    attr_types = (short *) malloc (sizeof(short) * NumAttrSyms);
    for (a = 0; a < NumAttrSyms; a++)
        attr_types[a] = -1;

    w_dump_c_attr_types (nodeGet(SchemaRoot, qClassTree), attr_types);

    fprintf (t_out, "};\n\n");


    fprintf (t_out, "static short dAttributeTags [%d][%d] = {\n", NumClasses, NumAttrSyms);

    attr_tags = (short *) malloc (sizeof(short) * NumAttrSyms);
    for (a = 0; a < NumAttrSyms; a++)
        attr_tags[a] = 0;

    w_dump_c_attr_tags (nodeGet(SchemaRoot, qClassTree), attr_tags);

    fprintf (t_out, "};\n\n");


    current_class_code = -1;

    fprintf (t_out, "static short dClassIsA [%d][%d] = {\n", NumClasses, NumClasses);

    w_dump_c_class_isa (nodeGet(SchemaRoot, qClassTree), &current_class_code, obj_isa);
    fprintf (t_out, "};\n\n");


    fprintf (t_out, "\n/* Public Data */\n\n");

    fprintf (t_out, "short dObjectSize [%d] = {\n\t", NumClasses);
    for (c = 0; c < NumClasses; c++)
    {
        fprintf (t_out, "%d, ", obj_sizes[c]);
        if ((c & 15) == 15)
            fprintf (t_out, "\n\t");
    }
    fprintf (t_out, "\n};\n\n");

    fprintf (t_out, "int   dObjects = %d;\n", NumClasses);
    fprintf (t_out, "char *dObjectName [%d] = {\n", NumClasses);

    w_dump_c_class_names (nodeGet(SchemaRoot, qClassTree));

    fprintf (t_out, "};\n\n");

    fprintf (t_out, "int   dAttributes = %d;\n", NumAttrSyms);
    fprintf (t_out, "char *dAttributeName [%d] = {\n", NumAttrSyms);

    w_dump_c_attr_names ();

    fprintf (t_out, "};\n\n");



    fprintf (t_out, "\n/* Public Functions */\n\n");

    fprintf (t_out, "short dGetAttributeOffset (class, attribute)      \n");
    fprintf (t_out, "short                      class;                 \n");
    fprintf (t_out, "short                      attribute;             \n");
    fprintf (t_out, "{                                                 \n");
    fprintf (t_out, "    return (dAttributeOffset[class][attribute]);  \n");
    fprintf (t_out, "}                                                 \n");
    fprintf (t_out, "                                                  \n");
    fprintf (t_out, "short dGetAttributeType (class, attribute)        \n");
    fprintf (t_out, "short                    class;                   \n");
    fprintf (t_out, "short                    attribute;               \n");
    fprintf (t_out, "{                                                 \n");
    fprintf (t_out, "    return (dAttributeType[class][attribute]);    \n");
    fprintf (t_out, "}                                                 \n");
    fprintf (t_out, "                                                  \n");
    fprintf (t_out, "short dGetAttributeTags (class, attribute)        \n");
    fprintf (t_out, "short                    class;                   \n");
    fprintf (t_out, "short                    attribute;               \n");
    fprintf (t_out, "{                                                 \n");
    fprintf (t_out, "    return (dAttributeTags[class][attribute]);    \n");
    fprintf (t_out, "}                                                 \n");
    fprintf (t_out, "                                                  \n");
    fprintf (t_out, "int   dGetClassIsA      (class, isaclass)         \n");
    fprintf (t_out, "short                    class;                   \n");
    fprintf (t_out, "short                    isaclass;                \n");
    fprintf (t_out, "{                                                 \n");
    fprintf (t_out, "    return (dClassIsA[class][isaclass]);          \n");
    fprintf (t_out, "}                                                 \n");
    fprintf (t_out, "                                                  \n");
}


w_dump_ssl_class (root)
NODE_PTR          root;
{
    NODE_PTR   NP;

    for (NP = root; NP != NULL; NP = nodeNext(NP))
    {
        fprintf (t_ssl_out, "\t%s\n", t_getIdString(nodeGetValue(NP, qIdent)));
        w_dump_ssl_class (nodeGet(NP, qDerived));
    }
}

w_dump_ssl_attr ()
{
    NODE_PTR   NP;

    for (NP = nodeGet(SchemaRoot, qAttrSyms); NP != NULL; NP = nodeNext(NP))
    {
        fprintf (t_ssl_out, "\t%s\n", t_getIdString(nodeGetValue(NP, qIdent)));
    }
}

w_dump_c_attr_offsets (NP, parent_offsets, parent_next_offset,
                       class_code_ptr, obj_sizes)
NODE_PTR               NP;
short                 *parent_offsets;
short                  parent_next_offset;
int                   *class_code_ptr;
short                 *obj_sizes;
{
    short    *attr_offsets;
    short     next_offset;
    short     a;
    NODE_PTR  Attr;
    NODE_PTR  Derived;
    long      attr_code;

    attr_offsets = (short *) malloc (sizeof(short) * NumAttrSyms);

    for (; NP != NULL; NP = nodeNext(NP))
    {
        (*class_code_ptr)++;

        /* Derive attributes from parent */

        for (a = 0; a < NumAttrSyms; a++)
            attr_offsets[a] = parent_offsets[a];

        next_offset = parent_next_offset;

        /* Add attributes to this class */

        for (Attr = nodeGet(NP, qAttrs); Attr != NULL; Attr = nodeNext(Attr))
        {
            attr_code = nodeGetValue(nodeGet(Attr, qAttrSym), qCode);
            attr_offsets[attr_code] = next_offset;
            next_offset += 4;
        }

        obj_sizes[*class_code_ptr] = next_offset;

        /* Print class layout to file */

        fprintf (t_out, "\t");
        for (a = 0; a < NumAttrSyms; a++)
        {
            fprintf (t_out, "%3d, ", attr_offsets[a]);
        }
        fprintf (t_out, "\n");

        /* Now derive any other classes from this class */

        Derived = nodeGet(NP, qDerived);
        if (Derived != NULL)
        {
            w_dump_c_attr_offsets (Derived, attr_offsets, next_offset,
                                   class_code_ptr, obj_sizes);
        }
    }

    free (attr_offsets);
}


w_dump_c_attr_types   (NP, parent_attr_types)
NODE_PTR               NP;
short                 *parent_attr_types;
{
    short    *attr_types;
    short     a;
    NODE_PTR  Attr;
    NODE_PTR  Derived;
    long      attr_code;

    attr_types = (short *) malloc (sizeof(short) * NumAttrSyms);

    for (; NP != NULL; NP = nodeNext(NP))
    {

        /* Derive attributes from parent */

        for (a = 0; a < NumAttrSyms; a++)
            attr_types[a] = parent_attr_types[a];

        /* Add attributes to this class */

        for (Attr = nodeGet(NP, qAttrs); Attr != NULL; Attr = nodeNext(Attr))
        {
            attr_code = nodeGetValue(nodeGet(Attr, qAttrSym), qCode);
            attr_types[attr_code] = nodeGetValue(Attr, qType);
        }

        /* Print to file */

        fprintf (t_out, "\t");
        for (a = 0; a < NumAttrSyms; a++)
        {
            fprintf (t_out, "%3d, ", attr_types[a]);
        }
        fprintf (t_out, "\n");

        /* Now derive any other classes from this class */

        Derived = nodeGet(NP, qDerived);
        if (Derived != NULL)
        {
            w_dump_c_attr_types (Derived, attr_types);
        }
    }

    free (attr_types);
}


w_dump_c_attr_tags    (NP, parent_attr_tags)
NODE_PTR               NP;
short                 *parent_attr_tags;
{
    short    *attr_tags;
    short     a;
    NODE_PTR  Attr;
    NODE_PTR  Derived;
    long      attr_code;

    attr_tags = (short *) malloc (sizeof(short) * NumAttrSyms);

    for (; NP != NULL; NP = nodeNext(NP))
    {

        /* Derive attributes from parent */

        for (a = 0; a < NumAttrSyms; a++)
            attr_tags[a] = parent_attr_tags[a];

        /* Add attributes to this class */

        for (Attr = nodeGet(NP, qAttrs); Attr != NULL; Attr = nodeNext(Attr))
        {
            attr_code = nodeGetValue(nodeGet(Attr, qAttrSym), qCode);
            attr_tags[attr_code] = nodeGetValue(Attr, qTags);
        }

        /* Print to file */

        fprintf (t_out, "\t");
        for (a = 0; a < NumAttrSyms; a++)
        {
            fprintf (t_out, "%3d, ", attr_tags[a]);
        }
        fprintf (t_out, "\n");

        /* Now derive any other classes from this class */

        Derived = nodeGet(NP, qDerived);
        if (Derived != NULL)
        {
            w_dump_c_attr_tags (Derived, attr_tags);
        }
    }

    free (attr_tags);
}


w_dump_c_class_names (root)
NODE_PTR              root;
{
    NODE_PTR    NP;

    for (NP = nodeFirst(root); NP != NULL; NP = nodeNext(NP))
    {
        fprintf (t_out, "\t\"%s\",\n", t_getIdString(nodeGetValue(NP, qIdent)));
        w_dump_c_class_names (nodeGet(NP, qDerived));
    }
}

w_dump_c_attr_names ()
{
    NODE_PTR    NP;

    for (NP = nodeGet(SchemaRoot, qAttrSyms); NP != NULL; NP = nodeNext(NP))
    {
        fprintf (t_out, "\t\"%s\",\n", t_getIdString(nodeGetValue(NP, qIdent)));
    }
}

w_dump_c_class_isa (NP, class_code_ptr, parent_obj_isa)
NODE_PTR               NP;
int                   *class_code_ptr;
char                  *parent_obj_isa;
{
    char     *obj_isa;
    short     o;
    NODE_PTR  Derived;

    obj_isa = (char *) malloc (sizeof(char) * NumClasses);

    for (; NP != NULL; NP = nodeNext(NP))
    {
        (*class_code_ptr)++;

        /* Derive IsA flags from parent */

        for (o = 0; o < NumClasses; o++)
            obj_isa[o] = parent_obj_isa[o];

        /* We are also our own object class */

        obj_isa[(*class_code_ptr)] = 1;


        /* Print IsA flags to file */

        fprintf (t_out, "\t");
        for (o = 0; o < NumClasses; o++)
        {
            fprintf (t_out, "%1d, ", obj_isa[o]);
        }
        fprintf (t_out, "\n");

        /* Now derive any other classes from this class */

        Derived = nodeGet(NP, qDerived);
        if (Derived != NULL)
        {
            w_dump_c_class_isa (Derived, class_code_ptr, obj_isa);
        }
    }

    free (obj_isa);
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

    if (w_debug && w_walking)
        longjmp (t_fatal_jmp_buf, 1);

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

char *w_rule_name (pc)
short        pc; 
{
    short    i;

    for (i = 1; i < w_ruleTableSize; i++)
    {
        if (pc < w_ruleTable[i].addr)
            return (w_ruleTable[i-1].name);
    }
    return (w_ruleTable[i-1].name);
}

short w_rule_addr (name)
char        *name;
{
    short    i;

    for (i = 0; i < w_ruleTableSize; i++)
    {
        if (strcmp (name, w_ruleTable[i].name) == 0)
            return (w_ruleTable[i].addr);
    }
    printf ("Unknown rule name: '%s'\n", name);
    return ((short) -1);
}

