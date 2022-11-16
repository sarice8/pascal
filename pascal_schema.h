/* Generated automatically by schema */

#include <stdint.h>
#include "schema_db.h"

#ifndef Object
typedef enum {
    nINVALID,
    Object,
    nWorkspace,
    nModule,
    nProgram,
    nUnit,
    nUnitImpl,
    nScope,
    nDeclaration,
    nIdent,
    nUnitRef,
    nMethod,
    nProc,
    nFunc,
    nBuiltInFunc,
    nBuiltInProc,
    nConst,
    nEnumValue,
    nTypeDecl,
    nVar,
    nGlobalVar,
    nLocalVar,
    nRecordField,
    nParam,
    nTypedConst,
    nLabel,
    nType,
    nFileType,
    nIntegerType,
    nByteType,
    nBooleanType,
    nBooleanFlowType,
    nCharType,
    nPointerType,
    nUniversalPointerType,
    nArrayType,
    nSubrangeType,
    nRecordType,
    nStrLitType,
    nShortStringType,
    nEnumType,
    nSetType,
} ObjectType;

typedef enum {
    qINVALID,
    qUnits,
    qProgram,
    qGlobalSize,
    qIdent,
    qUsedUnits,
    qChildScope,
    qMainRoutineScope,
    qImpl,
    qPublicScope,
    qPrivateScope,
    qInitLabel,
    qFinalLabel,
    qInitRoutineScope,
    qLevel,
    qDecls,
    qExtends,
    qSize,
    qAllocMode,
    qInitCode,
    qParentScope,
    qType,
    qValue,
    qParams,
    qBodyDefined,
    qExternal,
    qExternalName,
    qCdecl,
    qUsed,
    qOldParams,
    qOldType,
    qResultOffset,
    qNameOffset,
    qInOut,
    qDefined,
    qPointerType,
    qBaseType,
    qIndexType,
    qLow,
    qHigh,
    qScope,
    qCapacity,
    qNameTable,
    qHasGap,
} AttrCode;
#endif /* Object */

#define GetqINVALID(N)		(Integer4)	GetAttr(N, qINVALID)
#define GetqUnits(N)		(List)	GetAttr(N, qUnits)
#define GetqProgram(N)		(Node)	GetAttr(N, qProgram)
#define GetqGlobalSize(N)		(Integer4)	(intptr_t) GetAttr(N, qGlobalSize)
#define GetqIdent(N)		(Integer4)	(intptr_t) GetAttr(N, qIdent)
#define GetqUsedUnits(N)		(List)	GetAttr(N, qUsedUnits)
#define GetqChildScope(N)		(Node)	GetAttr(N, qChildScope)
#define GetqMainRoutineScope(N)		(Node)	GetAttr(N, qMainRoutineScope)
#define GetqImpl(N)		(Node)	GetAttr(N, qImpl)
#define GetqPublicScope(N)		(Node)	GetAttr(N, qPublicScope)
#define GetqPrivateScope(N)		(Node)	GetAttr(N, qPrivateScope)
#define GetqInitLabel(N)		(Integer4)	(intptr_t) GetAttr(N, qInitLabel)
#define GetqFinalLabel(N)		(Integer4)	(intptr_t) GetAttr(N, qFinalLabel)
#define GetqInitRoutineScope(N)		(Node)	GetAttr(N, qInitRoutineScope)
#define GetqLevel(N)		(Integer4)	(intptr_t) GetAttr(N, qLevel)
#define GetqDecls(N)		(List)	GetAttr(N, qDecls)
#define GetqExtends(N)		(Node)	GetAttr(N, qExtends)
#define GetqSize(N)		(Integer4)	(intptr_t) GetAttr(N, qSize)
#define GetqAllocMode(N)		(Boolean1)	(intptr_t) GetAttr(N, qAllocMode)
#define GetqInitCode(N)		(Integer4)	(intptr_t) GetAttr(N, qInitCode)
#define GetqParentScope(N)		(Node)	GetAttr(N, qParentScope)
#define GetqType(N)		(Node)	GetAttr(N, qType)
#define GetqValue(N)		(Integer4)	(intptr_t) GetAttr(N, qValue)
#define GetqParams(N)		(Node)	GetAttr(N, qParams)
#define GetqBodyDefined(N)		(Boolean1)	(intptr_t) GetAttr(N, qBodyDefined)
#define GetqExternal(N)		(Boolean1)	(intptr_t) GetAttr(N, qExternal)
#define GetqExternalName(N)		(StringN)	GetAttr(N, qExternalName)
#define GetqCdecl(N)		(Boolean1)	(intptr_t) GetAttr(N, qCdecl)
#define GetqUsed(N)		(Boolean1)	(intptr_t) GetAttr(N, qUsed)
#define GetqOldParams(N)		(Node)	GetAttr(N, qOldParams)
#define GetqOldType(N)		(Node)	GetAttr(N, qOldType)
#define GetqResultOffset(N)		(Integer4)	(intptr_t) GetAttr(N, qResultOffset)
#define GetqNameOffset(N)		(Integer4)	(intptr_t) GetAttr(N, qNameOffset)
#define GetqInOut(N)		(Boolean1)	(intptr_t) GetAttr(N, qInOut)
#define GetqDefined(N)		(Boolean1)	(intptr_t) GetAttr(N, qDefined)
#define GetqPointerType(N)		(Node)	GetAttr(N, qPointerType)
#define GetqBaseType(N)		(Node)	GetAttr(N, qBaseType)
#define GetqIndexType(N)		(Node)	GetAttr(N, qIndexType)
#define GetqLow(N)		(Integer4)	(intptr_t) GetAttr(N, qLow)
#define GetqHigh(N)		(Integer4)	(intptr_t) GetAttr(N, qHigh)
#define GetqScope(N)		(Node)	GetAttr(N, qScope)
#define GetqCapacity(N)		(Integer4)	(intptr_t) GetAttr(N, qCapacity)
#define GetqNameTable(N)		(Integer4)	(intptr_t) GetAttr(N, qNameTable)
#define GetqHasGap(N)		(Boolean1)	(intptr_t) GetAttr(N, qHasGap)

