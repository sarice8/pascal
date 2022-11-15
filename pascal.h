#define SSL_CODE_TABLE_SIZE 12100

#define oJumpForward 0
#define oJumpBack 1
#define oInput 2
#define oInputAny 3
#define oEmit 4
#define oError 5
#define oInputChoice 6
#define oCall 7
#define oReturn 8
#define oSetResult 9
#define oChoice 10
#define oEndChoice 11
#define oPushResult 12
#define oPop 13
#define oBreak 14
#define oGlobalSpace 15
#define oLocalSpace 16
#define oGetParam 17
#define oGetFromParam 18
#define oGetLocal 19
#define oGetGlobal 20
#define oGetAddrParam 21
#define oGetAddrLocal 22
#define oGetAddrGlobal 23
#define oAssign 24
#define pIdent 0
#define pIntLit 1
#define pStrLit 2
#define pCharLit 3
#define pAssign 4
#define pSemiColon 5
#define pEqual 6
#define pNotEqual 7
#define pLess 8
#define pGreater 9
#define pLessEqual 10
#define pGreaterEqual 11
#define pColon 12
#define pComma 13
#define pLParen 14
#define pRParen 15
#define pLSquare 16
#define pRSquare 17
#define pCarat 18
#define pAt 19
#define pDot 20
#define pDotDot 21
#define pTimes 22
#define pDivide 23
#define pPlus 24
#define pMinus 25
#define pEof 26
#define pInvalid 27
#define pProgram 28
#define pProcedure 29
#define pFunction 30
#define pConst 31
#define pType 32
#define pVar 33
#define pLabel 34
#define pBegin 35
#define pEnd 36
#define pArray 37
#define pRecord 38
#define pSet 39
#define pOf 40
#define pIf 41
#define pThen 42
#define pElse 43
#define pFor 44
#define pTo 45
#define pDownto 46
#define pDo 47
#define pWhile 48
#define pRepeat 49
#define pUntil 50
#define pContinue 51
#define pBreak 52
#define pGoto 53
#define pAnd 54
#define pOr 55
#define pNot 56
#define pUses 57
#define pUnit 58
#define pInterface 59
#define pImplementation 60
#define pInitialization 61
#define pFinalization 62
#define pWriteln 63
#define pWrite 64
#define pReadln 65
#define pRead 66
#define pForward 67
#define pExternal 68
#define pName 69
#define pCdecl 70
#define tPushGlobalI 0
#define tPushGlobalB 1
#define tPushGlobalP 2
#define tPushLocalI 3
#define tPushLocalB 4
#define tPushLocalP 5
#define tPushParamI 6
#define tPushParamB 7
#define tPushParamP 8
#define tPushUpLocalI 9
#define tPushUpLocalB 10
#define tPushUpLocalP 11
#define tPushUpParamI 12
#define tPushUpParamB 13
#define tPushUpParamP 14
#define tPushConstI 15
#define tPushAddrGlobal 16
#define tPushAddrLocal 17
#define tPushAddrParam 18
#define tPushAddrActual 19
#define tPushAddrUpLocal 20
#define tPushAddrUpParam 21
#define tSwap 22
#define tFetchI 23
#define tFetchB 24
#define tFetchP 25
#define tAssignI 26
#define tAssignB 27
#define tAssignP 28
#define tCopy 29
#define tCastBtoI 30
#define tCastItoB 31
#define tIncI 32
#define tDecI 33
#define tMultI 34
#define tDivI 35
#define tAddPI 36
#define tAddI 37
#define tSubP 38
#define tSubPI 39
#define tSubI 40
#define tNegI 41
#define tNot 42
#define tEqualB 43
#define tNotEqualB 44
#define tGreaterB 45
#define tLessB 46
#define tGreaterEqualB 47
#define tLessEqualB 48
#define tEqualI 49
#define tNotEqualI 50
#define tGreaterI 51
#define tLessI 52
#define tGreaterEqualI 53
#define tLessEqualI 54
#define tEqualP 55
#define tNotEqualP 56
#define tGreaterP 57
#define tLessP 58
#define tGreaterEqualP 59
#define tLessEqualP 60
#define tAllocActuals 61
#define tAllocActualsCdecl 62
#define tFreeActuals 63
#define tCall 64
#define tCallCdecl 65
#define tReturn 66
#define tEnter 67
#define tJump 68
#define tJumpTrue 69
#define tJumpFalse 70
#define tLabel 71
#define tLabelAlias 72
#define tLabelExtern 73
#define tWriteI 74
#define tWriteBool 75
#define tWriteChar 76
#define tWriteShortStr 77
#define tWritePChar 78
#define tWriteP 79
#define tWriteEnum 80
#define tWriteCR 81
#define tFile 82
#define tLine 83
#define tSpace 84
#define eBadStatement 0
#define eNotConst 1
#define eNotType 2
#define eNotOrdinalType 3
#define eNotVar 4
#define eNotIntVar 5
#define eNotValue 6
#define eNotInteger 7
#define eNotBoolean 8
#define eNotPointer 9
#define eNotArray 10
#define eNotRecord 11
#define eNotRecordField 12
#define eTooManySubscripts 13
#define eTypeMismatch 14
#define eMissingParameter 15
#define eNotImplemented 16
#define eNotAllowed 17
#define eNotInALoop 18
#define eRecordEmpty 19
#define eNotCurrentFunction 20
#define eAlreadyDefined 21
#define eOnlyOneVarCanBeInitialized 22
#define eExternalMethodCannotBeNested 23
#define eCantFindUnitFile 24
#define eInternalScopeMismatch 25
#define eEnumValueNotAscending 26
#define eUsedButNotDefined 27
#define eCantUsePredSuccOnEnumWithValueGaps 28
#define eCantDereference 29
#define nINVALID 0
#define Object 1
#define nWorkspace 2
#define nModule 3
#define nProgram 4
#define nUnit 5
#define nUnitImpl 6
#define nScope 7
#define nDeclaration 8
#define nIdent 9
#define nUnitRef 10
#define nMethod 11
#define nProc 12
#define nFunc 13
#define nBuiltInFunc 14
#define nBuiltInProc 15
#define nConst 16
#define nEnumValue 17
#define nTypeDecl 18
#define nVar 19
#define nGlobalVar 20
#define nLocalVar 21
#define nRecordField 22
#define nParam 23
#define nTypedConst 24
#define nLabel 25
#define nType 26
#define nFileType 27
#define nIntegerType 28
#define nByteType 29
#define nBooleanType 30
#define nBooleanCFType 31
#define nCharType 32
#define nPointerType 33
#define nUniversalPointerType 34
#define nArrayType 35
#define nSubrangeType 36
#define nRecordType 37
#define nStrLitType 38
#define nShortStringType 39
#define nEnumType 40
#define nSetType 41
#define qINVALID 0
#define qUnits 1
#define qProgram 2
#define qGlobalSize 3
#define qIdent 4
#define qUsedUnits 5
#define qChildScope 6
#define qMainRoutineScope 7
#define qImpl 8
#define qPublicScope 9
#define qPrivateScope 10
#define qInitLabel 11
#define qFinalLabel 12
#define qInitRoutineScope 13
#define qLevel 14
#define qDecls 15
#define qExtends 16
#define qSize 17
#define qAllocMode 18
#define qInitCode 19
#define qParentScope 20
#define qType 21
#define qValue 22
#define qParams 23
#define qBodyDefined 24
#define qExternal 25
#define qExternalName 26
#define qCdecl 27
#define qUsed 28
#define qOldParams 29
#define qOldType 30
#define qResultOffset 31
#define qNameOffset 32
#define qInOut 33
#define qDefined 34
#define qPointerType 35
#define qBaseType 36
#define qIndexType 37
#define qLow 38
#define qHigh 39
#define qScope 40
#define qCapacity 41
#define qNameTable 42
#define qHasGap 43
#define Null 0
#define NullIter 0
#define NullVec 0
#define false 0
#define true 1
#define tyNone 0
#define tyInteger 1
#define tyBoolean 2
#define tyChar 3
#define tyString 4
#define tyFile 5
#define tyPointer 6
#define tyArray 7
#define tyRecord 8
#define tyParams 9
#define tySet 10
#define labelNull 0
#define codeNull 0
#define codeDefault 1
#define stringNull 0
#define allocGlobal 0
#define allocDown 1
#define allocUp 2
#define oNodeNew 25
#define oNodeSet 26
#define oNodeSetString 27
#define oNodeSetInt 28
#define oNodeSetBoolean 29
#define oNodeSetLabel 30
#define oNodeSetCode 31
#define oNodeAddLast 32
#define oNodeGet 33
#define oNodeGetString 34
#define oNodeGetInt 35
#define oNodeGetBoolean 36
#define oNodeGetLabel 37
#define oNodeGetCode 38
#define oNodeNull 39
#define oNodeFind 40
#define oNodeGetIter 41
#define oNodeGetIterLast 42
#define oNodeIterValue 43
#define oNodeIterNext 44
#define oNodeIterPrev 45
#define oNodeType 46
#define oNodeEqual 47
#define oNodeVecNew 48
#define oNodeVecDelete 49
#define oNodeVecAppend 50
#define oNodeVecSize 51
#define oNodeVecElement 52
#define oEmitInt 53
#define oEmitLabel 54
#define oEmitCode 55
#define Here 56
#define oPatch 57
#define inc 58
#define dec 59
#define negate 60
#define add 61
#define subtract 62
#define multiply 63
#define equal 64
#define equal_zero 65
#define equal_node 66
#define equal_string 67
#define equal_node_type 68
#define equal_label 69
#define equal_code 70
#define greater 71
#define oMININT 72
#define oMAXINT 73
#define TOKEN_VALUE 74
#define LAST_ID 75
#define ID_STRING 76
#define CURRENT_STRLIT 77
#define oWorkspaceNew 78
#define oScopeBegin 79
#define oScopeEnter 80
#define oScopeEnd 81
#define oScopeCurrent 82
#define oScopeDeclare 83
#define oScopeDeclareAlloc 84
#define oScopeAllocType 85
#define oScopeAlloc 86
#define oScopeFind 87
#define oScopeFindRequire 88
#define oScopeFindInCurrentScope 89
#define oScopeFindRequireInScope 90
#define oTypeAdd 91
#define oTypeSPush 92
#define oTypeSPop 93
#define oTypeSTop 94
#define oTypeSNodeType 95
#define oId_mysystem 96
#define oId_ShortStringAppendShortString 97
#define oId_ShortStringAppendChar 98
#define oId_File 99
#define oId_Integer 100
#define oId_Boolean 101
#define oId_Char 102
#define oId_Byte 103
#define oId_Pointer 104
#define oId_ShortString 105
#define oId_True 106
#define oId_False 107
#define oId_Nil 108
#define oId_Ord 109
#define oId_Chr 110
#define oId_Pred 111
#define oId_Succ 112
#define oChangeIntLitToLabelIdent 113
#define oLabelNew 114
#define oCodeNew 115
#define oCodePush 116
#define oCodePop 117
#define oIncludeUnitFile 118
#define oIncludeEnd 119
#define oCountPush 120
#define oCountInc 121
#define oCountDec 122
#define oCountIsZero 123
#define oCountPop 124
#define oValuePush 125
#define oValueNegate 126
#define oValueTop 127
#define oValuePop 128
#define oStringAllocLit 129
#define oStringAllocShortStringLit 130
#define oLoopPush 131
#define oLoopContinueLabel 132
#define oLoopBreakLabel 133
#define oLoopPop 134
#define oMsg 135
#define oMsgTrace 136
#define oMsgNode 137
#define oMsgNodeLong 138
#define oMsgNodeVec 139

