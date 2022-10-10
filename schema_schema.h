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
    qINVALID,
    qClassTree,
    qAttrSyms,
    qAllClasses,
    qIdent,
    qText,
    qCode,
    qAttrs,
    qDerived,
    qAttrSym,
    qConstraint,
    qTags,
    qType,
} AttrCode;
#endif /* Object */

#define GetqINVALID(N)		(Integer4)	GetAttr(N, qINVALID)
#define GetqClassTree(N)		(List)	GetAttr(N, qClassTree)
#define GetqAttrSyms(N)		(List)	GetAttr(N, qAttrSyms)
#define GetqAllClasses(N)		(List)	GetAttr(N, qAllClasses)
#define GetqIdent(N)		(Integer4)	(intptr_t) GetAttr(N, qIdent)
#define GetqText(N)		(StringN)	GetAttr(N, qText)
#define GetqCode(N)		(Integer4)	(intptr_t) GetAttr(N, qCode)
#define GetqAttrs(N)		(List)	GetAttr(N, qAttrs)
#define GetqDerived(N)		(List)	GetAttr(N, qDerived)
#define GetqAttrSym(N)		(Node)	GetAttr(N, qAttrSym)
#define GetqConstraint(N)		(Node)	GetAttr(N, qConstraint)
#define GetqTags(N)		(Integer4)	(intptr_t) GetAttr(N, qTags)
#define GetqType(N)		(Integer4)	(intptr_t) GetAttr(N, qType)

#define SetqINVALID(N,V)		SetAttr(N, qINVALID, V)
#define SetqClassTree(N,V)		SetAttr(N, qClassTree, V)
#define SetqAttrSyms(N,V)		SetAttr(N, qAttrSyms, V)
#define SetqAllClasses(N,V)		SetAttr(N, qAllClasses, V)
#define SetqIdent(N,V)		SetAttr(N, qIdent, (void*)(intptr_t)(V))
#define SetqText(N,V)		SetAttr(N, qText, V)
#define SetqCode(N,V)		SetAttr(N, qCode, (void*)(intptr_t)(V))
#define SetqAttrs(N,V)		SetAttr(N, qAttrs, V)
#define SetqDerived(N,V)		SetAttr(N, qDerived, V)
#define SetqAttrSym(N,V)		SetAttr(N, qAttrSym, V)
#define SetqConstraint(N,V)		SetAttr(N, qConstraint, V)
#define SetqTags(N,V)		SetAttr(N, qTags, (void*)(intptr_t)(V))
#define SetqType(N,V)		SetAttr(N, qType, (void*)(intptr_t)(V))

#define NewnINVALID()		NewNode(nINVALID)
#define NewObject()		NewNode(Object)
#define NewnSchema()		NewNode(nSchema)
#define NewnClass()		NewNode(nClass)
#define NewnAttr()		NewNode(nAttr)
#define NewnConstraint()		NewNode(nConstraint)
#define NewnAttrSym()		NewNode(nAttrSym)

