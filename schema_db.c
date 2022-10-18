
/*
*****************************************************************************
*
*   Schema
*
*   by Steve Rice
*
*   March 19, 1991
*
*****************************************************************************
*
*   schema_db.c         Run-time schema database functions
*
*   These functions should be linked to an application using a schema.
*
*   HISTORY
*----------------------------------------------------------------------------
*   05/11/93 | Steve  | Moved from vmain.c into a generic package.
*   06/09/93 | Steve  | Moved from verilog analyzer into schema.
*   04/20/94 | Steve  | New node, list representations.  New PI functions.
*
*****************************************************************************
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef AMIGA
#include <dos.h>
#define bzero(p,n)   memset(p,'\0',n)
#endif /* AMIGA */


#include "schema_db.h"


/*
 *  Schema table access primitives, defined in <schema>_schema.c
 */

short    dGetAttributeOffset ( short theClass, short attribute );
short    dGetAttributeType   ( short attribute );
short    dGetAttributeTags   ( short theClass, short attribute );
int      dGetClassIsA        ( short theClass, short isaClass );

extern short    dObjectSize [];
extern int      dObjects;
extern char    *dObjectName [];
extern int      dAttributes;
extern char    *dAttributeName [];
extern short    dAttributeType [];


/*
 *  SCHEMA globals
 */

static long     SCH_next_node_num;
char            SCH_errbuf [1024];

/*
 * For debugging
 */

typedef struct node_lookup_struct  *NodeLookup;

struct node_lookup_struct {
    Node         node;
    NodeLookup   next;
};

static NodeLookup  SCH_node_lookup_list;


/*
 * For loading a database from a file
 */

typedef struct node_load_struct  *NodeLoad;

struct node_load_struct {
    Node         node;
    int          orig_num;   /* Node number as recorded in save file */
    NodeLoad     next;
};

static NodeLoad  SCH_node_load_list;

#define SCH_MAX_DB_LINE  10000
char     SCH_db_line [SCH_MAX_DB_LINE];
Node SCH_LookupNodeLoad (/* orig_num */);


/* Forward declarations */

void    SCH_Traverse (/* root, callback, udata */);
int     SCH_SaveNodeAscii (/* N, fp */);
Node    SCH_LoadNodeAscii (/* fp */);
void    SCH_LoadPatchNode ( Node N );


/*
 *  SCH_Init ()
 *
 *  Application must call this before using the SCHEMA database.
 */

void    SCH_Init ()
{
    SCH_next_node_num = 1;
    SCH_node_lookup_list = NULL;
}

/*
 *  SCH_Term ()
 */

void    SCH_Term ()
{
}


/*
 *  Nodes
 */
 
Node   NewNode (object_type)
int             object_type;
{
    int          size;
    Node         N;
    NodeLookup   nl;

    size = sizeof(struct SCH_NodeStruct) - sizeof(long) + dObjectSize[object_type];
    N = (Node) malloc (size);
    if (N == NULL) return (NULL);

    bzero (N, size);
    N->node_num  = SCH_next_node_num ++;
    N->kind      = object_type;
    N->signature = SCH_NODE_SIGNATURE;

    /* For debugging ... shouldn't really do this */
    nl = (NodeLookup) malloc(sizeof(struct node_lookup_struct));
    nl->node = N;
    nl->next = SCH_node_lookup_list;
    SCH_node_lookup_list = nl;

    return (N);
}

int   Kind (N)
Node        N;
{
    SCH_VERIFY_NODE (N,0);
    return (N->kind);
}

/* Note, this takes a Kind, not a Node */
Boolean1  IsA (type_1, type_2)
int            type_1;
int            type_2;
{
    return (dGetClassIsA(type_1, type_2));
}

Boolean1    IsNull (N)
Node                N;
{
    return (N == NULL);
}


void
SetAttr( Node N, int attr_code, void* value )
{
    void      **attr_ptr;
    SCH_VERIFY_NODE (N,);

    if (value != NULL)   /* can assign NULL to any type */
    {
        switch (dAttributeType[attr_code])
        {
            case SCH_Type_StringN:
                value = strdup (value);
                break;

            case SCH_Type_Node:
                SCH_VERIFY_NODE ((Node)value,);
                break;

            case SCH_Type_List:
                SCH_VERIFY_LIST ((List)value,);
                break;
        }
    }

    attr_ptr = SCH_GetAttrPtr (N, attr_code);
    *attr_ptr = value;
}


