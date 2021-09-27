/**/ 
static char sccsid[] = "%W% %G% %U% %P%";
/**/

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
*   node.c              Run-time schema node functions
*
*   HISTORY
*----------------------------------------------------------------------------
*   05/11/93 | Steve  | Moved from vmain.c into a generic package.
*   06/09/93 | Steve  | Moved from verilog analyzer into schema.
*
*****************************************************************************
*/

/*  TO CLEAN UP:
 *
 *  -- Makes direct calls to SSL/application functions:
 *
 *     ssl_error (msg)
 *     ssl_fatal (msg)
 *     char *ssl_get_id_string (id)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef AMIGA
#include <dos.h>
#endif // AMIGA


#include "node.h"
#include "ssl_rt.h"


/* Schema definitions, defined in *_schema.c */
/* NOTE, should read from *_schema.h */

/* -------------------------------------------------------- */

short dGetAttributeOffset ( short theClass, short attribute );
short dGetAttributeType   ( short theClass, short attribute );
short dGetAttributeTags   ( short theClass, short attribute );
int   dGetClassIsA        ( short theClass, short isaclass );

extern short dObjectSize [];

extern int   dObjects;
extern char *dObjectName [];

extern int   dAttributes;
extern char *dAttributeName [];

/* -------------------------------------------------------- */

void node_assert_fun (int expr, int line_num);
void nodeSaveNodeToFile ( FILE* t_out, void* data_ptr, int data_size );
void indent_printf ( int indent, char* message, ... ) __attribute__ ((format (printf, 2, 3 )));


#define node_assert(expr)  node_assert_fun((expr),__LINE__)


static long     NodeNextNum;

/* For debugging, to find a node given a number */
struct nodeLookupStruct {
    NODE_PTR                 node;
    struct nodeLookupStruct *next;
};
static struct nodeLookupStruct *NodeLookupList;


/* Initialize node package */
void
nodeInit ()
{
    NodeNextNum = 1;

    /* To support debugging */
    NodeLookupList = NULL;
}


NODE_PTR
nodeNew( short node_type )
{
    short      size;
    NODE_PTR   NP;
    struct nodeLookupStruct *nl;

    size = sizeof (NODE_HEADER_STRUCT) + dObjectSize[node_type];
    NP = (NODE_PTR) malloc(size);
    node_assert (NP != NULL);

    bzero (NP, size);
    NP->node_num  = NodeNextNum++;
    NP->node_type = node_type;
#if 0
    NP->line      = t_token.lineNumber;
    NP->col       = t_token.colNumber;
#endif /* 0 */

    /* Just required for debugging, to allow lookup of node
       given node number: */
    nl = (struct nodeLookupStruct *) malloc(sizeof(struct nodeLookupStruct));
    nl->node = NP;
    nl->next = NodeLookupList;
    NodeLookupList = nl;

    return (NP);
}


void
nodeLink( NODE_PTR fromNP, short fromAttr, NODE_PTR toNP )
{
    void      *attrP;

    attrP = nodeGetAttrPtr (fromNP, fromAttr);
    *(NODE_PTR *)attrP = toNP;
}


void
nodeSetValue ( NODE_PTR fromNP, short fromAttr, long value )
{
    void      *attrP;

    attrP = nodeGetAttrPtr (fromNP, fromAttr);
    *(long *)attrP = value;
}


/*  Append toNP to LIST attribute of fromNP  */

void
nodeAppend ( NODE_PTR fromNP, short fromAttr, NODE_PTR toNP )
{
    void      *attrP;
    NODE_PTR   NP;
    LIST_PTR   LP;

    attrP = nodeGetAttrPtr (fromNP, fromAttr);
    NP = *(NODE_PTR *) attrP;

    /* Note, NP may be NULL, meaning list will consist of toNP */
    *(NODE_PTR *)attrP = nodeAppendList (NP, toNP);
}


/*  Append toNP to same list as fromNP.
    Returns new head of list  */

NODE_PTR
nodeAppendList ( NODE_PTR fromNP, NODE_PTR toNP )
{
    LIST_PTR   LP;

    if (fromNP == NULL)
        return (toNP);    /* can append toNP to NULL, resulting in list toNP */

    if (fromNP->list)
    {
        /* Already have a list */
        ((LIST_PTR) fromNP)->tail->next = toNP;
        ((LIST_PTR) fromNP)->tail = toNP;
        return (fromNP);  /* No change in list head */
    }
    else
    {
        /* Already have a single node */
        LP = (LIST_PTR) malloc(sizeof(LIST_STRUCT));
        node_assert (LP != NULL);
        bzero (LP, sizeof(LIST_STRUCT));
        LP->node_num = NodeNextNum++;
        LP->list = 1;
        LP->node_type = fromNP->node_type;  /* should match */
        LP->head = fromNP;
        fromNP->next = toNP;
        LP->tail = toNP;
        return ((NODE_PTR) LP);   /* New list head */
    }
}