#define SetqINVALID(N,V)		SetAttr(N, qINVALID, V)
#define SetqUnits(N,V)		SetAttr(N, qUnits, V)
#define SetqProgram(N,V)		SetAttr(N, qProgram, V)
#define SetqGlobalSize(N,V)		SetAttr(N, qGlobalSize, (void*)(intptr_t)(V))
#define SetqIdent(N,V)		SetAttr(N, qIdent, (void*)(intptr_t)(V))
#define SetqUsedUnits(N,V)		SetAttr(N, qUsedUnits, V)
#define SetqChildScope(N,V)		SetAttr(N, qChildScope, V)
#define SetqMainRoutineScope(N,V)		SetAttr(N, qMainRoutineScope, V)
#define SetqImpl(N,V)		SetAttr(N, qImpl, V)
#define SetqPublicScope(N,V)		SetAttr(N, qPublicScope, V)
#define SetqPrivateScope(N,V)		SetAttr(N, qPrivateScope, V)
#define SetqInitLabel(N,V)		SetAttr(N, qInitLabel, (void*)(intptr_t)(V))
#define SetqFinalLabel(N,V)		SetAttr(N, qFinalLabel, (void*)(intptr_t)(V))
#define SetqInitRoutineScope(N,V)		SetAttr(N, qInitRoutineScope, V)
#define SetqLevel(N,V)		SetAttr(N, qLevel, (void*)(intptr_t)(V))
#define SetqDecls(N,V)		SetAttr(N, qDecls, V)
#define SetqExtends(N,V)		SetAttr(N, qExtends, V)
#define SetqSize(N,V)		SetAttr(N, qSize, (void*)(intptr_t)(V))
#define SetqAllocMode(N,V)		SetAttr(N, qAllocMode, (void*)(intptr_t)(V))
#define SetqInitCode(N,V)		SetAttr(N, qInitCode, (void*)(intptr_t)(V))
#define SetqParentScope(N,V)		SetAttr(N, qParentScope, V)
#define SetqType(N,V)		SetAttr(N, qType, V)
#define SetqValue(N,V)		SetAttr(N, qValue, (void*)(intptr_t)(V))
#define SetqParams(N,V)		SetAttr(N, qParams, V)
#define SetqBodyDefined(N,V)		SetAttr(N, qBodyDefined, (void*)(intptr_t)(V))
#define SetqExternal(N,V)		SetAttr(N, qExternal, (void*)(intptr_t)(V))
#define SetqExternalName(N,V)		SetAttr(N, qExternalName, V)
#define SetqCdecl(N,V)		SetAttr(N, qCdecl, (void*)(intptr_t)(V))
#define SetqUsed(N,V)		SetAttr(N, qUsed, (void*)(intptr_t)(V))
#define SetqOldParams(N,V)		SetAttr(N, qOldParams, V)
#define SetqOldType(N,V)		SetAttr(N, qOldType, V)
#define SetqResultOffset(N,V)		SetAttr(N, qResultOffset, (void*)(intptr_t)(V))
#define SetqNameOffset(N,V)		SetAttr(N, qNameOffset, (void*)(intptr_t)(V))
#define SetqInOut(N,V)		SetAttr(N, qInOut, (void*)(intptr_t)(V))
#define SetqDefined(N,V)		SetAttr(N, qDefined, (void*)(intptr_t)(V))
#define SetqPointerType(N,V)		SetAttr(N, qPointerType, V)
#define SetqBaseType(N,V)		SetAttr(N, qBaseType, V)
#define SetqIndexType(N,V)		SetAttr(N, qIndexType, V)
#define SetqLow(N,V)		SetAttr(N, qLow, (void*)(intptr_t)(V))
#define SetqHigh(N,V)		SetAttr(N, qHigh, (void*)(intptr_t)(V))
#define SetqScope(N,V)		SetAttr(N, qScope, V)
#define SetqCapacity(N,V)		SetAttr(N, qCapacity, (void*)(intptr_t)(V))
#define SetqNameTable(N,V)		SetAttr(N, qNameTable, (void*)(intptr_t)(V))
#define SetqHasGap(N,V)		SetAttr(N, qHasGap, (void*)(intptr_t)(V))

