#define w_codeTableSize 213

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
#define pINVALID 0
#define pEOF 1
#define pIDENTIFIER 2
#define pInteger4 3
#define pNode 4
#define pList 5
#define pPri 6
#define pAlt 7
#define pOpt 8
#define pSchema 9
#define pRoot 10
#define pIs 11
#define pEnd 12
#define pDERIVES 13
#define pCONTAINS 14
#define pCOLON 15
#define pCOMMA 16
#define pLPAREN 17
#define pRPAREN 18
#define pLSQUARE 19
#define pRSQUARE 20
#define acINVALID 0
#define acInteger4 4
#define acNode 100
#define acList 150
#define atINVALID 0
#define atPri 1
#define atOpt 2
#define atAlt 4
#define oTagPush 13
#define oNSPush 14
#define oObjectDefine 15
#define oObjectDefineNS 16
#define oObjectDerive 17
#define oObjectPop 18
#define oAttrDefineNS 19
#define oAttrClass 20
#define oDocDumpTable 21

#ifdef SSL_INCLUDE_ERR_TABLE

struct w_errTableType {
  char *msg;
  short val;
} w_errTable[] = {
   "", 0
};
#define w_errTableSize 0

#endif SSL_INCLUDE_ERR_TABLE