/*  Get a NODE attribute.  If points to a LIST, returns first node of list.  */

NODE_PTR
nodeGet ( NODE_PTR fromNP, short fromAttr )
{
    void      *attrP;
    NODE_PTR   NP;

    attrP = nodeGetAttrPtr (fromNP, fromAttr);
    NP = *(NODE_PTR *)attrP;
    if ((NP == NULL) || (!NP->list))
        return (NP);
    else
        return (((LIST_PTR)NP)->head);
}


/*  Get an INTEGER attribute.  */
long
nodeGetValue ( NODE_PTR fromNP, short fromAttr )
{
    void      *attrP;
    NODE_PTR   NP;

    attrP = nodeGetAttrPtr (fromNP, fromAttr);
    return (*(long *)attrP);
}


/*  Return the first node in a LIST.
    Usually not necessary since this is automatically done
    by nodeGet when retrieving a List attribute.
    However, if the application maintains its own list,
    then this is required rathern than accessing the
    list head directly (it may be a list head structure
    rather than a node). */

NODE_PTR
nodeFirst ( NODE_PTR fromNP )
{
    if ((fromNP == NULL) || (!fromNP->list))
        return (fromNP);
    else
        return (((LIST_PTR)fromNP)->head);
}


/*  Return the next node in a LIST  */
NODE_PTR
nodeNext ( NODE_PTR fromNP )
{
    return (fromNP->next);
}


/*  Return the node's type  */
int
nodeType ( NODE_PTR fromNP )
{
    return (fromNP->node_type);
}


/*  Compare two node trees for equivalent contents.
    If nodes contain Alt links, they must point to
    the same node.  (i.e. comparison won't traverse
    Alt links, and will fail unless alt links point
    to the same node). */

int
nodeCompare ( NODE_PTR NP1, NODE_PTR NP2 )
{
    short     attr;
    short     node_type;
    char     *Attr1;
    char     *Attr2;

    if (NP1 == NP2)
        return (1);

    if ((NP1 == NULL) || (NP2 == NULL))
        return (0);

    /* TO DO: proper handling of lists. */
    if (NP1->list || NP2->list)
        return (0);  /* fails for now */

    if (NP1->node_type != NP2->node_type)
        return (0);

    node_type = NP1->node_type;

    /* Need a better way to determine attributes of node */

    for (attr = 1; attr < dAttributes; attr++)
    {
        if (dGetAttributeOffset(node_type, attr) >= 0)
        {
            Attr1 = nodeGetAttrPtr (NP1, attr);
            Attr2 = nodeGetAttrPtr (NP2, attr);

            /* Node_type same for both nodes, so attr type same as well */
            switch (dGetAttributeType(node_type, attr))
            {
                case 4:                   /* Integer4 */
                    if (*(long*)Attr1 != *(long*)Attr2)
                        return (0);
                    break;
                case 100:                 /* Node */
                case 150:                 /* List */
                    if (dGetAttributeTags(node_type, attr) & 4)  /* Alt */
                    {
                        if (*(NODE_PTR *)Attr1 != *(NODE_PTR *)Attr2)
                            return (0);
                    }
                    else
                    {
                        if (nodeCompare (*(NODE_PTR *)Attr1,
                                         *(NODE_PTR *)Attr2) == 0)
	                    return (0);
                    }
                    break;
                default:
                    return (0);   /* Any other types: TO DO */
            }
        }
    }
    return (1);   /* Match */
}


/* Save tree to file  */
void
nodeSaveTree ( FILE* t_out, NODE_PTR fromNP )
{
    nodeSaveTreeToFile (t_out, fromNP);
}


/* Search list at fromNP.fromAttr, searching nodes for findAttr with
   value findValue.  Return NULL if not found. */
/* Implemented using fast node marcros, for speed.  But these macros
   still perform error checking.  */

NODE_PTR
nodeFindValue ( NODE_PTR fromNP, short fromAttr, short findAttr, long findValue )
{
    NODE_PTR  NP;

    for (NP = nodeGet (fromNP, fromAttr);
         NP != NULL;
         NP = nodeNext_M(NP))
    {
        if (nodeGetValue_M (NP, findAttr) == findValue)
            return (NP);
    }
    return (NULL);
}


