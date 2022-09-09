#define SSL_CODE_TABLE_SIZE 2400

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
#define tPushIntVar 0
#define tPushBoolVar 1
#define tPushPtrVar 2
#define tPushAddr 3
#define tFetchInt 4
#define tFetchBool 5
#define tFetchPtr 6
#define tPushIntLit 7
#define tAssignInt 8
#define tAssignBool 9
#define tAssignPtr 10
#define tCopy 11
#define tIncIntVar 12
#define tDecIntVar 13
#define tMultiplyInt 14
#define tDivideInt 15
#define tAddInt 16
#define tSubtractInt 17
#define tNegateInt 18
#define tNot 19
#define tAnd 20
#define tOr 21
#define tEqualInt 22
#define tNotEqualInt 23
#define tGreaterInt 24
#define tLessInt 25
#define tGreaterEqualInt 26
#define tLessEqualInt 27
#define tEqualPtr 28
#define tNotEqualPtr 29
#define tCall 30
#define tReturn 31
#define tJump 32
#define tJumpTrue 33
#define tJumpFalse 34
#define tPutInt 35
#define tPutBool 36
#define tPutStr 37
#define tPutPtr 38
#define tPutCR 39
#define tSpace 40
#define tConstant 41
#define tSymVal 42
#define eBadStatement 0
#define eNotConst 1
#define eNotType 2
#define eNotVar 3
#define eNotIntVar 4
#define eNotValue 5
#define eNotInteger 6
#define eNotBoolean 7
#define eNotPointer 8
#define eNotArray 9
#define eTooManySubscripts 10
#define eTypeMismatch 11
#define eNotImplemented 12
#define eNotAllowed 13
#define eNotInALoop 14
#define eRecordEmpty 15
#define nINVALID 0
#define Object 1
#define nScope 2
#define qINVALID 0
#define qParentScope 1
#define Null 0
#define false 0
#define true 1
#define zero 0
#define one 1
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
#define oCountPush 25
#define oCountInc 26
#define oCountDec 27
#define oCountIsZero 28
#define oCountPop 29
#define oSymPushLevel 30
#define oSymPopLevel 31
#define oSymPopLevelSaveType 32
#define oSymLookup 33
#define oSymChooseKind 34
#define oSymAddSPop 35
#define oSymLevelAnySyms 36
#define oSymSPushId 37
#define oSymSSetKind 38
#define oSymSSetValPop 39
#define oSymSSetTypS 40
#define oSymSSetParamTypS 41
#define oSymSAllocate 42
#define oValuePushToken 43
#define oValuePushVal 44
#define oValuePush 45
#define oValuePushSizeTS 46
#define oValuePushLowTS 47
#define oValueNegate 48
#define oValueDifference 49
#define oValueMultiply 50
#define oValueIsZero 51
#define oValueIsOne 52
#define oValuePop 53
#define oStringAllocLit 54
#define oTypSPushTyp 55
#define oTypSPush 56
#define oTypSPopPushBase 57
#define oTypSPopPushPtr 58
#define oTypSMatch 59
#define oTypSChoose 60
#define oTypSChoosePop 61
#define oTypSChooseKind 62
#define oTypSChoosePtr 63
#define oTypSSwap 64
#define oTypSPop 65
#define oTypNew 66
#define oTypSetLow 67
#define oTypSetHigh 68
#define oTypSetSize 69
#define oTypAssignBasePop 70
#define oTypAssignPtr 71
#define patchLoop 0
#define patchExit 1
#define patchIf 2
#define oPatchPushHere 72
#define oPatchAnyEntries 73
#define oPatchSwap 74
#define oPatchDup 75
#define oPatchPopFwd 76
#define oPatchPopBack 77

#ifdef SSL_INCLUDE_ERR_TABLE

struct ssl_error_table_struct ssl_error_table[] = {
   "eBadStatement", 0,
   "eNotConst", 1,
   "eNotType", 2,
   "eNotVar", 3,
   "eNotIntVar", 4,
   "eNotValue", 5,
   "eNotInteger", 6,
   "eNotBoolean", 7,
   "eNotPointer", 8,
   "eNotArray", 9,
   "eTooManySubscripts", 10,
   "eTypeMismatch", 11,
   "eNotImplemented", 12,
   "eNotAllowed", 13,
   "eNotInALoop", 14,
   "eRecordEmpty", 15,
   "", 0
};
int ssl_error_table_size = 16;

#endif // SSL_INCLUDE_ERR_TABLE
