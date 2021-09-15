#define SSL_CODE_TABLE_SIZE 174

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
#define pInvalid 0
#define pIdent 1
#define pIntLit 2
#define pStrLit 3
#define pEof 4
#define eErrorA 0
#define eErrorB 1
#define false 0
#define true 1
#define yes 1
#define no 0
#define oAdd 24
#define print 25
#define TOKEN_VAL 26

#ifdef SSL_INCLUDE_ERR_TABLE

struct ssl_error_table_struct ssl_error_table[] = {
   "eErrorA", 0,
   "eErrorB", 1,
   "", 0
};
int ssl_error_table_size = 2;

#endif SSL_INCLUDE_ERR_TABLE