#define NewnINVALID()		NewNode(nINVALID)
#define NewObject()		NewNode(Object)
#define NewnWorkspace()		NewNode(nWorkspace)
#define NewnModule()		NewNode(nModule)
#define NewnProgram()		NewNode(nProgram)
#define NewnUnit()		NewNode(nUnit)
#define NewnUnitImpl()		NewNode(nUnitImpl)
#define NewnScope()		NewNode(nScope)
#define NewnDeclaration()		NewNode(nDeclaration)
#define NewnIdent()		NewNode(nIdent)
#define NewnUnitRef()		NewNode(nUnitRef)
#define NewnMethod()		NewNode(nMethod)
#define NewnProc()		NewNode(nProc)
#define NewnFunc()		NewNode(nFunc)
#define NewnBuiltInFunc()		NewNode(nBuiltInFunc)
#define NewnBuiltInProc()		NewNode(nBuiltInProc)
#define NewnConst()		NewNode(nConst)
#define NewnEnumValue()		NewNode(nEnumValue)
#define NewnTypeDecl()		NewNode(nTypeDecl)
#define NewnVar()		NewNode(nVar)
#define NewnGlobalVar()		NewNode(nGlobalVar)
#define NewnLocalVar()		NewNode(nLocalVar)
#define NewnRecordField()		NewNode(nRecordField)
#define NewnParam()		NewNode(nParam)
#define NewnTypedConst()		NewNode(nTypedConst)
#define NewnLabel()		NewNode(nLabel)
#define NewnType()		NewNode(nType)
#define NewnFileType()		NewNode(nFileType)
#define NewnIntegerType()		NewNode(nIntegerType)
#define NewnByteType()		NewNode(nByteType)
#define NewnBooleanType()		NewNode(nBooleanType)
#define NewnBooleanFlowType()		NewNode(nBooleanFlowType)
#define NewnCharType()		NewNode(nCharType)
#define NewnPointerType()		NewNode(nPointerType)
#define NewnUniversalPointerType()		NewNode(nUniversalPointerType)
#define NewnArrayType()		NewNode(nArrayType)
#define NewnSubrangeType()		NewNode(nSubrangeType)
#define NewnRecordType()		NewNode(nRecordType)
#define NewnStrLitType()		NewNode(nStrLitType)
#define NewnShortStringType()		NewNode(nShortStringType)
#define NewnEnumType()		NewNode(nEnumType)
#define NewnSetType()		NewNode(nSetType)