// Set a non-pointer value
//
void
SetValue( Node N, int attr_code, long value )
{
    void      **attr_ptr;
    attr_ptr = SCH_GetAttrPtr (N, attr_code);
    *(long*)attr_ptr = value;
}


void*
GetAttr( Node N, int attr_code )
{
    void  **attr_ptr;
    void   *value;
    SCH_VERIFY_NODE (N,NULL);

    attr_ptr = SCH_GetAttrPtr (N, attr_code);
    value    = *attr_ptr;

    return (value);
}


// Get a non-pointer value
//
long
GetValue( Node N, int attr_code )
{
    void  **attr_ptr = SCH_GetAttrPtr (N, attr_code);
    return *(long*)attr_ptr;
}


void   FreeNode (N, recurse)
Node             N;
Boolean1         recurse;
{
    if (N == NULL)
        return;
    SCH_VERIFY_NODE (N,);

    free (N);    /* TO DO: recurse */
}


// Node methods that auto-create a List attriute.

void
NodeAddLast( Node N, int attr_code, Node value )
{
    List L = (List) GetAttr( N, attr_code );
    if ( !L ) {
        // Unfortunately we don't know the constraint type
        // given in the schema.  So allow anything.
        L = NewList( 1 );  // 1 = Object, in any schema
        SetAttr( N, attr_code, L );
    }
    AddLast( L, value );
}


/*
 *  Lists
 */

List   NewList (object_type)
int             object_type;
{
    List         L;

    L = (List) malloc (sizeof(struct SCH_ListStruct));
    if (L == NULL) return (NULL);

    bzero (L, sizeof(struct SCH_ListStruct));
    L->kind      = object_type;
    L->signature = SCH_LIST_SIGNATURE;

    return (L);
}

Item   AddFirst (L, N)
List             L;
Node             N;
{
    Item     I;
    SCH_VERIFY_LIST (L,NULL);

    I = SCH_NewItem (L);
    I->value = N;

    if (L->first == NULL)
    {
        L->first = I;
        L->last = I;
    }
    else
    {
        I->next = L->first;
        L->first->prev = I;
        L->first = I;
    }

    L->length++;

    return (I);
}

Item   AddLast (L, N)
List            L;
Node            N;
{
    Item     I;
    SCH_VERIFY_LIST (L,NULL);

    I = SCH_NewItem (L);
    I->value = N;

    if (L->first == NULL)
    {
        L->first = I;
        L->last = I;
    }
    else
    {
        I->prev = L->last;
        L->last->next = I;
        L->last = I;
    }

    L->length++;

    return (I);
}

Item   AddNext (I_from, N)
Item            I_from;
Node            N;
{
    Item     I;
    List     L;
    SCH_VERIFY_ITEM (I_from,NULL);

    L = I_from->list;
    SCH_VERIFY_LIST (L,NULL);

    I = SCH_NewItem (L);
    I->value = N;

    if (I_from->next == NULL)
    {
        L->last = I;
        I->prev = I_from;
        I_from->next = I;
    }
    else
    {
        I->prev = I_from;
        I->next = I_from->next;
        I_from->next = I;
        I->next->prev = I;
    }

    L->length++;

    return (I);
}

Item   AddPrev (I_from, N)
Item            I_from;
Node            N;
{
    Item     I;
    List     L;
    SCH_VERIFY_ITEM (I_from,NULL);

    L = I_from->list;
    SCH_VERIFY_LIST (L,NULL);

    I = SCH_NewItem (L);
    I->value = N;

    if (I_from->prev == NULL)
    {
        L->first = I;
        I->next = I_from;
        I_from->prev = I;
    }
    else
    {
        I->next = I_from;
        I->prev = I_from->prev;
        I_from->prev = I;
        I->prev->next = I;
    }

    L->length++;

    return (I);
}

Node   RemoveFirst (L)
List                L;
{
    Item     I;
    Node     N;
    SCH_VERIFY_LIST (L,NULL);

    if ((I = L->first) == NULL)
        return (NULL);

    L->first = I->next;

    if (I->next == NULL)
        L->last = NULL;
    else
        I->next->prev = NULL;

    L->length--;
    N = I->value;
    free (I);
    return (N);
}