#ifdef SSL_INCLUDE_ERR_TABLE

struct ssl_error_table_struct ssl_error_table[] = {
   "eBadStatement", 0,
   "eNotConst", 1,
   "eNotType", 2,
   "eNotOrdinalType", 3,
   "eNotVar", 4,
   "eNotIntVar", 5,
   "eNotValue", 6,
   "eNotInteger", 7,
   "eNotBoolean", 8,
   "eNotPointer", 9,
   "eNotArray", 10,
   "eNotRecord", 11,
   "eNotRecordField", 12,
   "eTooManySubscripts", 13,
   "eTypeMismatch", 14,
   "eMissingParameter", 15,
   "eNotImplemented", 16,
   "eNotAllowed", 17,
   "eNotInALoop", 18,
   "eRecordEmpty", 19,
   "eNotCurrentFunction", 20,
   "eAlreadyDefined", 21,
   "eOnlyOneVarCanBeInitialized", 22,
   "eExternalMethodCannotBeNested", 23,
   "eCantFindUnitFile", 24,
   "eInternalScopeMismatch", 25,
   "eEnumValueNotAscending", 26,
   "eUsedButNotDefined", 27,
   "eCantUsePredSuccOnEnumWithValueGaps", 28,
   "eCantDereference", 29,
   "", 0
};
int ssl_error_table_size = 30;

#endif // SSL_INCLUDE_ERR_TABLE