/*  Does no error checking, all nodes in list must be valid for search  */

NODE_PTR
nodeFindValue_NoErrorChecking ( NODE_PTR fromNP, short fromAttr, short findAttr, long findValue )
{
    NODE_PTR  NP;

    for (NP = nodeGet (fromNP, fromAttr);
         NP != NULL;
         NP = nodeNext_M(NP))
    {
        if (nodeGetValue_M_NoErrorChecking (NP, findAttr) == findValue)
            return (NP);
    }
    return (NULL);
}


/*  Given a node and an attribute code, return a pointer to
    the attribute field within the node.                    */

void*
nodeGetAttrPtr ( NODE_PTR dNode, short attr )
{
    short     offset;
    char      errbuf [256];
    LIST_PTR  dList;

    if (dNode->list)
    {
        dList = (LIST_PTR) dNode;
        sprintf (errbuf, "Attempt to access attribute '%s' of a LIST of '%s'",
                 dAttributeName [attr], dObjectName [dList->head->node_type]);
        ssl_fatal (errbuf);
    }

    if (dNode->node_type >= dObjects || dNode->node_type < 0)
        ssl_fatal ("Illegal node type observed in node");

    if (attr >= dAttributes || attr < 0)
        ssl_fatal ("Illegal attribute used in node access");

    offset = dGetAttributeOffset(dNode->node_type, attr);
    if (offset < 0)
    {
        sprintf (errbuf, "Attempt to access attribute '%s' in object '%s'",
                 dAttributeName [attr], dObjectName [dNode->node_type]);
        ssl_fatal (errbuf);
    }
    return (((char *) &(dNode->attribute[0])) + offset);
}


int        nodeIsA (NP, node_type)
NODE_PTR            NP;
short               node_type;
{
    if (NP == NULL)
        return (0);
    else
        return (dGetClassIsA(NP->node_type, node_type));
}

char      *nodeTypeName (node_type)
short                    node_type;
{
    return (dObjectName[node_type]);
}


#define nodeConvertToNum(ptr) (NODE_PTR) ((ptr) ? (ptr)->node_num : 0)


void
nodeSaveTreeToFile ( FILE* t_out, NODE_PTR root )
{
    short       attr;
    LIST_PTR    list;
    NODE_PTR    node_ptr;
    NODE_PTR   *attr_ptr;
    static char error_buf[256];

    if (root == NULL)
        return;

    if (root->list)
    {
        list = (LIST_PTR) root;
        root = list->head;     /* Advance before destroying pointer */
        list->head = nodeConvertToNum (list->head);
        list->tail = nodeConvertToNum (list->tail);
        nodeSaveNodeToFile (t_out, list, sizeof(LIST_STRUCT));
    }

    while (root != NULL)
    {
        for (attr = 1; attr < dAttributes; attr++)
        {
            if (dGetAttributeOffset(root->node_type, attr) >= 0)
            {

                /*  Define names for these types */
                switch (dGetAttributeType (root->node_type, attr))
                {
                    case 4:                   /* Integer4 */
                            break;
                    case 100:                 /* Node */
                    case 150:                 /* List */
                            attr_ptr = (NODE_PTR *) nodeGetAttrPtr (root, attr);
                            node_ptr = *attr_ptr;
#if 0      /*  This is normal way  */
                            *attr_ptr = nodeConvertToNum (*attr_ptr);
                            if ((dGetAttributeTags(root->node_type, attr) & 4) == 0)  /* not Alt */
                                nodeSaveTreeToFile (t_out, node_ptr);
#else      /*  For Testing  */
                            if (node_ptr != NULL)
                            {
                                if (((unsigned long) node_ptr) < 5000)
                                {
    sprintf (error_buf, "Attempt to follow '%s' of '%s' after converted to int",
             dAttributeName [attr], dObjectName [root->node_type]);
    ssl_error (error_buf);
                                }
                                else
                                {
                                    *(long *)attr_ptr = node_ptr->node_num;
                                    if ((dGetAttributeTags(root->node_type, attr) & 4) == 0)
                                        nodeSaveTreeToFile (t_out, node_ptr); /*not Alt*/
                                }
                            }
#endif  
                            break;
                    default:
                            break;
                }
            }
        }
        node_ptr = root;
        root = root->next;

        node_ptr->next = nodeConvertToNum (node_ptr->next);
        nodeSaveNodeToFile (t_out, node_ptr, sizeof(NODE_HEADER_STRUCT) +
                             dObjectSize [node_ptr->node_type]);
    }
}