Node   RemoveLast (L)
List               L;
{
    Item     I;
    Node     N;
    SCH_VERIFY_LIST (L,NULL);

    if ((I = L->last) == NULL)
        return (NULL);

    if (I->prev == NULL)
        L->first = NULL;
    else
        I->prev->next = NULL;

    L->last = I->prev;

    L->length--;
    N = I->value;
    free (I);
    return (N);
}

Node   RemoveItem (I_from)
Item               I_from;
{
    List     L;
    Node     N;
    SCH_VERIFY_ITEM (I_from,NULL);

    if (I_from == NULL)
        return (NULL);

    L = I_from->list;
    SCH_VERIFY_LIST (L,NULL);

    if (I_from->prev == NULL)
        L->first = I_from->next;
    else
        I_from->prev->next = I_from->next;

    if (I_from->next == NULL)
        L->last = I_from->prev;
    else
        I_from->next->prev = I_from->prev;

    L->length--;
    N = I_from->value;
    free (I_from);
    return (N);
}

Item   FirstItem (L)
List              L;
{
    Item     I;
    if (L == NULL) return (NULL);
    SCH_VERIFY_LIST (L,NULL);

    I = L->first;
    return (I);
}

Item   LastItem (L)
List             L;
{
    Item     I;
    if (L == NULL) return (NULL);
    SCH_VERIFY_LIST (L,NULL);

    I = L->last;
    return (I);
}

Item   NextItem (I_from)
Item             I_from;
{
    Item     I;
    SCH_VERIFY_ITEM (I_from,NULL);

    I = I_from->next;
    return (I);
}

Item   PrevItem (I_from)
Item             I_from;
{
    Item     I;
    SCH_VERIFY_ITEM (I_from,NULL);

    I = I_from->prev;
    return (I);
}

Node   Value (I)
Item          I;
{
    SCH_VERIFY_ITEM (I,NULL);

    return (I->value);
}

Item   FindItem (L, N)
List             L;
Node             N;
{
    Item     I;
    SCH_VERIFY_LIST (L,NULL);
    SCH_VERIFY_NODE (N,NULL);

    for (I = L->first; I != NULL; I = I->next)
    {
        if (I->value == N)
            return (I);
    }

    return (NULL);
}

Item   NullItem ()
{
    return (NULL);
}

Boolean1   IsEmpty (L)
List                L;
{
    if (L == NULL)
        return (True);

    SCH_VERIFY_LIST (L,True);
    return (L->length == 0);
}
    
List   ListOf (I)
Item           I;
{
    SCH_VERIFY_ITEM (I,NULL);

    return (I->list);
}

int    ListLength (L)
List               L;
{
    if (L == NULL)
        return 0;

    SCH_VERIFY_LIST (L,0);
    return (L->length);
}

void   FreeList (L, recurse)
List             L;
Boolean1         recurse;
{
    if (L == NULL)
        return;

    /* TO DO: Free items */
    /* TO DO: recurse to nodes */

    SCH_VERIFY_LIST (L,);
    free (L);
}


/*
 *  Debugger support
 */

void   DumpNodeShort (N)
Node                  N;
{
    printf ("\t");

    if (N == NULL)
        printf ("Null\n");
    else
        printf ("%ld: %s\n", N->node_num, dObjectName[N->kind]);
}

