/* Generated automatically by schema */

#include <stdint.h>
#include "schema_db.h"

#ifndef Object
typedef enum {
    nINVALID,
    Object,
    nWorkspace,
    nProgram,
    nUnit,
    nScope,
    nDeclaration,
    nIdent,
    nMethod,
    nProc,
    nFunc,
    nConst,
    nTypeDecl,
    nVar,
    nGlobalVar,
    nLocalVar,
    nRecordField,
    nParam,
    nType,
    nFileType,
    nIntegerType,
    nBooleanType,
    nBooleanCFType,
    nCharType,
    nStringType,
    nPointerType,
    nArrayType,
    nSubrangeType,
    nRecordType,
    nEnumType,
    nSetType,
} ObjectType;

typedef enum {
    qINVALID,
    qUnits,
    qProgram,
    qIdent,
    qUsedUnits,
    qChildScope,
    qMainRoutineScope,
    qInitLabel,
    qFinalLabel,
    qInitRoutineScope,
    qFinalRoutineScope,
    qLevel,
    qDecls,
    qSize,
    qAllocDown,
    qInitCode,
    qParentScope,
    qType,
    qValue,
    qParams,
    qBodyDefined,
    qExternal,
    qExternalName,
    qCdecl,
    qCalled,
    qOldParams,
    qOldType,
    qResultOffset,
    qInOut,
    qPointerType,
    qBaseType,
    qIndexType,
    qLow,
    qHigh,
    qScope,
} AttrCode;
#endif /* Object */

#define GetqINVALID(N)		(Integer4)	GetAttr(N, qINVALID)
#define GetqUnits(N)		(List)	GetAttr(N, qUnits)
#define GetqProgram(N)		(Node)	GetAttr(N, qProgram)
#define GetqIdent(N)		(Integer4)	(intptr_t) GetAttr(N, qIdent)
#define GetqUsedUnits(N)		(List)	GetAttr(N, qUsedUnits)
#define GetqChildScope(N)		(Node)	GetAttr(N, qChildScope)
#define GetqMainRoutineScope(N)		(Node)	GetAttr(N, qMainRoutineScope)
#define GetqInitLabel(N)		(Integer4)	(intptr_t) GetAttr(N, qInitLabel)
#define GetqFinalLabel(N)		(Integer4)	(intptr_t) GetAttr(N, qFinalLabel)
#define GetqInitRoutineScope(N)		(Node)	GetAttr(N, qInitRoutineScope)
#define GetqFinalRoutineScope(N)		(Node)	GetAttr(N, qFinalRoutineScope)
#define GetqLevel(N)		(Integer4)	(intptr_t) GetAttr(N, qLevel)
#define GetqDecls(N)		(List)	GetAttr(N, qDecls)
#define GetqSize(N)		(Integer4)	(intptr_t) GetAttr(N, qSize)
#define GetqAllocDown(N)		(Boolean1)	(intptr_t) GetAttr(N, qAllocDown)
#define GetqInitCode(N)		(Integer4)	(intptr_t) GetAttr(N, qInitCode)
#define GetqParentScope(N)		(Node)	GetAttr(N, qParentScope)
#define GetqType(N)		(Node)	GetAttr(N, qType)
#define GetqValue(N)		(Integer4)	(intptr_t) GetAttr(N, qValue)
#define GetqParams(N)		(Node)	GetAttr(N, qParams)
#define GetqBodyDefined(N)		(Boolean1)	(intptr_t) GetAttr(N, qBodyDefined)
#define GetqExternal(N)		(Boolean1)	(intptr_t) GetAttr(N, qExternal)
#define GetqExternalName(N)		(StringN)	GetAttr(N, qExternalName)
#define GetqCdecl(N)		(Boolean1)	(intptr_t) GetAttr(N, qCdecl)
#define GetqCalled(N)		(Boolean1)	(intptr_t) GetAttr(N, qCalled)
#define GetqOldParams(N)		(Node)	GetAttr(N, qOldParams)
#define GetqOldType(N)		(Node)	GetAttr(N, qOldType)
#define GetqResultOffset(N)		(Integer4)	(intptr_t) GetAttr(N, qResultOffset)
#define GetqInOut(N)		(Boolean1)	(intptr_t) GetAttr(N, qInOut)
#define GetqPointerType(N)		(Node)	GetAttr(N, qPointerType)
#define GetqBaseType(N)		(Node)	GetAttr(N, qBaseType)
#define GetqIndexType(N)		(Node)	GetAttr(N, qIndexType)
#define GetqLow(N)		(Integer4)	(intptr_t) GetAttr(N, qLow)
#define GetqHigh(N)		(Integer4)	(intptr_t) GetAttr(N, qHigh)
#define GetqScope(N)		(Node)	GetAttr(N, qScope)

