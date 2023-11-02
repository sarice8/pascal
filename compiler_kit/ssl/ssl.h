#define SSL_CODE_TABLE_SIZE 4500

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
#define eCantInitGlobalVarsYet 19
#define eRedeclaration 20
#define nINVALID 0
#define Object 1
#define nScope 2
#define nDeclaration 3
#define nIdent 4
#define nOutput 5
#define nError 6
#define nType 7
#define nValue 8
#define nMechanism 9
#define nOperation 10
#define nRule 11
#define nVariable 12
#define nParam 13
#define nInParam 14
#define nOutParam 15
#define nInOutParam 16
#define nLocal 17
#define nGlobal 18
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
#define wRuleMissingAtSign 0
#define wReturnValueIgnored 1
#define Null 0
#define NullIter 0
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
#define CollectDefinitionsPass 0
#define CodeGenerationPass 1
#define oResetInput 25
#define oStartListing 26
#define oWarning 27
#define oUndeclaredRule 28
#define oEmitInt 29
#define Here 30
#define oPatch 31
#define inc 32
#define dec 33
#define negate 34
#define equal_zero 35
#define equal_node_type 36
#define TOKEN_VALUE 37
#define LAST_ID 38
#define oShortFormAdd 39
#define oShortFormLookup 40
#define patchChoiceTableAddr 0
#define patchChoiceTable 1
#define patchChoiceExit 2
#define patchCall 3
#define patchLoop 4
#define patchBreak 5
#define oPatchMark 41
#define oPatchAtMark 42
#define oPatchPushHere 43
#define oPatchPushInt 44
#define oPatchPushIdent 45
#define oPatchAnyEntries 46
#define oPatchPopFwd 47
#define oPatchPopBack 48
#define oPatchPopValue 49
#define oPatchPopCall 50
#define oTitleSet 51
#define oDocNewRule 52
#define oInclude 53
#define oNodeNew 54
#define oNodeSet 55
#define oNodeSetInt 56
#define oNodeSetBoolean 57
#define oNodeGet 58
#define oNodeGetInt 59
#define oNodeGetBoolean 60
#define oNodeNull 61
#define oNodeGetIter 62
#define oNodeIterValue 63
#define oNodeIterNext 64
#define oNodeIterPrev 65
#define oNodeType 66
#define oNodeEqual 67
#define oScopeBegin 68
#define oScopeOpen 69
#define oScopeEnd 70
#define oScopeDeclare 71
#define oScopeFind 72
#define oScopeFindInCurrentScope 73
#define oScopeFindRequire 74
#define oInstallSystemOperations 75
#define oInstallSystemTypes 76
#define oWriteTables 77

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
   "eCantInitGlobalVarsYet", 19,
   "eRedeclaration", 20,
   "", 0
};
int ssl_error_table_size = 21;

#endif // SSL_INCLUDE_ERR_TABLE
