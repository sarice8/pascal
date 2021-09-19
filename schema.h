#define w_codeTableSize 210

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
#define nINVALID 0
#define Object 1
#define nSchema 2
#define nClass 3
#define nAttr 4
#define nConstraint 5
#define nAttrSym 6
#define qINVALID 0
#define qClassTree 1
#define qAttrSyms 2
#define qIdent 3
#define qCode 4
#define qAttrs 5
#define qDerived 6
#define qAttrSym 7
#define qType 8
#define qConstraint 9
#define qTags 10
#define type_INVALID 0
#define type_Integer4 4
#define type_Node 100
#define type_List 150
#define tag_INVALID 0
#define tag_Pri 1
#define tag_Opt 2
#define tag_Alt 4
#define oCreateEmptySchema 14
#define oFindClass 15
#define oDeriveClass 16
#define oThisClassWillGetAttrs 17
#define oNoClassWillGetAttrs 18
#define oCreateAttrSym 19
#define oCreateAttr 20
#define oAttrType 21
#define oAttrTag 22
#define oDocDumpTable 23

#ifdef SSL_INCLUDE_ERR_TABLE

struct w_errTableType {
  char *msg;
  short val;
} w_errTable[] = {
   "", 0
};
#define w_errTableSize 0

#endif SSL_INCLUDE_ERR_TABLE
