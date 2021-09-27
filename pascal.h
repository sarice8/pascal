#define SSL_CODE_TABLE_SIZE 1981

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
#define oSetParameter 12
#define oBreak 13
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
#define oCountPush 14
#define oCountInc 15
#define oCountDec 16
#define oCountIsZero 17
#define oCountPop 18
#define oSymPushLevel 19
#define oSymPopLevel 20
#define oSymPopLevelSaveType 21
#define oSymLookup 22
#define oSymChooseKind 23
#define oSymAddSPop 24
#define oSymLevelAnySyms 25
#define oSymSPushId 26
#define oSymSSetKind 27
#define oSymSSetValPop 28
#define oSymSSetTypS 29
#define oSymSSetParamTypS 30
#define oSymSAllocate 31
#define oValuePushToken 32
#define oValuePushVal 33
#define oValuePush 34
#define oValuePushSizeTS 35
#define oValuePushLowTS 36
#define oValueNegate 37
#define oValueDifference 38
#define oValueMultiply 39
#define oValueIsZero 40
#define oValueIsOne 41
#define oValuePop 42
#define oStringAllocLit 43
#define oTypSPushTyp 44
#define oTypSPush 45
#define oTypSPopPushBase 46
#define oTypSPopPushPtr 47
#define oTypSMatch 48
#define oTypSChoose 49
#define oTypSChoosePop 50
#define oTypSChooseKind 51
#define oTypSChoosePtr 52
#define oTypSSwap 53
#define oTypSPop 54
#define oTypNew 55
#define oTypSetLow 56
#define oTypSetHigh 57
#define oTypSetSize 58
#define oTypAssignBasePop 59
#define oTypAssignPtr 60
#define patchLoop 0
#define patchExit 1
#define patchIf 2
#define oPatchPushHere 61
#define oPatchAnyEntries 62
#define oPatchSwap 63
#define oPatchDup 64
#define oPatchPopFwd 65
#define oPatchPopBack 66

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