void  DumpNodeLong (N)
Node                N;
{
    int      attr_code;
    void   **attr_ptr;
    int      indent;
    List     L;
    Item     I;

    indent = 0;

    if (N == NULL)
    {
        SCH_indent_printf (indent, "Null\n");
        return;
    }

    SCH_indent_printf (indent, "%s\t\t[%ld]\n", dObjectName[N->kind], N->node_num);

    indent += 4;

    for (attr_code = 1; attr_code < dAttributes; attr_code++)
    {
        if (dGetAttributeOffset(N->kind, attr_code) >= 0)
        {
            attr_ptr = SCH_GetAttrPtr (N, attr_code);
            SCH_indent_printf (indent, "%s:\t\t", dAttributeName[attr_code]);

            switch (dGetAttributeType(attr_code))
            {
                case SCH_Type_Boolean1:
                    printf ("%s\n", *(long*)attr_ptr ? "True" : "False");
                    break;

                case SCH_Type_Character1:
                    printf ("'%c'\n", *(char*)attr_ptr);
                    break;

                case SCH_Type_Integer4:
                    printf ("%ld\n", *(long*)attr_ptr);
                    break;

                case SCH_Type_StringN:
                    if (*(char**)attr_ptr == NULL)
                        printf ("Null\n");
                    else
                        printf ("\"%s\"\n", *(char**)attr_ptr);
                    break;

                case SCH_Type_Node:
                    if (*(Node *)attr_ptr == NULL)
                        printf ("Null\n");
                    else if (dGetAttributeTags(N->kind, attr_code) & SCH_Tag_Alt)
                        printf ("[Alt %ld]\n", (*(Node *)attr_ptr)->node_num);
                    else
                        printf ("[%ld]\n", (*(Node *)attr_ptr)->node_num);
                    break;

                case SCH_Type_List:
                    L = *(List *) attr_ptr;
                    if (L == NULL)
                        printf ("Null\n");
                    else if (IsEmpty(L))
                        printf ("Empty\n");
                    else
                    {
                        printf ("List [");
                        for (I = FirstItem(L); I != NULL; I = NextItem(I))
                        {
                            printf (" %ld", Value(I)->node_num);
                        }
                        printf ("]\n");
                    }
                    break;

                default:
                    SCH_indent_printf (indent, "%s:\t\tUnknown attribute type %d",
                                       dAttributeName[attr_code],
                                       dGetAttributeType (attr_code));
            }
        }
    }

    indent -= 4;
}


/*
 *  Internal use only.
 */

void **SCH_GetAttrPtr (N, attr_code)
Node                   N;
int                    attr_code;
{
    int       offset;
    void    **attr_ptr;

    if ((N->kind >= dObjects) || (N->kind <  0))
        SCH_Fatal ("Illegal object type observed in node");

    if ((attr_code >= dAttributes) || (attr_code < 0))
        SCH_Fatal ("Illegal attribute type");

    offset = dGetAttributeOffset (N->kind, attr_code);

    if (offset < 0)
    {
        sprintf (SCH_errbuf, "Object '%s' does not have attribute '%s'",
                 dObjectName [N->kind], dAttributeName [attr_code]);
        SCH_Fatal (SCH_errbuf);
    }

    return ((void **) (((char *) &(N->attribute[0])) + offset));
}

Item  SCH_NewItem (L)
List               L;
{
    Item   I;

    I = (Item) malloc (sizeof(struct SCH_ItemStruct));
    if (I == NULL) return (NULL);

    bzero (I, sizeof(struct SCH_ItemStruct));
    I->signature = SCH_ITEM_SIGNATURE;
    I->list = L;

    return (I);
}

void
SCH_indent_printf ( int indent, char* message, ... )
{
    va_list    arglist;
    int        i;

    for (i = 0; i < indent; i++)
        putchar (' ');

    va_start( arglist, message );
    vprintf( message, arglist );
    va_end( arglist );
}


Node SCH_LookupNode (node_num)
int                  node_num;
{
    NodeLookup    NL;
    
    for (NL = SCH_node_lookup_list; NL != NULL; NL = NL->next)
        if (NL->node->node_num == node_num)
            return (NL->node);

    return (NULL);
}

/* Used during Load */
Node SCH_LookupNodeLoad (orig_num)
int                      orig_num;
{
    NodeLoad     nld;

    for (nld = SCH_node_load_list; nld != NULL; nld = nld->next)
        if (nld->orig_num == orig_num)
            return (nld->node);

    return (NULL);
}


char  *SCH_GetTypeName (type)
int                     type;
{
    switch (type)
    {
        case SCH_Type_Boolean1:    return ("Boolean1");
        case SCH_Type_Character1:  return ("Character1");
        case SCH_Type_Integer4:    return ("Integer4");
        case SCH_Type_StringN:     return ("StringN");
        case SCH_Type_Node:        return ("Node");
        case SCH_Type_List:        return ("List");

        default:                   return ("Integer4");
    }
}

/*
 *  Errors
 */

void  SCH_Error (message)
char            *message;
{
    printf ("Error: %s\n", message);
}

void  SCH_Fatal (message)
char            *message;
{
    printf ("Fatal ");
    SCH_Error (message);
    SCH_Abandon ();
}

