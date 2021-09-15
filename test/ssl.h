#define SSL_CODE_TABLE_SIZE 2265

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
#define oLocalSpace 15
#define oGetParam 16
#define oGetFromParam 17
#define oGetLocal 18
#define oGetGlobal 19
#define oGetAddrParam 20
#define oGetAddrLocal 21
#define oGetAddrGlobal 22
#define oAssign 23
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
#define pIn 33
#define pOut 34
#define pInOut 35
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
#define iPushResult 12
#define iPop 13
#define iBreak 14
#define iLocalSpace 15
#define iGetParam 16
#define iGetFromParam 17
#define iGetLocal 18
#define iGetGlobal 19
#define iGetAddrParam 20
#define iGetAddrLocal 21
#define iGetAddrGlobal 22
#define iAssign 23
#define iSpace 24
#define iConstant 25
#define eMissingProgramBlock 0
#define eMissingEnd 1
#define eWrongType 2
#define eNoShortFormHere 3
#define eNotAType 4
#define eNotAValue 5
#define eNotInput 6
#define eNotOutput 7
#define eNotARule 8
#define eNotAStatement 9
#define eUndeclaredIdent 10
#define eBadStatement 11
#define eNotAnErrSig 12
#define eNotInLoop 13
#define eNotChoice 14
#define eChoiceOpRuleOutOfPlace 15
#define eUnexpectedKind 16
#define eWrongNumberParams 17
#define eFwdRuleParamsNotSupportedYet 18
#define eNotTyped 19
#define eIdentNotAllowedInExpr 20
#define eParamNotSupportedYet 21
#define eIllegalLvalue 22
#define eNotRuleOrGlobalDefn 23
#define nINVALID 0
#define Object 1
#define nScope 2
#define nDeclaration 3
#define nIdent 4
#define nInput 5
#define nOutput 6
#define nError 7
#define nType 8
#define nValue 9
#define nMechanism 10
#define nOperation 11
#define nRule 12
#define nVariable 13
#define nParam 14
#define nInParam 15
#define nOutParam 16
#define nInOutParam 17
#define nLocal 18
#define nGlobal 19
#define qINVALID 0
#define qParentScope 1
#define qDecls 2
#define qIdent 3
#define qValue 4
#define qType 5
#define qParamScope 6
#define qScope 7
#define qAddr 8
#define false 0
#define true 1
#define zero 0
#define one 1
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
#define kVar 10
#define oCountPush 24
#define oCountPushIntLit 25
#define oCountPushValue 26
#define oCountPop 27
#define oCountInc 28
#define oCountDec 29
#define oCountNegate 30
#define oCountZero 31
#define oNextErrorPushCount 32
#define oNextErrorPopCount 33
#define oValuePush 34
#define oValuePushKind 35
#define oValuePushNodeType 36
#define oValuePushIdent 37
#define oValuePushIntLit 38
#define oValueChooseKind 39
#define oValuePushCount 40
#define oValuePushHere 41
#define oValueNegate 42
#define oValueSwap 43
#define oValueZero 44
#define oValuePop 45
#define patchChoiceTableAddr 0
#define patchChoiceTable 1
#define patchChoiceExit 2
#define patchCall 3
#define patchLoop 4
#define patchBreak 5
#define oPatchMark 46
#define oPatchAtMark 47
#define oPatchPushHere 48
#define oPatchPushIdent 49
#define oPatchPushValue 50
#define oPatchAnyEntries 51
#define oPatchPopFwd 52
#define oPatchPopBack 53
#define oPatchPopValue 54
#define oPatchPopCall 55
#define oShortFormAdd 56
#define oShortFormLookup 57
#define oTitleSet 58
#define oDocNewRule 59
#define oDocCheckpoint 60
#define oInclude 61
#define oNodeNew 62
#define oNodeNewValue 63
#define oNodeLink 64
#define oNodeLinkUnder1 65
#define oNodeSetValue 66
#define oNodeSetIdent 67
#define oNodeAppend 68
#define oNodeAppendNode 69
#define oNodeGet 70
#define oNodeGetValue 71
#define oNodeNull 72
#define oNodePushNull 73
#define oNodeNext 74
#define oNodeIsA 75
#define oNodeChooseType 76
#define oNodeCompareExact 77
#define oNodeCompareExactUnder2 78
#define oNodeSwap 79
#define oNodePop 80
#define oNodeGetIntType 81
#define oRuleSetCurrentRule 82
#define oRuleGetCurrentRule 83
#define oRuleSetNumLocals 84
#define oRuleIncNumLocals 85
#define oRuleGetNumLocals 86
#define oRuleSetLocalSpaceAddr 87
#define oRulePatchLocalSpace 88
#define oSetNumGlobals 89
#define oIncNumGlobals 90
#define oGetNumGlobals 91
#define oSetGlobalSpaceAddr 92
#define oPatchGlobalSpace 93
#define oScopeBegin 94
#define oScopeExtend 95
#define oScopeEnd 96
#define oScopeDeclare 97
#define oScopeDeclareKeep 98
#define oScopeFind 99
#define oScopeFindRequire 100
#define oInstallSystemOperations 101
#define oInstallSystemTypes 102
#define oWriteTables 103
#define Here 104
#define patch 105
#define oNodeSetInt 106
#define inc 107

#ifdef SSL_INCLUDE_ERR_TABLE

struct ssl_error_table_struct ssl_error_table[] = {
   "eMissingProgramBlock", 0,
   "eMissingEnd", 1,
   "eWrongType", 2,
   "eNoShortFormHere", 3,
   "eNotAType", 4,
   "eNotAValue", 5,
   "eNotInput", 6,
   "eNotOutput", 7,
   "eNotARule", 8,
   "eNotAStatement", 9,
   "eUndeclaredIdent", 10,
   "eBadStatement", 11,
   "eNotAnErrSig", 12,
   "eNotInLoop", 13,
   "eNotChoice", 14,
   "eChoiceOpRuleOutOfPlace", 15,
   "eUnexpectedKind", 16,
   "eWrongNumberParams", 17,
   "eFwdRuleParamsNotSupportedYet", 18,
   "eNotTyped", 19,
   "eIdentNotAllowedInExpr", 20,
   "eParamNotSupportedYet", 21,
   "eIllegalLvalue", 22,
   "eNotRuleOrGlobalDefn", 23,
   "", 0
};
int ssl_error_table_size = 24;

#endif SSL_INCLUDE_ERR_TABLE
