#define SSL_CODE_TABLE_SIZE 4600

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
#define tPushConstP 10
#define tPushAddrGlobal 11
#define tPushAddrLocal 12
#define tPushAddrParam 13
#define tPushAddrActual 14
#define tFetchI 15
#define tFetchB 16
#define tFetchP 17
#define tAssignI 18
#define tAssignB 19
#define tAssignP 20
#define tCopy 21
#define tIncGlobalI 22
#define tDecGlobalI 23
#define tMultI 24
#define tDivI 25
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
#define qParentScope 1
#define qDecls 2
#define qNextOffset 3
#define qIdent 4
#define qKind 5
#define qType 6
#define qParamType 7
#define qValue 8
#define qParams 9
#define qChildScope 10
#define qAddrDefined 11
#define qInOut 12
#define qSize 13
#define qPointerType 14
#define qBaseType 15
#define qIndexType 16
#define qLow 17
#define qHigh 18
#define qScope 19
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
#define oScopeFind 61
#define oScopeFindRequire 62
#define oTypeAdd 63
#define oTypeSPush 64
#define oTypeSPop 65
#define oTypeSTop 66
#define oTypeSNodeType 67
#define oIdAdd_File 68
#define oIdAdd_Integer 69
#define oIdAdd_Boolean 70
#define oIdAdd_Char 71
#define oIdAdd_String 72
#define oIdAdd_True 73
#define oIdAdd_False 74
#define oCountPush 75
#define oCountInc 76
#define oCountDec 77
#define oCountIsZero 78
#define oCountPop 79
#define oValuePush 80
#define oValueNegate 81
#define oValueTop 82
#define oValuePop 83
#define oStringAllocLit 84
#define patchLoop 0
#define patchExit 1
#define patchIf 2
#define oPatchPushHere 85
#define oPatchAnyEntries 86
#define oPatchSwap 87
#define oPatchDup 88
#define oPatchPopFwd 89
#define oPatchPopBack 90

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
   "", 0
};
int ssl_error_table_size = 20;

#endif // SSL_INCLUDE_ERR_TABLE