void  SCH_Abandon ()
{
    int    *p;
    int     i;

    /*  Deliberately cause a segmentation violation, to be caught in debugger  */
    /*  We need a better protocol for reporting application errors & up to SSL */

    p = (int *) 0;
    i = *p;

    exit (1);
}



/*
 *  SCH_SaveAscii ()
 */

void    SCH_SaveAscii (root, filename)
Node                   root;
char                  *filename;
{
    FILE    *fp;

    fp = fopen (filename, "w");
    if (fp != NULL)
    {
        SCH_Traverse (root, SCH_SaveNodeAscii, fp);
        fclose (fp);
    }
}


#define LIST_WRAP_COUNT 20

int SCH_SaveNodeAscii (N, fp)
Node                   N;
FILE                  *fp;
{
    int       attr_code;
    void    **attr_ptr;
    Node      N1;
    List      L;
    Item      I;
    int       wrap_count;

    /* assert N != NULL */

    fprintf (fp, "%ld: %s\n", N->node_num, dObjectName[N->kind]);

    for (attr_code = 1; attr_code < dAttributes; attr_code++)
    {
        if (dGetAttributeOffset(N->kind, attr_code) >= 0)
        {
            attr_ptr = SCH_GetAttrPtr (N, attr_code);
            switch (dGetAttributeType(attr_code))
            {
                case SCH_Type_Boolean1:
                    fprintf (fp, "  %s: %s\n", dAttributeName[attr_code],
                                               *(long*)attr_ptr ? "True" : "False");
                    break;

                case SCH_Type_Character1:
                    fprintf (fp, "  %s: '%c'\n", dAttributeName[attr_code],
                                                 *(char*)attr_ptr);
                    break;

                case SCH_Type_Integer4:
                    if (*(long*)attr_ptr != 0)   /* Don't bother to print if 0, for now */
                        fprintf (fp, "  %s: %ld\n", dAttributeName[attr_code],
                                                   *(long*)attr_ptr);
                    break;

                case SCH_Type_StringN:
                    if (*(char**)attr_ptr != NULL)
                        fprintf (fp, "  %s: \"%s\"\n", dAttributeName[attr_code],
                                                       *(char**)attr_ptr);
                    break;

                case SCH_Type_Node:
                    N1 = *(Node *)attr_ptr;
                    if (N1 != NULL)
                        fprintf (fp, "  %s: %ld\n", dAttributeName[attr_code],
                                                   N1->node_num);
                    break;

                case SCH_Type_List:
                    L = *(List *)attr_ptr;
                    if (L != NULL)
                    {
                        fprintf (fp, "  %s: [", dAttributeName[attr_code]);
                        wrap_count = LIST_WRAP_COUNT;
                        FOR_EACH_ITEM (I, L)
                        {
                            N1 = Value(I);
                            if (N1 != NULL)
                            {
                                fprintf (fp, "%ld ", N1->node_num);
                                if (wrap_count-- == 0)
                                {
                                    wrap_count = LIST_WRAP_COUNT;
                                    fprintf (fp, "\n\t\t");
                                }
                            }
                        }
                        fprintf (fp, "]\n");
                    }
                    break;

                default:
                    break;
            }
        }
    }

    fprintf (fp, "  ;\n");
}


/*
 *  SCH_LoadAscii ()
 *
 *  Returns the root node of the database.
 */

Node    SCH_LoadAscii (filename)
char                  *filename;
{
    FILE      *fp;
    Node       root;
    Node       N;
    NodeLoad   nld, nld2;

    root = NULL;
    SCH_node_load_list = NULL;

    fp = fopen (filename, "r");
    if (fp != NULL)
    {

        while (1)
        {
            N = SCH_LoadNodeAscii(fp);
            if (N == NULL)
                break;

            /* The root is the first node in the file */
            if (root == NULL)
                root = N;
        }

        fclose (fp);

        /* Now patch up links */

        for (nld = SCH_node_load_list; nld != NULL; nld = nld->next)
        {
            SCH_LoadPatchNode (nld->node);
        }


        /* Now can free the node load list */

        for (nld = SCH_node_load_list; nld != NULL; nld = nld2)
        {
            nld2 = nld->next;
            free (nld);
        }
        SCH_node_load_list = NULL;
    }

    return (root);
}

