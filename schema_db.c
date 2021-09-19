
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

#ifdef AMIGA
#include <dos.h>
#endif /* AMIGA */


#include "schema_db.h"


/*
 *  Schema table access primitives, defined in <schema>_schema.c
 */

extern short    dGetAttributeOffset (/* class, attribute */);
extern short    dGetAttributeTags   (/* class, attribute */);
extern short    dGetAttributeType   (/* attribute */);

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

/* For debugging */

typedef struct node_lookup_struct  *NodeLookup;

struct node_lookup_struct {
    Node         node;
    NodeLookup   next;
};

static NodeLookup  SCH_node_lookup_list;


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

    size = sizeof(struct SCH_NodeStruct) - 4 + dObjectSize[object_type];
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

void   SetAttr (attr_code, N, value)
int             attr_code;
Node            N;
void           *value;
{
    void      **attr_ptr;

    if ((dAttributeType[attr_code] == SCH_Type_StringN) &&
        (value != NULL))
    {
        value = strdup (value);
    }

    attr_ptr = SCH_GetAttrPtr (N, attr_code);
    *attr_ptr = value;
}

void  *GetAttr (attr_code, N)
int             attr_code;
Node            N;
{
    void  **attr_ptr;
    void   *value;

    attr_ptr = SCH_GetAttrPtr (N, attr_code);
    value    = *attr_ptr;

    return (value);
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

    L = I_from->list;

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

    L = I_from->list;

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

Item   FirstItem (L)
List              L;
{
    Item     I;

    I = L->first;
    return (I);
}

Item   LastItem (L)
List             L;
{
    Item     I;

    I = L->last;
    return (I);
}

Item   NextItem (I_from)
Item             I_from;
{
    Item     I;

    I = I_from->next;
    return (I);
}

Item   PrevItem (I_from)
Item             I_from;
{
    Item     I;

    I = I_from->prev;
    return (I);
}

Node   Value (I)
Item          I;
{
    return (I->value);
}

Item   NullItem ()
{
    return (NULL);
}

Boolean1   IsEmpty (L)
List                L;
{
    return ((L == NULL) || (L->length == 0));
}
    
List   ListOf (I)
Item           I;
{
    return (I->list);
}

int    ListLength (L)
List               L;
{
    if (L == NULL)
        return 0;

    return (L->length);
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
        printf ("%d: %s\n", N->node_num, dObjectName[N->kind]);
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

    SCH_indent_printf (indent, "%s\t\t[%d]\n", dObjectName[N->kind], N->node_num);

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
                    printf ("%d\n", *(long*)attr_ptr);
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
                        printf ("[Alt %d]\n", (*(Node *)attr_ptr)->node_num);
                    else
                        printf ("[%d]\n", (*(Node *)attr_ptr)->node_num);
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
                            printf (" %d", Value(I)->node_num);
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
    char      errbuf [256];

    if ((N->kind >= dObjects) || (N->kind <  0))
        SCH_Fatal ("Illegal object type observed in node");

    if ((attr_code >= dAttributes) || (attr_code < 0))
        SCH_Fatal ("Illegal attribute type");

    offset = dGetAttributeOffset (N->kind, attr_code);

    if (offset < 0)
    {
        sprintf (errbuf, "Object '%s' does not have attribute '%s'",
                 dObjectName [N->kind], dAttributeName [attr_code]);
        SCH_Fatal (errbuf);
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

void SCH_indent_printf (indent, message, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)
int                     indent;
char                   *message;
int                     p0, p1, p2, p3, p4, p5, p6, p7, p8, p9;
{
    int        i;

    for (i = 0; i < indent; i++)
        putchar (' ');
    printf (message, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
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

