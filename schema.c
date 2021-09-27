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
*   08/24/93 |        | Repackaged to use generic SSL runtime module.
*
*****************************************************************************
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef AMIGA
#include <dos.h>
#endif // AMIGA

/*  SSL Runtime module  */
#include "ssl_rt.h"


/* Schema database access functions */
#include "node.h"


#define  SSL_INCLUDE_ERR_TABLE
#include "schema.h"

#include "debug.h"


#define DEBUG_FILE   "schema.dbg"         /* SSL debug information */
#define PROGRAM_FILE "schema.lst"


#include "schema.tbl"


// Defined in schema_scan.c but no header file for that yet!
void init_my_scanner();


char     input_filename[256];             /* schema source */


FILE    *f_lst;                           /* listing file                   */
FILE    *f_out;                           /* C code output                  */
FILE    *f_ssl_out;                       /* SSL code output                */



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

short w_define_obj ();                    /* Define an object given an id   */
short w_define_attr ();                   /* Define an attribute given id   */
// w_define_attr_of_obj ();               /* ...given attr # and object #   */
// w_derive_obj ();                       /* obj1 derives from obj2         */
void w_dump_table ();
NODE_PTR w_find_class();
void w_dump_ssl_class ( NODE_PTR root );
void w_dump_ssl_attr();
void w_dump_c_attr_offsets ( NODE_PTR NP, short* parent_offsets, short parent_next_offset,
                             int* class_code_ptr, short* obj_sizes );
void w_dump_c_attr_types ( NODE_PTR NP, short* parent_attr_types );
void w_dump_c_attr_tags( NODE_PTR NP, short* parent_attr_tags );
void w_dump_c_class_names ( NODE_PTR root );
void w_dump_c_attr_names();
void w_dump_c_class_isa ( NODE_PTR NP, int* class_code_ptr, char* parent_obj_isa );


char           C_out_filename[100];
char           SSL_out_filename[100];
char           list_filename[256];

int            option_list;
int            option_debug;


void print_node( char* variable, char* udata );
void print_long( char* variable, char* udata );

dbg_variables debug_variables[] =
{
    /* "Name",         address,                    udata,    function */

    "SchemaRoot",      (char *) &SchemaRoot,       0,        print_node,
    "CurrentClass",    (char *) &CurrentClass,     0,        print_node,
    "GetsAttrsClass",  (char *) &GetsAttrsClass,   0,        print_node,
    "CurrentAttrSym",  (char *) &CurrentAttrSym,   0,        print_node,

    "NumAttrSyms",     (char *) &NumAttrSyms,      0,        print_long,
    "NumClasses",      (char *) &NumAttrSyms,      0,        print_long,

    "",                NULL,                       0,        NULL,
};

void  my_listing_function ( char* source_line, int token_accepted );
void  init_my_operations ();
void  open_my_files ();
void  close_my_files ();
int   t_hitBreak ();


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

void
main ( int argc, char* argv[] )
{
    short   arg;
    int     i;
    int     status;


    /* check command line arguments */

    option_list = 0;
    option_debug = 0;

    for (arg = 1; arg<argc && argv[arg][0]=='-'; arg++)
    {
        for (i = 1; argv[arg][i]; i++)
        {
            if (argv[arg][i]=='L' || argv[arg][i]=='l')
                option_list = 1;
            else if (argv[arg][i]=='D' || argv[arg][i]=='d')
                option_debug = 1;
        }
    }

    if (arg >= argc)
    {
        printf("Usage:  schema [-l] [-d] <file>\n");
        printf("        -l : produce listing file\n");
        printf("        -d : invoke full screen debugger\n");
        exit(10);
    }


    sprintf (input_filename,   "%s.schema",     argv[arg]);

    sprintf (C_out_filename,   "%s_schema.c",   argv[arg]);
    sprintf (SSL_out_filename, "%s_schema.ssl", argv[arg]);
    sprintf (list_filename,    "%s_schema.lst", argv[arg]);

    open_my_files ();

    ssl_init ();

    init_my_scanner ();

    ssl_set_debug (option_debug);
    ssl_set_debug_info (DEBUG_FILE, PROGRAM_FILE, oBreak, debug_variables);

    ssl_set_input_filename (input_filename);

    ssl_set_recovery_token (pDERIVES);

    if (option_list)
        ssl_set_listing_callback (my_listing_function);

    ssl_set_init_operations_callback (init_my_operations);

#ifdef AMIGA
    onbreak(&t_hitBreak);
#endif // AMIGA


    /* execute SSL program */

    status = ssl_run_program ();

    close_my_files ();

    if (status != 0)
        exit(5);

    exit(0);
}