void
nodeSaveNodeToFile ( FILE* t_out, void* data_ptr, int data_size )
{
    char* char_ptr = (char*) data_ptr;

    int    i;
    for (i = 0; i < data_size; i++)
        fputc (char_ptr[i], t_out);
}


/*  Dump a tree to the screen, given the root node  */

void
nodeDumpTree ( NODE_PTR root, int indent )
{
    short    attr;
    char    *Attr;

    if (root == NULL)
    {
        return;
    }

    if (root->list)
    {
        indent_printf (indent, "LIST\n");

        /* Print all the elements of the list */
        for (root = ((LIST_PTR)root)->head; root != NULL; root = root->next)
            nodeDumpTree (root, indent);
    }
    else
    {
        indent_printf (indent, "%s\t\t[%ld]\n", dObjectName [root->node_type],
                                root->node_num);

        indent += 4;
        for (attr = 1; attr < dAttributes; attr++)
        {
            if (dGetAttributeOffset(root->node_type, attr) >= 0)
            {
                Attr = nodeGetAttrPtr (root, attr);

                /*  NOTE:  Get "schema" to define appropriate names for these types */
                switch (dGetAttributeType(root->node_type, attr))
                {
                    case 4:    /* Integer4 */
                               indent_printf (indent, "%s:", dAttributeName [attr]);
                               /* Kludge special case: print qIdent names. Later should be a String attr. */
                               if (strcmp(dAttributeName[attr], "qIdent") == 0)
                                   printf ("\t\t%ld   \"%s\"\n", *(long *)Attr, ssl_get_id_string(*(long*)Attr));
                               else
                                   printf ("\t\t%ld\n", *(long *)Attr);
                               break;

                    case 100:  /* Node */
                    case 150:  /* List */

#if 0
                               /* Don't bother to list NULL pointer attributes */
                               if (*(NODE_PTR *)Attr == NULL)
                                   break;
#endif /* 0 */

                               indent_printf (indent, "%s:", dAttributeName [attr]);
                               if (dGetAttributeTags(root->node_type, attr) & 4)  /* Alt */
                               {
                                   if (*(NODE_PTR *)Attr != NULL)
                                       printf ("\t\t[Alt %ld]\n",
                                             (*(NODE_PTR *)Attr)->node_num);
                                   else
                                       printf ("\n");
                               }
                               else
                               {
                                   printf ("\n");
	                           indent += 4;
                                   nodeDumpTree (*(NODE_PTR *)Attr, indent);
	                           indent -= 4;
                               }
                               break;

                    default:
                               indent_printf (indent, "%s:", dAttributeName [attr]);
	                       printf ("\tInvalid Attribute Class\n");
                               break;
                }
            }
        }
        indent -= 4;
    }
}


void
indent_printf ( int indent, char* message, ... )
{
    va_list    arglist;
    int        i;

    for (i = 0; i < indent; i++)
        putchar (' ');

    va_start( arglist, message );
    vprintf( message, arglist );
    va_end( arglist );
}


void
nodeDumpTreeNum ( long node_number )
{
    NODE_PTR    NP;

    NP = nodeNumToPtr (node_number);
    nodeDumpTree (NP, 0);
}


void
nodeDumpNodeShort ( NODE_PTR NP )
{
    short    attr;
    char    *Attr;

    if (NP == NULL)
    {
        printf ("\tNull\n");
    }
    else if (NP->list)
    {
        printf ("\tLIST ");    /* For a list, just print node numbers */

        NP = ((LIST_PTR)NP)->head;
        while (NP != NULL)
        {
            printf ("%ld ", NP->node_num);
            NP = NP->next;
        }
        printf ("\n");
    }
    else
    {
        printf ("\t%ld: %s\n", NP->node_num, dObjectName[NP->node_type]);
    }
}


int
nodeNum ( NODE_PTR NP )
{
    if (NP == NULL)
        return (0);
    else
        return (NP->node_num);
}


/* For debugging.  Get node ptr given node number.
   Not so efficient. */

NODE_PTR
nodeNumToPtr ( long node_number )
{
    struct nodeLookupStruct *nl;

    for (nl = NodeLookupList; nl != NULL; nl = nl->next)
        if (nl->node->node_num == node_number)
            return (nl->node);

    return (NULL);
}


void
node_assert_fun (int expr, int line_num)
{
    char   buffer[100];

    if (!expr)
    {
        sprintf (buffer, "Assertion error in node functions, line %d", line_num);
        ssl_fatal (buffer);
    }
}