Node SCH_LoadNodeAscii (fp)
FILE                   *fp;
{
    int       attr_code;
    void    **attr_ptr;
    Node      N;
    List      L;
    Item      I;
    int       status;
    int       orig_num;
    char      object_name[256];
    char      attr_name[256];
    int       object_type;
    NodeLoad  nld;
    char     *p, *q;
    char     *val;

    if (fscanf (fp, "%d: %s\n", &orig_num, object_name) == EOF)
        return (NULL);

    if (orig_num <= 0)
    {
        sprintf (SCH_errbuf, "Invalid node number '%d' in database file", orig_num);
        SCH_Error (SCH_errbuf);
        return (NULL);
    }

    for (object_type = 0; object_type < dObjects; object_type++)
        if (strcmp(object_name, dObjectName[object_type]) == 0)
            break;

    if (object_type >= dObjects)
    {
        sprintf (SCH_errbuf, "Unknown object type '%s' in database file", object_name);
        SCH_Error (SCH_errbuf);
        return (NULL);
    }

    N = NewNode (object_type);

    /* Track original number while loading.  Will use to patch links later. */
    nld = (NodeLoad) malloc(sizeof(struct node_load_struct));
    nld->node = N;
    nld->orig_num = orig_num;
    nld->next = SCH_node_load_list;
    SCH_node_load_list = nld;

    while (fgets (SCH_db_line, SCH_MAX_DB_LINE, fp) != NULL)
    {
        for (p = SCH_db_line; (*p == ' ') || (*p == '\t'); p++);

        q = attr_name;
        while ((*p != ':') && (*p) && (*p != ' ') && (*p != '\t') && (*p != '\n'))
            *q++ = *p++;
        *q = '\0';

        val = p;
        if (*val == ':') val++;

        if (strcmp(attr_name, ";") == 0)
            break;

        if ((attr_name[0] >= '0') && (attr_name[0] <= '9'))
        {
            SCH_Error ("Missing ';' in database file");
            return (NULL);
        }

        for (attr_code = 1; attr_code < dAttributes; attr_code++)
            if (strcmp(attr_name, dAttributeName[attr_code]) == 0)
                break;

        if (attr_code >= dAttributes)
        {
            sprintf (SCH_errbuf, "Invalid attribute name '%s' in database file", attr_name);
            SCH_Error (SCH_errbuf);
            return (NULL);
        }

        if (dGetAttributeOffset(object_type, attr_code) < 0)
        {
            sprintf (SCH_errbuf, "Invalid attribute '%s' for object '%s' in database file",
                                 attr_name, object_name);
            SCH_Error (SCH_errbuf);
            return (NULL);
        }

        /* skip white space */
        for (; (*val == ' ') || (*val == '\t'); val++);

        /* remove trailing \n */
        if (*val && (val[strlen(val)-1] == '\n'))
            val[strlen(val)-1] = '\0';

        attr_ptr = SCH_GetAttrPtr (N, attr_code);

        switch (dGetAttributeType(attr_code))
        {
            case SCH_Type_Boolean1:
                *(long*)attr_ptr = (strcmp(val, "True") == 0);
                break;

            case SCH_Type_Character1:
                sscanf (val, "'%c'", (char*)attr_ptr);
                break;

            case SCH_Type_Integer4:
                sscanf (val, "%ld", (long*)attr_ptr);
                break;

            case SCH_Type_StringN:
                if (*val == '"') val++;
                if (val[strlen(val)-1] == '"') val[strlen(val)-1] = '\0';
                *(char**)attr_ptr = strdup(val);
                break;

            case SCH_Type_Node:
                orig_num = atoi(val);

                /*
                 * For now, record the original number rather than a pointer.
                 * Later we'll go through and patch all these.
                 * Record the number shifted left | 1, to distinguish it from a
                 * Node pointer (which will always be an address, a multiple of 4 or 8)
                 */
                *(long*)attr_ptr = (orig_num << 1) | 1;
                break;
              
            case SCH_Type_List:
                /* Here we lose the constraint. Should save it in database, or
                 * save the static constraint in the schema
                 */

                L = NewList (1 /*Object*/ );
                *(List *)attr_ptr = L;

                if (*val = '[') val++;
                while (1)
                {
                    for (; (*val == ' ') || (*val == '\t'); val++);
                    if (*val == ']')
                        break;

                    if ((*val == '\n') || (*val == '\0'))   /* wrap around to next line */
                    {
                        if (fgets (SCH_db_line, SCH_MAX_DB_LINE, fp) == NULL)
                        {
                            SCH_Error ("Database file ends in middle of a list");
                            return (NULL);
                        }
                        for (val = SCH_db_line; (*val == ' ') || (*val == '\t'); val++);
                        if (*val == ']')
                            break;
                    }

                    orig_num = atoi(val);
                    for (; (*val >= '0') && (*val <= '9'); val++);

                    /* Based on AddLast() but not a real Node pointer */

                    I = SCH_NewItem (L);
                    I->value = (Node) (intptr_t) ((orig_num << 1) | 1);

                    if (L->first == NULL)
                    {
                        L->first = I;
                        L->last = I;
                    }
                    else
                    {
                        I->prev = L->last;
                        L->last->next = I;
                        L->last = I;
                    }

                    L->length++;
                }
                break;

            default:
                break;
        }
    }

    return (N);
}