void
my_listing_function ( char* source_line, int token_accepted )
{

    fprintf(f_lst, "====  %s", source_line);
}


void
open_my_files ()
{

    if ((f_out=fopen(C_out_filename,"w"))==NULL)
    {
        printf ("Can't open output file %s\n", C_out_filename);
        exit (10);
    }

    if ((f_ssl_out=fopen(SSL_out_filename,"w"))==NULL)
    {
        printf ("Can't open output file %s\n", SSL_out_filename);
        exit (10);
    }

    if (option_list)
    {
        if ((f_lst=fopen(list_filename, "w"))==NULL)
        {
            printf ("Can't open listing file %s\n", list_filename);
            exit (10);
        }
    }
}


void
close_my_files ()
{
    fclose (f_out);
    fclose (f_ssl_out);
    if (option_list)
        fclose (f_lst);
}


int
t_hitBreak()
{
    printf("Breaking...\n");
    close_my_files ();
    return(1);
}


/*********** S e m a n t i c   O p e r a t i o n s ***********/

void
init_my_operations ()
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


/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */


#include "ssl_begin.h"

       /* mechanism Class */

       case oCreateEmptySchema :
              SchemaRoot = nodeNew ((short) nSchema);

              dTemp = ssl_add_id ("nINVALID", pIDENTIFIER);   /* Add to ident table */
              dNode = nodeNew ((short) nClass);
              nodeSetValue (dNode, qIdent, (long)dTemp);
              nodeAppend (SchemaRoot, qClassTree, dNode);
              NumClasses++;

              dTemp = ssl_add_id ("Object", pIDENTIFIER);   /* Add to ident table */
              dNode = nodeNew ((short) nClass);
              nodeSetValue (dNode, qIdent, (long)dTemp);
              nodeAppend (SchemaRoot, qClassTree, dNode);
              NumClasses++;

              dTemp = ssl_add_id ("qINVALID", pIDENTIFIER);   /* Add to ident table */
              dNode = nodeNew ((short) nAttrSym);
              nodeSetValue (dNode, qIdent, (long)dTemp);
              nodeSetValue (dNode, qCode, (long) NumAttrSyms);
              nodeAppend (SchemaRoot, qAttrSyms, dNode);
              NumAttrSyms++;

              continue;
       case oFindClass :
              CurrentClass = w_find_class (nodeGet(SchemaRoot, qClassTree), (long) ssl_last_id);
              if (CurrentClass == NULL)
                  ssl_fatal("Undefined class");
              continue;
       case oDeriveClass :
              /* Verify class not already defined */
              if (w_find_class (nodeGet(SchemaRoot, qClassTree), (long) ssl_last_id) != NULL)
                  ssl_fatal("Multiply defined class");
              dNode = nodeNew ((short) nClass);
              nodeSetValue (dNode, qIdent, (long) ssl_last_id);
              nodeAppend (CurrentClass, qDerived, dNode);
              NumClasses++;
              continue;
       case oThisClassWillGetAttrs :
              GetsAttrsClass = CurrentClass;
              continue;
       case oNoClassWillGetAttrs :
              GetsAttrsClass = NULL;
              continue;
       case oAClassWillGetAttrs :
              ssl_result = (GetsAttrsClass != NULL);
              continue;


       /* mechanism Attr */

       case oCreateAttrSym :
              for (dNode = nodeGet(SchemaRoot, qAttrSyms); dNode != NULL; dNode = nodeNext(dNode))
                  if (nodeGetValue(dNode, qIdent) == ssl_last_id)
                      break;
              if (dNode == NULL)
              {
                  dNode = nodeNew ((short) nAttrSym);
                  nodeSetValue (dNode, qIdent, (long) ssl_last_id);
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
              nodeSetValue (CurrentAttr, qType, (long) ssl_param);
              continue;
       case oAttrTag :
              dLong = nodeGetValue (CurrentAttr, qTags);
              nodeSetValue (CurrentAttr, qTags, (long) (dLong | ssl_param));
              continue;

       /* mechanism doc */

       case oDocDumpTable :
              w_dump_table ();
              continue;

#include "ssl_end.h"

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */


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


void
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

    fprintf (f_ssl_out, "\n%% Generated automatically by schema\n\n");

    fprintf (f_ssl_out, "type node_type:\n");

    w_dump_ssl_class (nodeGet(SchemaRoot, qClassTree));

    fprintf (f_ssl_out, "\t;\n\n");

    fprintf (f_ssl_out, "type node_attribute:\n");

    w_dump_ssl_attr ();

    fprintf (f_ssl_out, "\t;\n\n");

    printf ("------- Writing C code to %s --------\n", C_out_filename);

    fprintf (f_out, "\n/* Generated automatically by schema */\n\n");

    fprintf (f_out, "/* Private Data */\n\n");



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

    fprintf (f_out, "static short dAttributeOffset [%ld][%ld] = {\n", NumClasses, NumAttrSyms);

    w_dump_c_attr_offsets (nodeGet(SchemaRoot, qClassTree), attr_offsets, next_offset,
                           &current_class_code, obj_sizes);
    fprintf (f_out, "};\n\n");


    fprintf (f_out, "static short dAttributeType [%ld][%ld] = {\n", NumClasses, NumAttrSyms);

    attr_types = (short *) malloc (sizeof(short) * NumAttrSyms);
    for (a = 0; a < NumAttrSyms; a++)
        attr_types[a] = -1;

    w_dump_c_attr_types (nodeGet(SchemaRoot, qClassTree), attr_types);

    fprintf (f_out, "};\n\n");


    fprintf (f_out, "static short dAttributeTags [%ld][%ld] = {\n", NumClasses, NumAttrSyms);

    attr_tags = (short *) malloc (sizeof(short) * NumAttrSyms);
    for (a = 0; a < NumAttrSyms; a++)
        attr_tags[a] = 0;

    w_dump_c_attr_tags (nodeGet(SchemaRoot, qClassTree), attr_tags);

    fprintf (f_out, "};\n\n");


    current_class_code = -1;

    fprintf (f_out, "static short dClassIsA [%ld][%ld] = {\n", NumClasses, NumClasses);

    w_dump_c_class_isa (nodeGet(SchemaRoot, qClassTree), &current_class_code, obj_isa);
    fprintf (f_out, "};\n\n");


    fprintf (f_out, "\n/* Public Data */\n\n");

    fprintf (f_out, "short dObjectSize [%ld] = {\n\t", NumClasses);
    for (c = 0; c < NumClasses; c++)
    {
        fprintf (f_out, "%d, ", obj_sizes[c]);
        if ((c & 15) == 15)
            fprintf (f_out, "\n\t");
    }
    fprintf (f_out, "\n};\n\n");

    fprintf (f_out, "int   dObjects = %ld;\n", NumClasses);
    fprintf (f_out, "char *dObjectName [%ld] = {\n", NumClasses);

    w_dump_c_class_names (nodeGet(SchemaRoot, qClassTree));

    fprintf (f_out, "};\n\n");

    fprintf (f_out, "int   dAttributes = %ld;\n", NumAttrSyms);
    fprintf (f_out, "char *dAttributeName [%ld] = {\n", NumAttrSyms);

    w_dump_c_attr_names ();

    fprintf (f_out, "};\n\n");



    fprintf (f_out, "\n/* Public Functions */\n\n");

    fprintf (f_out, "short dGetAttributeOffset (class, attribute)      \n");
    fprintf (f_out, "short                      class;                 \n");
    fprintf (f_out, "short                      attribute;             \n");
    fprintf (f_out, "{                                                 \n");
    fprintf (f_out, "    return (dAttributeOffset[class][attribute]);  \n");
    fprintf (f_out, "}                                                 \n");
    fprintf (f_out, "                                                  \n");
    fprintf (f_out, "short dGetAttributeType (class, attribute)        \n");
    fprintf (f_out, "short                    class;                   \n");
    fprintf (f_out, "short                    attribute;               \n");
    fprintf (f_out, "{                                                 \n");
    fprintf (f_out, "    return (dAttributeType[class][attribute]);    \n");
    fprintf (f_out, "}                                                 \n");
    fprintf (f_out, "                                                  \n");
    fprintf (f_out, "short dGetAttributeTags (class, attribute)        \n");
    fprintf (f_out, "short                    class;                   \n");
    fprintf (f_out, "short                    attribute;               \n");
    fprintf (f_out, "{                                                 \n");
    fprintf (f_out, "    return (dAttributeTags[class][attribute]);    \n");
    fprintf (f_out, "}                                                 \n");
    fprintf (f_out, "                                                  \n");
    fprintf (f_out, "int   dGetClassIsA      (class, isaclass)         \n");
    fprintf (f_out, "short                    class;                   \n");
    fprintf (f_out, "short                    isaclass;                \n");
    fprintf (f_out, "{                                                 \n");
    fprintf (f_out, "    return (dClassIsA[class][isaclass]);          \n");
    fprintf (f_out, "}                                                 \n");
    fprintf (f_out, "                                                  \n");
}


