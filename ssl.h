#define w_codeTableSize 1067

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
#define pRules 29
#define pEnd 30
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
#define numberSystemOps 13
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
#define oCountPush 13
#define oCountPushIntLit 14
#define oCountPop 15
#define oCountInc 16
#define oCountDec 17
#define oCountZero 18
#define oValuePushKind 19
#define oValuePushVal 20
#define oValuePushISVal 21
#define oValuePushIdent 22
#define oValuePushType 23
#define oValueChooseKind 24
#define oValuePushCount 25
#define oValueSwap 26
#define oValuePop 27
#define patchChoiceTableAddr 0
#define patchChoiceTable 1
#define patchChoiceExit 2
#define patchCall 3
#define patchLoop 4
#define patchBreak 5
#define oPatchMark 28
#define oPatchAtMark 29
#define oPatchPushHere 30
#define oPatchPushIdent 31
#define oPatchPushValue 32
#define oPatchAnyEntries 33
#define oPatchPopFwd 34
#define oPatchPopBack 35
#define oPatchPopValue 36
#define oPatchPopCall 37
#define oShortFormAdd 38
#define oShortFormLookup 39
#define oIdentSetDeclared 40
#define oIdentSetKind 41
#define oIdentSetKindVS 42
#define oIdentSetType 43
#define oIdentSetValCount 44
#define oIdentSetValHere 45
#define oIdentSetChoice 46
#define oIdentChooseKind 47
#define oIdentChooseParam 48
#define oIdentChooseChoice 49
#define oIdentChooseDeclared 50
#define oIdentMatchType 51
#define oIdentMatchParamType 52
#define oIdentISPush 53
#define oIdentISPushBottom 54
#define oIdentISPop 55
#define oIdentSetISParamType 56
#define oIdentSetISParam 57
#define oIdentSetISChoice 58
#define oIdentSetISType 59
#define oIdentChooseISKind 60
#define oIdentChooseISChoice 61
#define oIdentChooseISParam 62
#define oIdentMatchISType 63
#define oTitleSet 64
#define oDocNewRule 65
#define oDocCheckpoint 66

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
