/**/ 
/* static char sccsid[] = "@(#)node.h	1.5 6/11/93 14:54:06 /files/home/sim/sarice/compilers/schema/SCCS/s.node.h"; */
/**/

/*
*****************************************************************************
*
*   Verilog Analyzer
*
*   by Steve Rice
*
*   May 04, 1991
*
*****************************************************************************
*
*   vnode.h             Parse tree structures.
*
*   This file contains the node and list structures used for the parse tree.
*
*   A node of the tree can either be a "NODE" or a "LIST".  A LIST is a
*   header that points to a linked list.  Any NODE can be in a list,
*   but only one list.
*
*   HISTORY
*----------------------------------------------------------------------------
*   05/04/91 | Steve  | First version
*
*****************************************************************************
*/

typedef struct node_struct NODE_STRUCT, *NODE_PTR;

struct node_struct {

    /*  common for NODE or LIST  */

    char       list;          /* 0 = NODE, 1 = LIST */
    char       col;           /* column of source text when node created */
    short      line;          /* line of source text when node created */
    short      node_type;     /* type of node */
    short      pad1;          /* unused */
    long       node_num;      /* used for saving to file */
    NODE_PTR   next;          /* next node in LIST */

    /*  for NODE only  */

    long       attribute[1];  /* attributes follow -- each is a "long" for now */
};


/*  If the node is actually a LIST, it can be accessed as follows:  */
/*  (Note, the node's "next" field is also known as "head" here)    */

typedef struct list_struct LIST_STRUCT, *LIST_PTR;

struct list_struct {

    /*  common for NODE or LIST  */

    char       list;          /* 0 = NODE, 1 = LIST */
    char       col;           /* column of source text when node created */
    short      line;          /* line of source text when node created */
    short      node_type;     /* type of node to go in LIST */
    short      pad1;          /* unused */
    long       node_num;      /* used for saving to file */
    NODE_PTR   head;

    /*  for LIST only  */

    NODE_PTR   tail;
};


/*  This is just a duplicate of the common info of a node  */

typedef struct node_header_struct NODE_HEADER_STRUCT, *NODE_HEADER_PTR;

struct node_header_struct {

    /*  common for NODE or LIST  */

    char       list;          /* 0 = NODE, 1 = LIST */
    char       col;           /* column of source text when node created */
    short      line;          /* line of source text when node created */
    short      node_type;     /* type of node */
    short      pad1;          /* unused */
    long       node_num;      /* used for saving to file */
    NODE_PTR   next;          /* used for linking nodes into a LIST */

};


void      nodeInit();

/*  Operations defined on nodes  (implemented in vnode.c)  */

NODE_PTR  nodeNew        ( short node_type );
void      nodeLink       ( NODE_PTR fromNP, short fromAttr, NODE_PTR toNP );
void      nodeSetValue   ( NODE_PTR fromNP, short fromAttr, long value );
void      nodeAppend     ( NODE_PTR fromNP, short fromAttr, NODE_PTR toNP );
NODE_PTR  nodeAppendList ( NODE_PTR fromNP, NODE_PTR toNP );
NODE_PTR  nodeGet        ( NODE_PTR fromNP, short fromAttr );
long      nodeGetValue   ( NODE_PTR fromNP, short fromAttr );
NODE_PTR  nodeFirst      ( NODE_PTR fromNP );
NODE_PTR  nodeNext       ( NODE_PTR fromNP );
int       nodeType       ( NODE_PTR fromNP );
NODE_PTR  nodeFindValue  ( NODE_PTR fromNP, short fromAttr, short findAttr, long findValue );
int       nodeCompare    ( NODE_PTR NP1, NODE_PTR NP2 );
void      nodeSaveTree   ( FILE* t_out, NODE_PTR fromNP );

void*     nodeGetAttrPtr ( NODE_PTR fromNP, short fromAttr );
char*     nodeTypeName   ( short node_type );
int       nodeIsA        ( NODE_PTR fromNP, short node_type );
void      nodeSaveTreeToFile ( FILE* t_out, NODE_PTR fromNP );
void      nodeDumpTree   ( NODE_PTR fromNP, int indent );
void      nodeDumpNodeShort ( NODE_PTR fromNP );
void      nodeDumpTreeNum ( long node_num );

int       nodeNum        ( NODE_PTR fromNP );
NODE_PTR  nodeNumToPtr   ( long node_num );


/*  The following operations are defined as macros for speed.
    However, anyone using them needs to know the NODE interal definition.
    I might decide to limit their use to the implementation of node.c  */

#define nodeNext_M(NP)           ((NP)->next)
#define nodeType_M(NP)           ((NP)->node_type)
#define nodeGetValue_M(NP,attr)  (*(long *)nodeGetAttrPtr((NP),(attr)))

/* Note, this does NO error checking! */
#define nodeGetAttrPtr_M(NP,attr) (((char *)&((NP)->attribute[0])) + dGetAttributeOffset(nodeType_M(NP),(attr)))

/* Note, this does NO error checking! */
#define nodeGetValue_M_NoErrorChecking(NP,attr) (*(long *)nodeGetAttrPtr_M((NP),(attr)))

/* Note, this does NO error checking! */
NODE_PTR nodeFindValue_NoErrorChecking ( NODE_PTR fromNP, short fromAttr, short findAttr, long findValue );

