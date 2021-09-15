#define SSL_CODE_TABLE_SIZE 2130

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
#define iGlobalSpace 15
#define iLocalSpace 16
#define iGetParam 17
#define iGetFromParam 18
#define iGetLocal 19
#define iGetGlobal 20
#define iGetAddrParam 21
#define iGetAddrLocal 22
#define iGetAddrGlobal 23
#define iAssign 24
#define iSpace 25
#define iConstant 26
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
#define eUndeclRuleParamsNotSupported 18
#define eNotTyped 19
#define eIdentNotAllowedInExpr 20
#define eParamNotSupportedYet 21
#define eIllegalLvalue 22
#define eNotRuleOrGlobalDefn 23
#define eRuleBodyAlreadyDeclared 24
#define eReturnTypeMismatch 25
#define eParameterMismatch 26
#define eUndefinedIdentifier 27
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
#define qAddrDefined 8
#define qTypeDefined 9
#define qAddr 10
#define false 0
#define true 1
#define no 0
#define yes 1
#define zero 0
#define one 1
#define wRuleMissingAtSign 0
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
#define oWarning 14
#define oCountPush 15
#define oCountPushIntLit 16
#define oCountPushValue 17
#define oCountPop 18
#define oCountInc 19
#define oCountDec 20
#define oCountNegate 21
#define oCountZero 22
#define oNextErrorPushCount 23
#define oNextErrorPopCount 24
#define oValuePush 25
#define oValuePushBoolean 26
#define oValuePushKind 27
#define oValuePushNodeType 28
#define oValuePushIdent 29
#define oValuePushIntLit 30
#define oValueChooseKind 31
#define oValueChooseBoolean 32
#define oValuePushCount 33
#define oValuePushHere 34
#define oValueNegate 35
#define oValueSwap 36
#define oValueZero 37
#define oValueMatch 38
#define oValuePop 39
#define patchChoiceTableAddr 0
#define patchChoiceTable 1
#define patchChoiceExit 2
#define patchCall 3
#define patchLoop 4
#define patchBreak 5
#define oPatchMark 40
#define oPatchAtMark 41
#define oPatchPushHere 42
#define oPatchPushIdent 43
#define oPatchPushValue 44
#define oPatchAnyEntries 45
#define oPatchPopFwd 46
#define oPatchPopBack 47
#define oPatchPopValue 48
#define oPatchPopCall 49
#define oShortFormAdd 50
#define oShortFormLookup 51
#define oTitleSet 52
#define oDocNewRule 53
#define oDocCheckpoint 54
#define oInclude 55
#define oNodeNew 56
#define oNodeNewValue 57
#define oNodeLink 58
#define oNodeLinkUnder1 59
#define oNodeSetValue 60
#define oNodeSetIdent 61
#define oNodeAppend 62
#define oNodeAppendNode 63
#define oNodeGet 64
#define oNodeGetValue 65
#define oNodeNull 66
#define oNodePushNull 67
#define oNodeNext 68
#define oNodeIsA 69
#define oNodeChooseType 70
#define oNodeGetType 71
#define oNodeCompareExact 72
#define oNodeCompareExactUnder2 73
#define oNodeSwap 74
#define oNodePop 75
#define oNodeGetIntType 76
#define oRuleSetCurrentRule 77
#define oRuleGetCurrentRule 78
#define oRuleSetNumLocals 79
#define oRuleIncNumLocals 80
#define oRuleGetNumLocals 81
#define oRuleSetLocalSpaceAddr 82
#define oRulePatchLocalSpace 83
#define oSetNumGlobals 84
#define oIncNumGlobals 85
#define oGetNumGlobals 86
#define oSetGlobalSpaceAddr 87
#define oPatchGlobalSpace 88
#define oScopeBegin 89
#define oScopeOpen 90
#define oScopeEnd 91
#define oScopeDeclare 92
#define oScopeDeclareKeep 93
#define oScopeFind 94
#define oScopeFindRequire 95
#define oInstallSystemOperations 96
#define oInstallSystemTypes 97
#define oWriteTables 98

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
   "eUndeclRuleParamsNotSupported", 18,
   "eNotTyped", 19,
   "eIdentNotAllowedInExpr", 20,
   "eParamNotSupportedYet", 21,
   "eIllegalLvalue", 22,
   "eNotRuleOrGlobalDefn", 23,
   "eRuleBodyAlreadyDeclared", 24,
   "eReturnTypeMismatch", 25,
   "eParameterMismatch", 26,
   "eUndefinedIdentifier", 27,
   "", 0
};
int ssl_error_table_size = 28;

#endif SSL_INCLUDE_ERR_TABLE
