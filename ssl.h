#define SSL_CODE_TABLE_SIZE 1110

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
#define pMinus 3
#define pEquals 4
#define pColon 5
#define pSemiColon 6
#define pComma 7
#define pLParen 8
#define pRParen 9
#define pReturn 10
#define pBreak 11
#define pLCurly 12
#define pRCurly 13
#define pLSquare 14
#define pRSquare 15
#define pBar 16
#define pCall 17
#define pEmit 18
#define pStar 19
#define pErr 20
#define pQuestion 21
#define pEof 22
#define pInvalid 23
#define pTitle 24
#define pInput 25
#define pOutput 26
#define pType 27
#define pError 28
#define pMechanism 29
#define pInclude 30
#define pRules 31
#define pEnd 32
#define iJumpForward 0
#define iJumpBack 1
#define iInput 2
#define iInputAny 3
#define iEmit 4
#define iError 5
#define iInputChoice 6
#define iCall 7
#define iReturn 8
#define iSetResult 9
#define iChoice 10
#define iEndChoice 11
#define iSetParameter 12
#define iSpace 13
#define iConstant 14
#define iIdentVal 15
#define iIdentISVal 16
#define eMissingProgramBlock 0
#define eMissingEnd 1
#define eWrongType 2
#define eNoShortFormHere 3
#define eNotAType 4
#define eNotAVal 5
#define eNotInput 6
#define eNotOutput 7
#define eNotARule 8
#define eNotOpRule 9
#define eUndeclaredIdent 10
#define eBadStatement 11
#define eNotAnErrSig 12
#define eNotInLoop 13
#define eNotChoice 14
#define eChoiceRuleOutOfPlace 15
#define eChoiceOpOutOfPlace 16
#define false 0
#define true 1
#define zero 0
#define one 1
#define numberSystemOps 14
#define kIllegal 0
#define kUnknown 1
#define kInput 2
#define kOutput 3
#define kError 4
#define kType 5
#define kVal 6
#define kMech 7
#define kOp 8
#define kRule 9
#define oCountPush 14
#define oCountPushIntLit 15
#define oCountPop 16
#define oCountInc 17
#define oCountDec 18
#define oCountNegate 19
#define oCountZero 20
#define oNextErrorPushCount 21
#define oNextErrorPopCount 22
#define oValuePushKind 23
#define oValuePushVal 24
#define oValuePushISVal 25
#define oValuePushIdent 26
#define oValuePushType 27
#define oValueChooseKind 28
#define oValuePushCount 29
#define oValueSwap 30
#define oValuePop 31
#define patchChoiceTableAddr 0
#define patchChoiceTable 1
#define patchChoiceExit 2
#define patchCall 3
#define patchLoop 4
#define patchBreak 5
#define oPatchMark 32
#define oPatchAtMark 33
#define oPatchPushHere 34
#define oPatchPushIdent 35
#define oPatchPushValue 36
#define oPatchAnyEntries 37
#define oPatchPopFwd 38
#define oPatchPopBack 39
#define oPatchPopValue 40
#define oPatchPopCall 41
#define oShortFormAdd 42
#define oShortFormLookup 43
#define oIdentSetDeclared 44
#define oIdentSetKind 45
#define oIdentSetKindVS 46
#define oIdentSetType 47
#define oIdentSetValCount 48
#define oIdentSetValHere 49
#define oIdentSetChoice 50
#define oIdentChooseKind 51
#define oIdentChooseParam 52
#define oIdentChooseChoice 53
#define oIdentChooseDeclared 54
#define oIdentMatchType 55
#define oIdentMatchParamType 56
#define oIdentISPush 57
#define oIdentISPushBottom 58
#define oIdentISPop 59
#define oIdentSetISParamType 60
#define oIdentSetISParam 61
#define oIdentSetISChoice 62
#define oIdentSetISType 63
#define oIdentChooseISKind 64
#define oIdentChooseISChoice 65
#define oIdentChooseISParam 66
#define oIdentMatchISType 67
#define oTitleSet 68
#define oDocNewRule 69
#define oDocCheckpoint 70
#define oInclude 71

#ifdef SSL_INCLUDE_ERR_TABLE

struct ssl_error_table_struct ssl_error_table[] = {
   "eMissingProgramBlock", 0,
   "eMissingEnd", 1,
   "eWrongType", 2,
   "eNoShortFormHere", 3,
   "eNotAType", 4,
   "eNotAVal", 5,
   "eNotInput", 6,
   "eNotOutput", 7,
   "eNotARule", 8,
   "eNotOpRule", 9,
   "eUndeclaredIdent", 10,
   "eBadStatement", 11,
   "eNotAnErrSig", 12,
   "eNotInLoop", 13,
   "eNotChoice", 14,
   "eChoiceRuleOutOfPlace", 15,
   "eChoiceOpOutOfPlace", 16,
   "", 0
};
int ssl_error_table_size = 17;

#endif SSL_INCLUDE_ERR_TABLE
