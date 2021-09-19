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


/*  Operations defined on nodes  (implemented in vnode.c)  */

NODE_PTR  nodeNew        (/* node_type */);
          nodeLink       (/* fromNP, fromAttr, toNP */);
          nodeSetValue   (/* fromNP, fromAttr, value */);
          nodeAppend     (/* fromNP, fromAttr, toNP */);
NODE_PTR  nodeAppendList (/* fromNP, toNP */);
NODE_PTR  nodeGet        (/* fromNP, fromAttr */);
long      nodeGetValue   (/* fromNP, fromAttr */);
NODE_PTR  nodeFirst      (/* fromNP */);
NODE_PTR  nodeNext       (/* fromNP */);
int       nodeType       (/* fromNP */);
NODE_PTR  nodeFindValue  (/* fromNP, fromAttr, findAttr, findValue */);
int       nodeCompare    (/* NP1, NP2 */);
          nodeSaveTree   (/* fromNP */);

void     *nodeGetAttrPtr (/* fromNP, fromAttr */);
char     *nodeTypeName   (/* node_type */);
int       nodeIsA        (/* fromNP, node_type */);
          nodeSaveTreeToFile (/* fromNP */);
          nodeDumpTree   (/* fromNP */);
          nodeDumpNodeShort (/* fromNP */);
          nodeDumpTreeNum (/* node_num */);

NODE_PTR  nodeNumToPtr   (/* node_num */);
