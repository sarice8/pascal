#define SSL_CODE_TABLE_SIZE 6700

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
#define pAssign 3
#define pSemiColon 4
#define pEqual 5
#define pNotEqual 6
#define pLess 7
#define pGreater 8
#define pLessEqual 9
#define pGreaterEqual 10
#define pColon 11
#define pComma 12
#define pLParen 13
#define pRParen 14
#define pLSquare 15
#define pRSquare 16
#define pCarat 17
#define pAt 18
#define pDot 19
#define pDotDot 20
#define pTimes 21
#define pDivide 22
#define pPlus 23
#define pMinus 24
#define pEof 25
#define pInvalid 26
#define pProgram 27
#define pProcedure 28
#define pFunction 29
#define pConst 30
#define pType 31
#define pVar 32
#define pBegin 33
#define pEnd 34
#define pArray 35
#define pRecord 36
#define pSet 37
#define pOf 38
#define pIf 39
#define pThen 40
#define pElse 41
#define pFor 42
#define pTo 43
#define pDownto 44
#define pDo 45
#define pWhile 46
#define pRepeat 47
#define pUntil 48
#define pContinue 49
#define pBreak 50
#define pAnd 51
#define pOr 52
#define pNot 53
#define pWriteln 54
#define pWrite 55
#define pReadln 56
#define pRead 57
#define pForward 58
#define pExternal 59
#define pName 60
#define pCdecl 61
#define tPushGlobalI 0
#define tPushGlobalB 1
#define tPushGlobalP 2
#define tPushLocalI 3
#define tPushLocalB 4
#define tPushLocalP 5
#define tPushParamI 6
#define tPushParamB 7
#define tPushParamP 8
#define tPushConstI 9
#define tPushAddrGlobal 10
#define tPushAddrLocal 11
#define tPushAddrParam 12
#define tPushAddrActual 13
#define tFetchI 14
#define tFetchB 15
#define tFetchP 16
#define tAssignI 17
#define tAssignB 18
#define tAssignP 19
#define tCopy 20
#define tIncI 21
#define tDecI 22
#define tMultI 23
#define tDivI 24
#define tAddPI 25
#define tAddI 26
#define tSubI 27
#define tNegI 28
#define tNot 29
#define tEqualI 30
#define tNotEqualI 31
#define tGreaterI 32
#define tLessI 33
#define tGreaterEqualI 34
#define tLessEqualI 35
#define tEqualP 36
#define tNotEqualP 37
#define tAllocActuals 38
#define tAllocActualsCdecl 39
#define tFreeActuals 40
#define tCall 41
#define tCallCdecl 42
#define tReturn 43
#define tEnter 44
#define tJump 45
#define tJumpTrue 46
#define tJumpFalse 47
#define tLabel 48
#define tLabelAlias 49
#define tLabelExtern 50
#define tWriteI 51
#define tWriteBool 52
#define tWriteStr 53
#define tWriteP 54
#define tWriteCR 55
#define tSpace 56
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
#define nINVALID 0
#define Object 1
#define nScope 2
#define nDeclaration 3
#define nIdent 4
#define nProgram 5
#define nMethod 6
#define nProc 7
#define nFunc 8
#define nConst 9
#define nTypeDecl 10
#define nVar 11
#define nGlobalVar 12
#define nLocalVar 13
#define nRecordField 14
#define nParam 15
#define nType 16
#define nFileType 17
#define nIntegerType 18
#define nBooleanType 19
#define nBooleanCFType 20
#define nCharType 21
#define nStringType 22
#define nPointerType 23
#define nArrayType 24
#define nSubrangeType 25
#define nRecordType 26
#define nEnumType 27
#define nSetType 28
#define qINVALID 0
#define qDecls 1
#define qSize 2
#define qAllocDown 3
#define qInitCode 4
#define qIdent 5
#define qType 6
#define qValue 7
#define qMainRoutineScope 8
#define qParams 9
#define qChildScope 10
#define qBodyDefined 11
#define qExternal 12
#define qExternalName 13
#define qCdecl 14
#define qCalled 15
#define qOldParams 16
#define qOldType 17
#define qResultOffset 18
#define qInOut 19
#define qPointerType 20
#define qBaseType 21
#define qIndexType 22
#define qLow 23
#define qHigh 24
#define qScope 25
#define Null 0
#define NullIter 0
#define NullVec 0
#define false 0
#define true 1
#define kUndefined 0
#define kProgram 1
#define kProc 2
#define kFunc 3
#define kConst 4
#define kType 5
#define kVar 6
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
#define oNodeNew 25
#define oNodeSet 26
#define oNodeSetString 27
#define oNodeSetInt 28
#define oNodeSetBoolean 29
#define oNodeSetKind 30
#define oNodeSetLabel 31
#define oNodeSetCode 32
#define oNodeGet 33
#define oNodeGetString 34
#define oNodeGetInt 35
#define oNodeGetBoolean 36
#define oNodeGetLabel 37
#define oNodeGetCode 38
#define oNodeNull 39
#define oNodeGetIter 40
#define oNodeIterValue 41
#define oNodeIterNext 42
#define oNodeType 43
#define oNodeEqual 44
#define oNodeVecNew 45
#define oNodeVecDelete 46
#define oNodeVecAppend 47
#define oNodeVecSize 48
#define oNodeVecElement 49
#define oEmitInt 50
#define oEmitLabel 51
#define oEmitCode 52
#define Here 53
#define oPatch 54
#define inc 55
#define dec 56
#define negate 57
#define subtract 58
#define multiply 59
#define equal 60
#define equal_zero 61
#define equal_node 62
#define equal_string 63
#define equal_node_type 64
#define equal_label 65
#define equal_code 66
#define oMININT 67
#define TOKEN_VALUE 68
#define LAST_ID 69
#define ID_STRING 70
#define CURRENT_STRLIT 71
#define oScopeBegin 72
#define oScopeEnter 73
#define oScopeEnd 74
#define oScopeCurrent 75
#define oScopeDeclare 76
#define oScopeDeclareAlloc 77
#define oScopeAllocType 78
#define oScopeFind 79
#define oScopeFindRequire 80
#define oScopeFindInCurrentScope 81
#define oTypeAdd 82
#define oTypeSPush 83
#define oTypeSPop 84
#define oTypeSTop 85
#define oTypeSNodeType 86
#define oIdAdd_File 87
#define oIdAdd_Integer 88
#define oIdAdd_Boolean 89
#define oIdAdd_Char 90
#define oIdAdd_String 91
#define oIdAdd_True 92
#define oIdAdd_False 93
#define oLabelNew 94
#define oCodeNew 95
#define oCodePush 96
#define oCodePop 97
#define oCountPush 98
#define oCountInc 99
#define oCountDec 100
#define oCountIsZero 101
#define oCountPop 102
#define oValuePush 103
#define oValueNegate 104
#define oValueTop 105
#define oValuePop 106
#define oStringAllocLit 107
#define oLoopPush 108
#define oLoopContinueLabel 109
#define oLoopBreakLabel 110
#define oLoopPop 111
#define oMsg 112
#define oMsgTrace 113
#define oMsgNode 114
#define oMsgNodeVec 115

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
   "", 0
};
int ssl_error_table_size = 23;

#endif // SSL_INCLUDE_ERR_TABLE
