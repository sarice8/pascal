/* Generated automatically by schema */

#include <stdint.h>
#include "schema_db.h"

#ifndef Object
typedef enum {
    nINVALID,
    Object,
    nScope,
    nDeclaration,
    nIdent,
    nOutput,
    nError,
    nType,
    nValue,
    nMechanism,
    nOperation,
    nRule,
    nVariable,
    nParam,
    nInParam,
    nOutParam,
    nInOutParam,
    nLocal,
    nGlobal,
} ObjectType;

typedef enum {
    qINVALID,
    qParentScope,
    qDecls,
    qIdent,
    qValue,
    qType,
    qParamScope,
    qScope,
    qAddrDefined,
    qTypeDefined,
    qAddr,
} AttrCode;
#endif /* Object */

#define GetqINVALID(N)		(Integer4)	GetAttr(N, qINVALID)
#define GetqParentScope(N)		(Node)	GetAttr(N, qParentScope)
#define GetqDecls(N)		(List)	GetAttr(N, qDecls)
#define GetqIdent(N)		(Integer4)	(intptr_t) GetAttr(N, qIdent)
#define GetqValue(N)		(Integer4)	(intptr_t) GetAttr(N, qValue)
#define GetqType(N)		(Node)	GetAttr(N, qType)
#define GetqParamScope(N)		(Node)	GetAttr(N, qParamScope)
#define GetqScope(N)		(Node)	GetAttr(N, qScope)
#define GetqAddrDefined(N)		(Boolean1)	(intptr_t) GetAttr(N, qAddrDefined)
#define GetqTypeDefined(N)		(Boolean1)	(intptr_t) GetAttr(N, qTypeDefined)
#define GetqAddr(N)		(Integer4)	(intptr_t) GetAttr(N, qAddr)

#define SetqINVALID(N,V)		SetAttr(N, qINVALID, V)
#define SetqParentScope(N,V)		SetAttr(N, qParentScope, V)
#define SetqDecls(N,V)		SetAttr(N, qDecls, V)
#define SetqIdent(N,V)		SetAttr(N, qIdent, (void*)(intptr_t)(V))
#define SetqValue(N,V)		SetAttr(N, qValue, (void*)(intptr_t)(V))
#define SetqType(N,V)		SetAttr(N, qType, V)
#define SetqParamScope(N,V)		SetAttr(N, qParamScope, V)
#define SetqScope(N,V)		SetAttr(N, qScope, V)
#define SetqAddrDefined(N,V)		SetAttr(N, qAddrDefined, (void*)(intptr_t)(V))
#define SetqTypeDefined(N,V)		SetAttr(N, qTypeDefined, (void*)(intptr_t)(V))
#define SetqAddr(N,V)		SetAttr(N, qAddr, (void*)(intptr_t)(V))

#define NewnINVALID()		NewNode(nINVALID)
#define NewObject()		NewNode(Object)
#define NewnScope()		NewNode(nScope)
#define NewnDeclaration()		NewNode(nDeclaration)
#define NewnIdent()		NewNode(nIdent)
#define NewnOutput()		NewNode(nOutput)
#define NewnError()		NewNode(nError)
#define NewnType()		NewNode(nType)
#define NewnValue()		NewNode(nValue)
#define NewnMechanism()		NewNode(nMechanism)
#define NewnOperation()		NewNode(nOperation)
#define NewnRule()		NewNode(nRule)
#define NewnVariable()		NewNode(nVariable)
#define NewnParam()		NewNode(nParam)
#define NewnInParam()		NewNode(nInParam)
#define NewnOutParam()		NewNode(nOutParam)
#define NewnInOutParam()		NewNode(nInOutParam)
#define NewnLocal()		NewNode(nLocal)
#define NewnGlobal()		NewNode(nGlobal)