#define SetqINVALID(N,V)		SetAttr(N, qINVALID, V)
#define SetqUnits(N,V)		SetAttr(N, qUnits, V)
#define SetqProgram(N,V)		SetAttr(N, qProgram, V)
#define SetqIdent(N,V)		SetAttr(N, qIdent, (void*)(intptr_t)(V))
#define SetqUsedUnits(N,V)		SetAttr(N, qUsedUnits, V)
#define SetqChildScope(N,V)		SetAttr(N, qChildScope, V)
#define SetqMainRoutineScope(N,V)		SetAttr(N, qMainRoutineScope, V)
#define SetqInitLabel(N,V)		SetAttr(N, qInitLabel, (void*)(intptr_t)(V))
#define SetqFinalLabel(N,V)		SetAttr(N, qFinalLabel, (void*)(intptr_t)(V))
#define SetqInitRoutineScope(N,V)		SetAttr(N, qInitRoutineScope, V)
#define SetqFinalRoutineScope(N,V)		SetAttr(N, qFinalRoutineScope, V)
#define SetqLevel(N,V)		SetAttr(N, qLevel, (void*)(intptr_t)(V))
#define SetqDecls(N,V)		SetAttr(N, qDecls, V)
#define SetqSize(N,V)		SetAttr(N, qSize, (void*)(intptr_t)(V))
#define SetqAllocDown(N,V)		SetAttr(N, qAllocDown, (void*)(intptr_t)(V))
#define SetqInitCode(N,V)		SetAttr(N, qInitCode, (void*)(intptr_t)(V))
#define SetqParentScope(N,V)		SetAttr(N, qParentScope, V)
#define SetqType(N,V)		SetAttr(N, qType, V)
#define SetqValue(N,V)		SetAttr(N, qValue, (void*)(intptr_t)(V))
#define SetqParams(N,V)		SetAttr(N, qParams, V)
#define SetqBodyDefined(N,V)		SetAttr(N, qBodyDefined, (void*)(intptr_t)(V))
#define SetqExternal(N,V)		SetAttr(N, qExternal, (void*)(intptr_t)(V))
#define SetqExternalName(N,V)		SetAttr(N, qExternalName, V)
#define SetqCdecl(N,V)		SetAttr(N, qCdecl, (void*)(intptr_t)(V))
#define SetqCalled(N,V)		SetAttr(N, qCalled, (void*)(intptr_t)(V))
#define SetqOldParams(N,V)		SetAttr(N, qOldParams, V)
#define SetqOldType(N,V)		SetAttr(N, qOldType, V)
#define SetqResultOffset(N,V)		SetAttr(N, qResultOffset, (void*)(intptr_t)(V))
#define SetqInOut(N,V)		SetAttr(N, qInOut, (void*)(intptr_t)(V))
#define SetqPointerType(N,V)		SetAttr(N, qPointerType, V)
#define SetqBaseType(N,V)		SetAttr(N, qBaseType, V)
#define SetqIndexType(N,V)		SetAttr(N, qIndexType, V)
#define SetqLow(N,V)		SetAttr(N, qLow, (void*)(intptr_t)(V))
#define SetqHigh(N,V)		SetAttr(N, qHigh, (void*)(intptr_t)(V))
#define SetqScope(N,V)		SetAttr(N, qScope, V)

#define NewnINVALID()		NewNode(nINVALID)
#define NewObject()		NewNode(Object)
#define NewnWorkspace()		NewNode(nWorkspace)
#define NewnProgram()		NewNode(nProgram)
#define NewnUnit()		NewNode(nUnit)
#define NewnScope()		NewNode(nScope)
#define NewnDeclaration()		NewNode(nDeclaration)
#define NewnIdent()		NewNode(nIdent)
#define NewnMethod()		NewNode(nMethod)
#define NewnProc()		NewNode(nProc)
#define NewnFunc()		NewNode(nFunc)
#define NewnConst()		NewNode(nConst)
#define NewnTypeDecl()		NewNode(nTypeDecl)
#define NewnVar()		NewNode(nVar)
#define NewnGlobalVar()		NewNode(nGlobalVar)
#define NewnLocalVar()		NewNode(nLocalVar)
#define NewnRecordField()		NewNode(nRecordField)
#define NewnParam()		NewNode(nParam)
#define NewnType()		NewNode(nType)
#define NewnFileType()		NewNode(nFileType)
#define NewnIntegerType()		NewNode(nIntegerType)
#define NewnBooleanType()		NewNode(nBooleanType)
#define NewnBooleanCFType()		NewNode(nBooleanCFType)
#define NewnCharType()		NewNode(nCharType)
#define NewnStringType()		NewNode(nStringType)
#define NewnPointerType()		NewNode(nPointerType)
#define NewnArrayType()		NewNode(nArrayType)
#define NewnSubrangeType()		NewNode(nSubrangeType)
#define NewnRecordType()		NewNode(nRecordType)
#define NewnEnumType()		NewNode(nEnumType)
#define NewnSetType()		NewNode(nSetType)

