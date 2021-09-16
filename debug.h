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
 

/*  Set address field in dbg_variables to this, to register a
 *  generic display routine for a type (defined in SSL).
 *  The routine will be called with the address of the variable
 *  and the udata (just like a variable display method).
 */
#define DBG_TYPE_DISPLAY  ((char *)0x1)


