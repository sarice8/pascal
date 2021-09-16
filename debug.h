/**/
/* static char sccsid[] = "%W% %G% %U% %P%"; */
/**/

/*  vdebug.h  */

/*  Application Interface to debugger  */

#ifdef AMIGA
#include <string.h>
#define bzero(p,n)   memset(p,'\0',n)
#endif AMIGA

typedef struct dbg_variables_struct {
    char  *name;    /* Name of variable for debug user */
    char  *address; /* Location of variable */
    char  *udata;   /* User data passed to display func */
    int  (*display_method)(/* char *, char * */);  /* display func */
} dbg_variables;
 


