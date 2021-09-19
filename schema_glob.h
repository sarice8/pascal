
/*  vglob.h  */

#ifdef AMIGA
#include <string.h>
#define bzero(p,n)   memset(p,'\0',n)
#endif AMIGA


extern FILE *t_src,*t_out,*t_doc,*t_lst;


/*  Interesting scanner constants  */

#define t_nameSize 31


/*  Interesting scanner variables  */

extern char   t_lineBuffer[];
extern char  *t_ptr;
extern char   t_strBuffer[];              /* for StrLit tokens */
extern short  t_lineNumber;
extern char   t_lineListed;
extern char   t_listing;                  /* do we want a listing file? */
extern short  t_lastId;                   /* id# of last id accepted */
extern char   t_idBuffer[];               /* id names stored here.. avg 12 chars */
extern short  t_idBufferNext;

struct t_idTableType {         /* identifier # is index into this table */
  char *name;
  short namelen;
  short code;                  /* keyword code, or 'pIdent' */
};

extern struct t_idTableType t_idTable[];
extern short  t_idTableNext;



struct t_tokenType {
  char accepted;
  char name[t_nameSize+1];
  short namelen;
  short val;                   /* id #, int lit, etc */
  short code;                  /* input token code (pIdent, pIntLit, etc) */
  short lineNumber;
  short colNumber;
};

extern  struct t_tokenType t_token;

#define t_getToken()   { if (t_token.accepted) t_getNextToken(); }




/*  Functions  */


char *t_getCodeName();


