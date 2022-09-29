#define SSL_CODE_TABLE_SIZE 5700

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
#define pCycle 49
#define pExit 50
#define pReturn 51
#define pAnd 52
#define pOr 53
#define pNot 54
#define pWriteln 55
#define pWrite 56
#define pReadln 57
#define pRead 58
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
#define tAnd 30
#define tOr 31
#define tEqualI 32
#define tNotEqualI 33
#define tGreaterI 34
#define tLessI 35
#define tGreaterEqualI 36
#define tLessEqualI 37
#define tEqualP 38
#define tNotEqualP 39
#define tAllocActuals 40
#define tFreeActuals 41
#define tCall 42
#define tReturn 43
#define tEnter 44
#define tJump 45
#define tJumpTrue 46
#define tJumpFalse 47
#define tLabel 48
#define tLabelAlias 49
#define tWriteI 50
#define tWriteBool 51
#define tWriteStr 52
#define tWriteP 53
#define tWriteCR 54
#define tSpace 55
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
#define qIdent 4
#define qType 5
#define qValue 6
#define qMainRoutineScope 7
#define qParams 8
#define qChildScope 9
#define qAddrDefined 10
#define qResultOffset 11
#define qInOut 12
#define qPointerType 13
#define qBaseType 14
#define qIndexType 15
#define qLow 16
#define qHigh 17
#define qScope 18
#define Null 0
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
#define oNodeNew 25
#define oNodeSet 26
#define oNodeSetInt 27
#define oNodeSetBoolean 28
#define oNodeSetKind 29
#define oNodeSetLabel 30
#define oNodeGet 31
#define oNodeGetInt 32
#define oNodeGetBoolean 33
#define oNodeNull 34
#define oNodeNext 35
#define oNodeType 36
#define oNodeEqual 37
#define oNodeVecNew 38
#define oNodeVecDelete 39
#define oNodeVecAppend 40
#define oNodeVecSize 41
#define oNodeVecElement 42
#define oEmitInt 43
#define oEmitLabel 44
#define Here 45
#define oPatch 46
#define inc 47
#define dec 48
#define negate 49
#define subtract 50
#define multiply 51
#define equal 52
#define equal_zero 53
#define equal_node_type 54
#define equal_label 55
#define TOKEN_VALUE 56
#define LAST_ID 57
#define oScopeBegin 58
#define oScopeEnter 59
#define oScopeEnd 60
#define oScopeCurrent 61
#define oScopeDeclare 62
#define oScopeDeclareAlloc 63
#define oScopeAllocType 64
#define oScopeFind 65
#define oScopeFindRequire 66
#define oTypeAdd 67
#define oTypeSPush 68
#define oTypeSPop 69
#define oTypeSTop 70
#define oTypeSNodeType 71
#define oIdAdd_File 72
#define oIdAdd_Integer 73
#define oIdAdd_Boolean 74
#define oIdAdd_Char 75
#define oIdAdd_String 76
#define oIdAdd_True 77
#define oIdAdd_False 78
#define oLabelNew 79
#define oCountPush 80
#define oCountInc 81
#define oCountDec 82
#define oCountIsZero 83
#define oCountPop 84
#define oValuePush 85
#define oValueNegate 86
#define oValueTop 87
#define oValuePop 88
#define oStringAllocLit 89
#define oLoopPush 90
#define oLoopCycleLabel 91
#define oLoopExitLabel 92
#define oLoopPop 93
#define oMsg 94
#define oMsgTrace 95

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
   "", 0
};
int ssl_error_table_size = 21;

#endif // SSL_INCLUDE_ERR_TABLE