/*
 *  The node has been loaded but it may still contains node numbers
 *  instead of node pointers.  Patch those links.
 */

void
SCH_LoadPatchNode ( Node N )
{
    int       attr_code;
    void    **attr_ptr;
    Node      N1;
    List      L;
    Item      I;
    int       orig_num;
    NodeLoad  nld;

    if (N == NULL)
        return;

    for (attr_code = 1; attr_code < dAttributes; attr_code++)
    {
        if (dGetAttributeOffset(N->kind, attr_code) >= 0)
        {
            attr_ptr = SCH_GetAttrPtr (N, attr_code);
            switch (dGetAttributeType(attr_code))
            {
                case SCH_Type_Node:
                    orig_num = *(int *)attr_ptr;
                    if (orig_num & 1)
                    {
                        orig_num = orig_num >> 1;
                        N1 = SCH_LookupNodeLoad (orig_num);
                        *(Node *)attr_ptr = N1;
                        if (N1 == NULL)
                        {
                            sprintf (SCH_errbuf, "Missing node %d in database file", orig_num);
                            SCH_Error (SCH_errbuf);
                        }
                    }
                    break;

                case SCH_Type_List:
                    L = *(List *)attr_ptr;
                    if (L != NULL)
                    {
                        FOR_EACH_ITEM (I, L)
                        {
                            orig_num = (int) (intptr_t) I->value;
                            if (orig_num & 1)
                            {
                                orig_num = orig_num >> 1;
                                N1 = SCH_LookupNodeLoad (orig_num);
                                I->value = N1;
                                if (N1 == NULL)
                                {
                                    sprintf (SCH_errbuf, "Missing node %d in database file", orig_num);
                                    SCH_Error (SCH_errbuf);
                                }
                            }
                        }
                    }
                    break;

                default:
                    break;
            }
        }
    }
}


/*
 *  SCH_Traverse ()
 *
 *  Traverse the primary links in the design, starting at root N.
 *  At each node, calls callback(N, udata).
 */

void    SCH_Traverse (N, callback, udata)
Node                  N;
int                 (*callback)();
void                 *udata;
{
    int       attr_code;
    void    **attr_ptr;
    Node      N1;
    List      L;
    Item      I;

    if (N == NULL)
        return;

    (*callback) (N, udata);

    for (attr_code = 1; attr_code < dAttributes; attr_code++)
    {
        if (dGetAttributeOffset(N->kind, attr_code) >= 0)
        {
            attr_ptr = SCH_GetAttrPtr (N, attr_code);
            switch (dGetAttributeType(attr_code))
            {
                case SCH_Type_Node:
                    N1 = *(Node *)attr_ptr;
                    if (N1 != NULL)
                        if (!(dGetAttributeTags(N->kind, attr_code) & SCH_Tag_Alt))
                            SCH_Traverse (N1, callback, udata);
                    break;

                case SCH_Type_List:
                    L = *(List *)attr_ptr;
                    if (L != NULL)
                    {
                        if (!(dGetAttributeTags(N->kind, attr_code) & SCH_Tag_Alt))
                        {
                            FOR_EACH_ITEM (I, L)
                            {
                                N1 = Value(I);
                                if (N1 != NULL)
                                    SCH_Traverse (N1, callback, udata);
                            }
                        }
                    }
                    break;

                default:
                    break;
            }
        }
    }
}