void
w_dump_ssl_class ( NODE_PTR root )
{
    NODE_PTR   NP;

    for (NP = root; NP != NULL; NP = nodeNext(NP))
    {
        fprintf (f_ssl_out, "\t%s\n", ssl_get_id_string(nodeGetValue(NP, qIdent)));
        w_dump_ssl_class (nodeGet(NP, qDerived));
    }
}


void
w_dump_ssl_attr ()
{
    NODE_PTR   NP;

    for (NP = nodeGet(SchemaRoot, qAttrSyms); NP != NULL; NP = nodeNext(NP))
    {
        fprintf (f_ssl_out, "\t%s\n", ssl_get_id_string(nodeGetValue(NP, qIdent)));
    }
}


void
w_dump_c_attr_offsets ( NODE_PTR NP, short* parent_offsets, short parent_next_offset,
                        int* class_code_ptr, short* obj_sizes )
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
            // SARICE 9/27/2021: 
            // Padding for worst case, not optimized for type.
            // This code was originally written for 32-bit.
            // next_offset += 4;
            next_offset += 8;
        }

        obj_sizes[*class_code_ptr] = next_offset;

        /* Print class layout to file */

        fprintf (f_out, "\t");
        for (a = 0; a < NumAttrSyms; a++)
        {
            fprintf (f_out, "%3d, ", attr_offsets[a]);
        }
        fprintf (f_out, "\n");

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


