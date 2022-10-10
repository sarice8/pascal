
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
*   schema_db.h         Run-time schema database
* 
*   This header should be included in applications that use a schema.
* 
*   HISTORY 
*----------------------------------------------------------------------------
*   05/04/91 | Steve  | First version (node.h)
*   04/20/94 | Steve  | Improved representation, new PI
* 
***************************************************************************** 
*/

#ifndef SCHEMA_DB_H
#define SCHEMA_DB_H

/*
 *  Primitive types
 */

typedef char                     Boolean1;
typedef char                     Character1;
typedef long                     Integer4;    // SARICE 9/2021 needs work
typedef char                    *StringN;

typedef struct SCH_NodeStruct   *Node;
typedef struct SCH_ListStruct   *List;
typedef struct SCH_ItemStruct   *Item;


/*
 *  Primitive values
 */

#define  True      1
#define  False     0

/*
 *  --------------------------------------------------------------------
 *
 *  Internal representation -- private
 */

/*
 *  Primitive type codes
 */

#define SCH_Type_Boolean1       1
#define SCH_Type_Character1     2
#define SCH_Type_Integer4       4
#define SCH_Type_StringN        10
#define SCH_Type_Node           100
#define SCH_Type_List           150

/*
 *  Tag codes
 */

#define SCH_Tag_Alt             4


struct SCH_NodeStruct {
    short         kind;
    unsigned char signature;      /* constant to verify a valid node */
    char          pad1;           /* unused */
    long          node_num;
    long          attribute[1];   /* replaced by node attributes */
};

struct SCH_ListStruct {
    short         kind;           /* constraint type for members of list */
    unsigned char signature;      /* constant to verify a valid list */
    char          pad1;           /* unused */
    Item          first;
    Item          last;
    int           length;
};

struct SCH_ItemStruct {
    short         pad0;
    unsigned char signature;
    char          pad1;
    List          list;           /* owning list */
    Item          next;
    Item          prev;
    Node          value;
};

#define SCH_NODE_SIGNATURE 0xAB
#define SCH_LIST_SIGNATURE 0xCD
#define SCH_ITEM_SIGNATURE 0xEF

#define SCH_VERIFY_NODE(N,Ret) { \
    if ((N)==NULL) \
       { SCH_Error("Schema: Null Node"); return Ret; } \
    else if ((N)->signature != SCH_NODE_SIGNATURE) \
       { SCH_Error("Schema: Invalid Node"); return Ret; } \
}

#define SCH_VERIFY_LIST(L,Ret) { \
    if ((L)==NULL) \
       { SCH_Error("Schema: Null List"); return Ret; } \
    else if ((L)->signature != SCH_LIST_SIGNATURE) \
       { SCH_Error("Schema: Invalid List"); return Ret; } \
}

#define SCH_VERIFY_ITEM(I,Ret) { \
    if ((I)==NULL) \
       { SCH_Error("Schema: Null Item"); return Ret; } \
    else if ((I)->signature != SCH_ITEM_SIGNATURE) \
       { SCH_Error("Schema: Invalid Item"); return Ret; } \
}


/*
 *  End of private internal representation
 *
 *  --------------------------------------------------------------------
 */


/*
 *
 *  SCHEMA Public Database Functions
 *
 */

void  SCH_Init ();
void  SCH_Term ();

/*
 *  Nodes
 */

Node     NewNode  ( int object_type );
int      Kind     ( Node N );
Boolean1 IsA      ( int object_type_1, int object_type_2 );
Boolean1 IsNull   ( Node N );
void     SetAttr  ( Node node, int attr_code, void* value );
void     SetValue ( Node node, int attr_code, long value );
void*    GetAttr  ( Node node, int attr_code );
long     GetValue ( Node node, int attr_code );
void     FreeNode ( Node N, Boolean1 recurse );

/*
 *  Lists
 */

List     NewList  ( int object_type );
Item     AddFirst ( List L, Node N );
Item     AddLast  ( List L, Node N );
Item     AddNext  ( Item I, Node N );
Item     AddPrev  ( Item I, Node N );
Node     RemoveFirst ( List L );
Node     RemoveLast  ( List L );
Node     RemoveItem  ( Item I );
Item     FirstItem ( List L );
Item     LastItem ( List L );
Item     NextItem ( Item I );
Item     PrevItem ( Item I );
Node     Value    ( Item I );
Item     FindItem ( List L, Node N );
Item     NullItem ();
Boolean1 IsEmpty  ( List L );
List     ListOf   ( Item I );
int      ListLength ( List L );
void     FreeList ( List L, Boolean1 recurse );

#define  FOR_EACH_ITEM(I,L)  for (I=FirstItem(L); I != NULL; I = NextItem(I))

/*
 *  Database I/O
 */

void     SCH_SaveAscii ( Node root, char* filename );
Node     SCH_LoadAscii ( char* filename );

/*
 *  Debugger
 */

void     DumpNodeShort ( Node N );
void     DumpNodeLong  ( Node N );

/*
 *  Internal use
 */

void** SCH_GetAttrPtr ( Node node, int attr_code );
Item   SCH_NewItem ( List owner );
void   SCH_indent_printf ( int indent, char* message, ... ) __attribute__ ((format (printf, 2, 3 )));
Node   SCH_LookupNode ( int node_num );
char*  SCH_GetTypeName ( int type );

void   SCH_Error ( char* message );
void   SCH_Fatal ( char* message );
void   SCH_Abandon ();


#endif /* SCHEMA_DB_H */
