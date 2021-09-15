#define SSL_CODE_TABLE_SIZE 3875

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
#define eWrongType 0
#define eNotAType 1
#define eNotAValue 2
#define eNotInput 3
#define eNotOutput 4
#define eNotARule 5
#define eNotAnErrSig 6
#define eNotInLoop 7
#define eChoiceOpRuleOutOfPlace 8
#define eUndeclRuleParamsNotSupported 9
#define eNotTyped 10
#define eIdentNotAllowedInExpr 11
#define eIllegalLvalue 12
#define eNotRuleOrGlobalDefn 13
#define eRuleBodyAlreadyDeclared 14
#define eReturnTypeMismatch 15
#define eParameterMismatch 16
#define eUndeclaredIdentifier 17
#define eAliasNotAllowed 18
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
#define wReturnValueIgnored 1
#define Null 0
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
#define oWarning 25
#define oEmitInt 26
#define Here 27
#define oPatch 28
#define inc 29
#define dec 30
#define negate 31
#define equal_zero 32
#define equal_node_type 33
#define TOKEN_VALUE 34
#define LAST_ID 35
#define oShortFormAdd 36
#define oShortFormLookup 37
#define patchChoiceTableAddr 0
#define patchChoiceTable 1
#define patchChoiceExit 2
#define patchCall 3
#define patchLoop 4
#define patchBreak 5
#define oPatchMark 38
#define oPatchAtMark 39
#define oPatchPushHere 40
#define oPatchPushInt 41
#define oPatchPushIdent 42
#define oPatchAnyEntries 43
#define oPatchPopFwd 44
#define oPatchPopBack 45
#define oPatchPopValue 46
#define oPatchPopCall 47
#define oTitleSet 48
#define oDocNewRule 49
#define oInclude 50
#define oNodeNew 51
#define oNodeSet 52
#define oNodeSetInt 53
#define oNodeSetBoolean 54
#define oNodeGet 55
#define oNodeGetInt 56
#define oNodeGetBoolean 57
#define oNodeNull 58
#define oNodeNext 59
#define oNodeType 60
#define oNodeEqual 61
#define oScopeBegin 62
#define oScopeOpen 63
#define oScopeEnd 64
#define oScopeDeclare 65
#define oScopeFind 66
#define oScopeFindRequire 67
#define oInstallSystemOperations 68
#define oInstallSystemTypes 69
#define oWriteTables 70

#ifdef SSL_INCLUDE_ERR_TABLE

struct ssl_error_table_struct ssl_error_table[] = {
   "eWrongType", 0,
   "eNotAType", 1,
   "eNotAValue", 2,
   "eNotInput", 3,
   "eNotOutput", 4,
   "eNotARule", 5,
   "eNotAnErrSig", 6,
   "eNotInLoop", 7,
   "eChoiceOpRuleOutOfPlace", 8,
   "eUndeclRuleParamsNotSupported", 9,
   "eNotTyped", 10,
   "eIdentNotAllowedInExpr", 11,
   "eIllegalLvalue", 12,
   "eNotRuleOrGlobalDefn", 13,
   "eRuleBodyAlreadyDeclared", 14,
   "eReturnTypeMismatch", 15,
   "eParameterMismatch", 16,
   "eUndeclaredIdentifier", 17,
   "eAliasNotAllowed", 18,
   "", 0
};
int ssl_error_table_size = 19;

#endif SSL_INCLUDE_ERR_TABLE