void
w_dump_c_attr_types   ( NODE_PTR NP, short* parent_attr_types )
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

        fprintf (f_out, "\t");
        for (a = 0; a < NumAttrSyms; a++)
        {
            fprintf (f_out, "%3d, ", attr_types[a]);
        }
        fprintf (f_out, "\n");

        /* Now derive any other classes from this class */

        Derived = nodeGet(NP, qDerived);
        if (Derived != NULL)
        {
            w_dump_c_attr_types (Derived, attr_types);
        }
    }

    free (attr_types);
}


void
w_dump_c_attr_tags( NODE_PTR NP, short* parent_attr_tags )
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

        fprintf (f_out, "\t");
        for (a = 0; a < NumAttrSyms; a++)
        {
            fprintf (f_out, "%3d, ", attr_tags[a]);
        }
        fprintf (f_out, "\n");

        /* Now derive any other classes from this class */

        Derived = nodeGet(NP, qDerived);
        if (Derived != NULL)
        {
            w_dump_c_attr_tags (Derived, attr_tags);
        }
    }

    free (attr_tags);
}


void
w_dump_c_class_names ( NODE_PTR root )
{
    NODE_PTR    NP;

    for (NP = nodeFirst(root); NP != NULL; NP = nodeNext(NP))
    {
        fprintf (f_out, "\t\"%s\",\n", ssl_get_id_string(nodeGetValue(NP, qIdent)));
        w_dump_c_class_names (nodeGet(NP, qDerived));
    }
}


void
w_dump_c_attr_names ()
{
    NODE_PTR    NP;

    for (NP = nodeGet(SchemaRoot, qAttrSyms); NP != NULL; NP = nodeNext(NP))
    {
        fprintf (f_out, "\t\"%s\",\n", ssl_get_id_string(nodeGetValue(NP, qIdent)));
    }
}


void
w_dump_c_class_isa ( NODE_PTR NP, int* class_code_ptr, char* parent_obj_isa )
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

        fprintf (f_out, "\t");
        for (o = 0; o < NumClasses; o++)
        {
            fprintf (f_out, "%1d, ", obj_isa[o]);
        }
        fprintf (f_out, "\n");

        /* Now derive any other classes from this class */

        Derived = nodeGet(NP, qDerived);
        if (Derived != NULL)
        {
            w_dump_c_class_isa (Derived, class_code_ptr, obj_isa);
        }
    }

    free (obj_isa);
}


/*  Callbacks to display variables in debugger  */

void
print_node (variable, udata)
char       *variable;
char       *udata;
{
    NODE_PTR  node_ptr = *((NODE_PTR *) variable);

    nodeDumpNodeShort (node_ptr);
}


void
print_long (variable, udata)
char       *variable;
char       *udata;
{
    printf ("%ld\n", *((long *) variable));
}

