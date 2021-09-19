#define SSL_CODE_TABLE_SIZE 400

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
#define pINVALID 0
#define pEOF 1
#define pIDENTIFIER 2
#define pBoolean1 3
#define pCharacter1 4
#define pInteger4 5
#define pInteger8 6
#define pIntegerN 7
#define pReal4 8
#define pReal8 9
#define pRealN 10
#define pStringN 11
#define pObjectType 12
#define pNode 13
#define pList 14
#define pPri 15
#define pAlt 16
#define pOpt 17
#define pInt 18
#define pExt 19
#define pSys 20
#define pTmp 21
#define pSchema 22
#define pRoot 23
#define pIs 24
#define pEnd 25
#define pDERIVES 26
#define pCONTAINS 27
#define pCOLON 28
#define pCOMMA 29
#define pLPAREN 30
#define pRPAREN 31
#define pLSQUARE 32
#define pRSQUARE 33
#define eNoClassForAttribute 0
#define nINVALID 0
#define Object 1
#define nSchema 2
#define nClass 3
#define nAttr 4
#define nConstraint 5
#define nAttrSym 6
#define T_qINVALID 0
#define T_qClassTree 1
#define T_qAttrSyms 2
#define T_qIdent 3
#define T_qText 4
#define T_qCode 5
#define T_qAttrs 6
#define T_qDerived 7
#define T_qAttrSym 8
#define T_qConstraint 9
#define T_qTags 10
#define T_qType 11
#define type_INVALID 0
#define type_Boolean1 1
#define type_Character1 2
#define type_Integer4 4
#define type_StringN 10
#define type_Node 100
#define type_List 150
#define tag_INVALID 0
#define tag_Pri 1
#define tag_Opt 2
#define tag_Alt 4
#define false 0
#define true 1
#define oCreateEmptySchema 25
#define oFindClass 26
#define oDeriveClass 27
#define oThisClassWillGetAttrs 28
#define oNoClassWillGetAttrs 29
#define oAClassWillGetAttrs 30
#define oCreateAttrSym 31
#define oCreateAttr 32
#define oAttrType 33
#define oAttrTag 34
#define oDocDumpTable 35

#ifdef SSL_INCLUDE_ERR_TABLE

struct ssl_error_table_struct ssl_error_table[] = {
   "eNoClassForAttribute", 0,
   "", 0
};
int ssl_error_table_size = 1;

#endif SSL_INCLUDE_ERR_TABLE
