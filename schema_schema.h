/* Generated automatically by schema */

#include <stdint.h>
#include "schema_db.h"

#ifndef Object
typedef enum {
    nINVALID,
    Object,
    nSchema,
    nClass,
    nAttr,
    nConstraint,
    nAttrSym,
} ObjectType;

typedef enum {
    T_qINVALID,
    T_qClassTree,
    T_qAttrSyms,
    T_qAllClasses,
    T_qIdent,
    T_qText,
    T_qCode,
    T_qAttrs,
    T_qDerived,
    T_qAttrSym,
    T_qConstraint,
    T_qTags,
    T_qType,
} AttrCode;
#endif /* Object */

#define qINVALID(N)		(Integer4)	GetAttr(T_qINVALID, N)
#define qClassTree(N)		(List)	GetAttr(T_qClassTree, N)
#define qAttrSyms(N)		(List)	GetAttr(T_qAttrSyms, N)
#define qAllClasses(N)		(List)	GetAttr(T_qAllClasses, N)
#define qIdent(N)		(Integer4)	(intptr_t) GetAttr(T_qIdent, N)
#define qText(N)		(StringN)	GetAttr(T_qText, N)
#define qCode(N)		(Integer4)	(intptr_t) GetAttr(T_qCode, N)
#define qAttrs(N)		(List)	GetAttr(T_qAttrs, N)
#define qDerived(N)		(List)	GetAttr(T_qDerived, N)
#define qAttrSym(N)		(Node)	GetAttr(T_qAttrSym, N)
#define qConstraint(N)		(Node)	GetAttr(T_qConstraint, N)
#define qTags(N)		(Integer4)	(intptr_t) GetAttr(T_qTags, N)
#define qType(N)		(Integer4)	(intptr_t) GetAttr(T_qType, N)

#define SetqINVALID(N,V)		SetAttr(T_qINVALID, N, V)
#define SetqClassTree(N,V)		SetAttr(T_qClassTree, N, V)
#define SetqAttrSyms(N,V)		SetAttr(T_qAttrSyms, N, V)
#define SetqAllClasses(N,V)		SetAttr(T_qAllClasses, N, V)
#define SetqIdent(N,V)		SetAttr(T_qIdent, N, (void*)(intptr_t)(V))
#define SetqText(N,V)		SetAttr(T_qText, N, V)
#define SetqCode(N,V)		SetAttr(T_qCode, N, (void*)(intptr_t)(V))
#define SetqAttrs(N,V)		SetAttr(T_qAttrs, N, V)
#define SetqDerived(N,V)		SetAttr(T_qDerived, N, V)
#define SetqAttrSym(N,V)		SetAttr(T_qAttrSym, N, V)
#define SetqConstraint(N,V)		SetAttr(T_qConstraint, N, V)
#define SetqTags(N,V)		SetAttr(T_qTags, N, (void*)(intptr_t)(V))
#define SetqType(N,V)		SetAttr(T_qType, N, (void*)(intptr_t)(V))

#define NewnINVALID()		NewNode(nINVALID)
#define NewObject()		NewNode(Object)
#define NewnSchema()		NewNode(nSchema)
#define NewnClass()		NewNode(nClass)
#define NewnAttr()		NewNode(nAttr)
#define NewnConstraint()		NewNode(nConstraint)
#define NewnAttrSym()		NewNode(nAttrSym)

