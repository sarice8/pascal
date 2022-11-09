#define SSL_CODE_TABLE_SIZE 11000

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
#define tFetchI 22
#define tFetchB 23
#define tFetchP 24
#define tAssignI 25
#define tAssignB 26
#define tAssignP 27
#define tCopy 28
#define tCastBtoI 29
#define tCastItoB 30
#define tIncI 31
#define tDecI 32
#define tMultI 33
#define tDivI 34
#define tAddPI 35
#define tAddI 36
#define tSubI 37
#define tNegI 38
#define tNot 39
#define tEqualI 40
#define tNotEqualI 41
#define tGreaterI 42
#define tLessI 43
#define tGreaterEqualI 44
#define tLessEqualI 45
#define tEqualP 46
#define tNotEqualP 47
#define tAllocActuals 48
#define tAllocActualsCdecl 49
#define tFreeActuals 50
#define tCall 51
#define tCallCdecl 52
#define tReturn 53
#define tEnter 54
#define tJump 55
#define tJumpTrue 56
#define tJumpFalse 57
#define tLabel 58
#define tLabelAlias 59
#define tLabelExtern 60
#define tWriteI 61
#define tWriteBool 62
#define tWriteChar 63
#define tWriteStr 64
#define tWriteP 65
#define tWriteEnum 66
#define tWriteCR 67
#define tSpace 68
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
#define nStringType 33
#define nPointerType 34
#define nArrayType 35
#define nSubrangeType 36
#define nRecordType 37
#define nEnumType 38
#define nSetType 39
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
#define qNameTable 41
#define qHasGap 42
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
#define oIdAdd_mysystem 96
#define oIdAdd_File 97
#define oIdAdd_Integer 98
#define oIdAdd_Boolean 99
#define oIdAdd_Char 100
#define oIdAdd_Byte 101
#define oIdAdd_String 102
#define oIdAdd_True 103
#define oIdAdd_False 104
#define oIdAdd_Ord 105
#define oIdAdd_Chr 106
#define oIdAdd_Pred 107
#define oIdAdd_Succ 108
#define oChangeIntLitToLabelIdent 109
#define oLabelNew 110
#define oCodeNew 111
#define oCodePush 112
#define oCodePop 113
#define oIncludeUnitFile 114
#define oIncludeEnd 115
#define oCountPush 116
#define oCountInc 117
#define oCountDec 118
#define oCountIsZero 119
#define oCountPop 120
#define oValuePush 121
#define oValueNegate 122
#define oValueTop 123
#define oValuePop 124
#define oStringAllocLit 125
#define oLoopPush 126
#define oLoopContinueLabel 127
#define oLoopBreakLabel 128
#define oLoopPop 129
#define oMsg 130
#define oMsgTrace 131
#define oMsgNode 132
#define oMsgNodeLong 133
#define oMsgNodeVec 134

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
   "", 0
};
int ssl_error_table_size = 29;

#endif // SSL_INCLUDE_ERR_TABLE
