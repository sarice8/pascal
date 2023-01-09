#define SSL_CODE_TABLE_SIZE 15300

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
#define pDoubleLit 2
#define pStrLit 3
#define pCharLit 4
#define pAssign 5
#define pSemiColon 6
#define pEqual 7
#define pNotEqual 8
#define pLess 9
#define pGreater 10
#define pLessEqual 11
#define pGreaterEqual 12
#define pColon 13
#define pComma 14
#define pLParen 15
#define pRParen 16
#define pLSquare 17
#define pRSquare 18
#define pCarat 19
#define pAt 20
#define pDot 21
#define pDotDot 22
#define pTimes 23
#define pDivide 24
#define pPlus 25
#define pMinus 26
#define pEof 27
#define pInvalid 28
#define pProgram 29
#define pProcedure 30
#define pFunction 31
#define pConst 32
#define pType 33
#define pVar 34
#define pLabel 35
#define pBegin 36
#define pEnd 37
#define pArray 38
#define pRecord 39
#define pSet 40
#define pOf 41
#define pIf 42
#define pThen 43
#define pElse 44
#define pFor 45
#define pTo 46
#define pDownto 47
#define pDo 48
#define pWhile 49
#define pRepeat 50
#define pUntil 51
#define pContinue 52
#define pBreak 53
#define pCase 54
#define pOtherwise 55
#define pGoto 56
#define pAnd 57
#define pOr 58
#define pNot 59
#define pUses 60
#define pUnit 61
#define pInterface 62
#define pImplementation 63
#define pInitialization 64
#define pFinalization 65
#define pWriteln 66
#define pWrite 67
#define pReadln 68
#define pRead 69
#define pForward 70
#define pExternal 71
#define pName 72
#define pCdecl 73
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
#define tPushConstD 16
#define tPushAddrGlobal 17
#define tPushAddrLocal 18
#define tPushAddrParam 19
#define tPushAddrActual 20
#define tPushAddrUpLocal 21
#define tPushAddrUpParam 22
#define tSwap 23
#define tFetchI 24
#define tFetchB 25
#define tFetchP 26
#define tAssignI 27
#define tAssignB 28
#define tAssignP 29
#define tAssignD 30
#define tCopy 31
#define tCastBtoI 32
#define tCastItoB 33
#define tCastItoD 34
#define tIncI 35
#define tDecI 36
#define tMultI 37
#define tDivI 38
#define tAddPI 39
#define tAddI 40
#define tSubP 41
#define tSubPI 42
#define tSubI 43
#define tNegI 44
#define tMultD 45
#define tDivD 46
#define tAddD 47
#define tSubD 48
#define tNegD 49
#define tNot 50
#define tEqualB 51
#define tNotEqualB 52
#define tGreaterB 53
#define tLessB 54
#define tGreaterEqualB 55
#define tLessEqualB 56
#define tEqualI 57
#define tNotEqualI 58
#define tGreaterI 59
#define tLessI 60
#define tGreaterEqualI 61
#define tLessEqualI 62
#define tEqualP 63
#define tNotEqualP 64
#define tGreaterP 65
#define tLessP 66
#define tGreaterEqualP 67
#define tLessEqualP 68
#define tGreaterD 69
#define tLessD 70
#define tGreaterEqualD 71
#define tLessEqualD 72
#define tAllocActuals 73
#define tAllocActualsCdecl 74
#define tFreeActuals 75
#define tCall 76
#define tCallCdecl 77
#define tReturn 78
#define tEnter 79
#define tJump 80
#define tJumpTrue 81
#define tJumpFalse 82
#define tJumpCaseB 83
#define tJumpCaseI 84
#define tJumpCaseS 85
#define tCase 86
#define tCaseRange 87
#define tCaseEnd 88
#define tLabel 89
#define tLabelAlias 90
#define tLabelExtern 91
#define tWriteI 92
#define tWriteBool 93
#define tWriteChar 94
#define tWriteShortStr 95
#define tWritePChar 96
#define tWriteP 97
#define tWriteEnum 98
#define tWriteD 99
#define tWriteCR 100
#define tReadI 101
#define tReadChar 102
#define tReadShortStr 103
#define tReadCR 104
#define tFile 105
#define tLine 106
#define tSpace 107
#define eBadStatement 0
#define eNotConst 1
#define eNotType 2
#define eNotOrdinalType 3
#define eNotVar 4
#define eNotIntVar 5
#define eNotValue 6
#define eNotInteger 7
#define eNotBoolean 8
#define eNotDouble 9
#define eNotPointer 10
#define eNotArray 11
#define eNotRecord 12
#define eNotRecordField 13
#define eTooManySubscripts 14
#define eTypeMismatch 15
#define eMissingParameter 16
#define eNotImplemented 17
#define eNotAllowed 18
#define eNotInALoop 19
#define eRecordEmpty 20
#define eNotCurrentFunction 21
#define eAlreadyDefined 22
#define eOnlyOneVarCanBeInitialized 23
#define eExternalMethodCannotBeNested 24
#define eCantFindUnitFile 25
#define eInternalScopeMismatch 26
#define eEnumValueNotAscending 27
#define eUsedButNotDefined 28
#define eCantUsePredSuccOnEnumWithValueGaps 29
#define eCantDereference 30
#define eSizeMismatch 31
#define eTypeNameNotAllowedHere 32
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
#define nConstStr 18
#define nConstDouble 19
#define nTypeDecl 20
#define nVar 21
#define nGlobalVar 22
#define nLocalVar 23
#define nRecordField 24
#define nParam 25
#define nTypedConst 26
#define nLabel 27
#define nType 28
#define nFileType 29
#define nByteType 30
#define nIntegerType 31
#define nSingleType 32
#define nDoubleType 33
#define nBooleanType 34
#define nBooleanFlowType 35
#define nCharType 36
#define nPointerType 37
#define nUniversalPointerType 38
#define nArrayType 39
#define nSubrangeType 40
#define nRecordType 41
#define nStrLitType 42
#define nShortStringType 43
#define nEnumType 44
#define nSetType 45
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
#define qValueStr 33
#define qValueDouble 34
#define qInOut 35
#define qDefined 36
#define qPointerType 37
#define qBaseType 38
#define qIndexType 39
#define qLow 40
#define qHigh 41
#define qScope 42
#define qCapacity 43
#define qNameTable 44
#define qHasGap 45
#define Null 0
#define NullIter 0
#define NullVec 0
#define false 0
#define true 1
#define doubleZero 0
#define labelNull 0
#define codeNull 0
#define codeDefault 1
#define stringNull 0
#define allocGlobal 0
#define allocDown 1
#define allocUp 2
#define oNodeNew 25
#define oNodeSet 26
#define oNodeSetDouble 27
#define oNodeSetString 28
#define oNodeSetInt 29
#define oNodeSetBoolean 30
#define oNodeSetLabel 31
#define oNodeSetCode 32
#define oNodeAddLast 33
#define oNodeGet 34
#define oNodeGetDouble 35
#define oNodeGetString 36
#define oNodeGetInt 37
#define oNodeGetBoolean 38
#define oNodeGetLabel 39
#define oNodeGetCode 40
#define oNodeNull 41
#define oNodeFind 42
#define oNodeGetIter 43
#define oNodeGetIterLast 44
#define oNodeIterValue 45
#define oNodeIterNext 46
#define oNodeIterPrev 47
#define oNodeType 48
#define oNodeEqual 49
#define oNodeVecNew 50
#define oNodeVecDelete 51
#define oNodeVecAppend 52
#define oNodeVecSize 53
#define oNodeVecElement 54
#define oEmitInt 55
#define oEmitDouble 56
#define oEmitLabel 57
#define oEmitCode 58
#define Here 59
#define oPatch 60
#define inc 61
#define dec 62
#define negate 63
#define add 64
#define subtract 65
#define multiply 66
#define equal 67
#define equal_zero 68
#define equal_node 69
#define equal_string 70
#define equal_node_type 71
#define equal_label 72
#define equal_code 73
#define greater 74
#define oMININT 75
#define oMAXINT 76
#define TOKEN_VALUE 77
#define TOKEN_VALUE_DOUBLE 78
#define LAST_ID 79
#define ID_STRING 80
#define CURRENT_STRLIT 81
#define UNACCEPT_TOKEN 82
#define oWorkspaceNew 83
#define oScopeBegin 84
#define oScopeEnter 85
#define oScopeEnd 86
#define oScopeCurrent 87
#define oScopeDeclare 88
#define oScopeDeclareAlloc 89
#define oScopeAllocType 90
#define oScopeAlloc 91
#define oScopeFind 92
#define oScopeFindRequire 93
#define oScopeFindInCurrentScope 94
#define oScopeFindRequireInScope 95
#define oTypeAdd 96
#define oTypeSPush 97
#define oTypeSPop 98
#define oTypeSTop 99
#define oTypeSNodeType 100
#define oId_mysystem 101
#define oId_ShortStringAppendShortString 102
#define oId_ShortStringAppendChar 103
#define oId_ShortStringCmp 104
#define oId_File 105
#define oId_Integer 106
#define oId_Boolean 107
#define oId_Char 108
#define oId_Byte 109
#define oId_Single 110
#define oId_Double 111
#define oId_Pointer 112
#define oId_ShortString 113
#define oId_True 114
#define oId_False 115
#define oId_Nil 116
#define oId_Ord 117
#define oId_Chr 118
#define oId_Pred 119
#define oId_Succ 120
#define oId_Sizeof 121
#define oChangeIntLitToLabelIdent 122
#define oLabelNew 123
#define oCodeNew 124
#define oCodePush 125
#define oCodePop 126
#define oCodeDiscard 127
#define oIncludeUnitFile 128
#define oIncludeEnd 129
#define oCountPush 130
#define oCountInc 131
#define oCountDec 132
#define oCountIsZero 133
#define oCountPop 134
#define oValuePush 135
#define oValuePushDouble 136
#define oValuePushString 137
#define oValueTop 138
#define oValueTopDouble 139
#define oValueTopString 140
#define oValueSwap 141
#define oValuePop 142
#define oValueCharToString 143
#define oValueIntToDouble 144
#define oValueNegate 145
#define oValueEqual 146
#define oValueNotEqual 147
#define oValueLess 148
#define oValueGreater 149
#define oValueLessEqual 150
#define oValueGreaterEqual 151
#define oValueNegateD 152
#define oValueEqualD 153
#define oValueNotEqualD 154
#define oValueLessD 155
#define oValueGreaterD 156
#define oValueLessEqualD 157
#define oValueGreaterEqualD 158
#define oValueOr 159
#define oValueAnd 160
#define oValueNot 161
#define oValueAdd 162
#define oValueSub 163
#define oValueMult 164
#define oValueDiv 165
#define oValueAddD 166
#define oValueSubD 167
#define oValueMultD 168
#define oValueDivD 169
#define oValueStringCmp 170
#define oValueStringConcat 171
#define oStringAllocLit 172
#define oStringAllocShortStringLit 173
#define oLoopPush 174
#define oLoopContinueLabel 175
#define oLoopBreakLabel 176
#define oLoopPop 177
#define oMsg 178
#define oMsgTrace 179
#define oMsgNode 180
#define oMsgNodeLong 181
#define oMsgNodeVec 182

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
   "eNotDouble", 9,
   "eNotPointer", 10,
   "eNotArray", 11,
   "eNotRecord", 12,
   "eNotRecordField", 13,
   "eTooManySubscripts", 14,
   "eTypeMismatch", 15,
   "eMissingParameter", 16,
   "eNotImplemented", 17,
   "eNotAllowed", 18,
   "eNotInALoop", 19,
   "eRecordEmpty", 20,
   "eNotCurrentFunction", 21,
   "eAlreadyDefined", 22,
   "eOnlyOneVarCanBeInitialized", 23,
   "eExternalMethodCannotBeNested", 24,
   "eCantFindUnitFile", 25,
   "eInternalScopeMismatch", 26,
   "eEnumValueNotAscending", 27,
   "eUsedButNotDefined", 28,
   "eCantUsePredSuccOnEnumWithValueGaps", 29,
   "eCantDereference", 30,
   "eSizeMismatch", 31,
   "eTypeNameNotAllowedHere", 32,
   "", 0
};
int ssl_error_table_size = 33;

#endif // SSL_INCLUDE_ERR_TABLE
