#define w_codeTableSize 1088

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
#define pEquals 3
#define pColon 4
#define pSemiColon 5
#define pComma 6
#define pLParen 7
#define pRParen 8
#define pReturn 9
#define pBreak 10
#define pLCurly 11
#define pRCurly 12
#define pLSquare 13
#define pRSquare 14
#define pBar 15
#define pCall 16
#define pEmit 17
#define pStar 18
#define pErr 19
#define pQuestion 20
#define pEof 21
#define pInvalid 22
#define pTitle 23
#define pInput 24
#define pOutput 25
#define pType 26
#define pError 27
#define pMechanism 28
#define pInclude 29
#define pRules 30
#define pEnd 31
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
#define oCountZero 19
#define oNextErrorPushCount 20
#define oNextErrorPopCount 21
#define oValuePushKind 22
#define oValuePushVal 23
#define oValuePushISVal 24
#define oValuePushIdent 25
#define oValuePushType 26
#define oValueChooseKind 27
#define oValuePushCount 28
#define oValueSwap 29
#define oValuePop 30
#define patchChoiceTableAddr 0
#define patchChoiceTable 1
#define patchChoiceExit 2
#define patchCall 3
#define patchLoop 4
#define patchBreak 5
#define oPatchMark 31
#define oPatchAtMark 32
#define oPatchPushHere 33
#define oPatchPushIdent 34
#define oPatchPushValue 35
#define oPatchAnyEntries 36
#define oPatchPopFwd 37
#define oPatchPopBack 38
#define oPatchPopValue 39
#define oPatchPopCall 40
#define oShortFormAdd 41
#define oShortFormLookup 42
#define oIdentSetDeclared 43
#define oIdentSetKind 44
#define oIdentSetKindVS 45
#define oIdentSetType 46
#define oIdentSetValCount 47
#define oIdentSetValHere 48
#define oIdentSetChoice 49
#define oIdentChooseKind 50
#define oIdentChooseParam 51
#define oIdentChooseChoice 52
#define oIdentChooseDeclared 53
#define oIdentMatchType 54
#define oIdentMatchParamType 55
#define oIdentISPush 56
#define oIdentISPushBottom 57
#define oIdentISPop 58
#define oIdentSetISParamType 59
#define oIdentSetISParam 60
#define oIdentSetISChoice 61
#define oIdentSetISType 62
#define oIdentChooseISKind 63
#define oIdentChooseISChoice 64
#define oIdentChooseISParam 65
#define oIdentMatchISType 66
#define oTitleSet 67
#define oDocNewRule 68
#define oDocCheckpoint 69
#define oInclude 70

#ifdef SSL_INCLUDE_ERR_TABLE

struct w_errTableType {
  char *msg;
  short val;
} w_errTable[] = {
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
#define w_errTableSize 17

#endif SSL_INCLUDE_ERR_TABLE
