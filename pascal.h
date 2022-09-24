#define SSL_CODE_TABLE_SIZE 5100

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
#define pDot 18
#define pDotDot 19
#define pTimes 20
#define pDivide 21
#define pPlus 22
#define pMinus 23
#define pEof 24
#define pInvalid 25
#define pProgram 26
#define pProcedure 27
#define pFunction 28
#define pConst 29
#define pType 30
#define pVar 31
#define pBegin 32
#define pEnd 33
#define pArray 34
#define pRecord 35
#define pSet 36
#define pOf 37
#define pIf 38
#define pThen 39
#define pElse 40
#define pFor 41
#define pTo 42
#define pDownto 43
#define pDo 44
#define pWhile 45
#define pRepeat 46
#define pUntil 47
#define pCycle 48
#define pExit 49
#define pReturn 50
#define pAnd 51
#define pOr 52
#define pNot 53
#define pWriteln 54
#define pWrite 55
#define pReadln 56
#define pRead 57
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
#define tWriteI 48
#define tWriteBool 49
#define tWriteStr 50
#define tWriteP 51
#define tWriteCR 52
#define tSpace 53
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
#define nCharType 20
#define nStringType 21
#define nPointerType 22
#define nArrayType 23
#define nSubrangeType 24
#define nRecordType 25
#define nEnumType 26
#define nSetType 27
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
#define oNodeNew 25
#define oNodeSet 26
#define oNodeSetInt 27
#define oNodeSetBoolean 28
#define oNodeSetKind 29
#define oNodeGet 30
#define oNodeGetInt 31
#define oNodeGetBoolean 32
#define oNodeNull 33
#define oNodeNext 34
#define oNodeType 35
#define oNodeEqual 36
#define oNodeVecNew 37
#define oNodeVecDelete 38
#define oNodeVecAppend 39
#define oNodeVecSize 40
#define oNodeVecElement 41
#define oEmitInt 42
#define Here 43
#define oPatch 44
#define inc 45
#define dec 46
#define negate 47
#define subtract 48
#define multiply 49
#define equal 50
#define equal_zero 51
#define equal_node_type 52
#define TOKEN_VALUE 53
#define LAST_ID 54
#define oScopeBegin 55
#define oScopeEnter 56
#define oScopeEnd 57
#define oScopeCurrent 58
#define oScopeDeclare 59
#define oScopeDeclareAlloc 60
#define oScopeAllocType 61
#define oScopeFind 62
#define oScopeFindRequire 63
#define oTypeAdd 64
#define oTypeSPush 65
#define oTypeSPop 66
#define oTypeSTop 67
#define oTypeSNodeType 68
#define oIdAdd_File 69
#define oIdAdd_Integer 70
#define oIdAdd_Boolean 71
#define oIdAdd_Char 72
#define oIdAdd_String 73
#define oIdAdd_True 74
#define oIdAdd_False 75
#define oCountPush 76
#define oCountInc 77
#define oCountDec 78
#define oCountIsZero 79
#define oCountPop 80
#define oValuePush 81
#define oValueNegate 82
#define oValueTop 83
#define oValuePop 84
#define oStringAllocLit 85
#define patchLoop 0
#define patchExit 1
#define patchIf 2
#define oPatchPushHere 86
#define oPatchAnyEntries 87
#define oPatchSwap 88
#define oPatchDup 89
#define oPatchPopFwd 90
#define oPatchPopBack 91

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
