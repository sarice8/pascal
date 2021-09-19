/*
*****************************************************************************
*
*   Database Schema Compiler
*
*   by Steve Rice
*
*   May 1, 1991
*
*****************************************************************************
*
*   schema_scanner.c    Scanner subsystem
*
*   HISTORY
*----------------------------------------------------------------------------
*   05/01/91 | Steve  | Initial version, based on Verilog scanner vscan.c
*
*****************************************************************************
*/



#include <stdio.h>
#include <string.h>

#ifdef AMIGA
#include <dos.h>
#endif AMIGA

#include "schema.h"

#include "schema_glob.h"



/*
*****************************************************************************

   DESIGN NOTES:

   Scanner variables start with 't_'.

   A token is read and screened when oInput, oInputAny, or oInputChoice
   is encountered.  When an id is accepted, set t_lastId to its id #.

   The 'code' of a token is pIDENTIFIER, etc.
   The 'id #' of an identifier is the unique # for that name (actually
   the index into the scanner's identifier table).

   The scanner maintains an identifier table, which is used to find
   keywords, and to translate names into integers.  Whenever a new
   identifier is found, it is added to the table.  It is separate from
   the symbol table constructed by the compiler's semantic operations.

*****************************************************************************
*/


#define t_lineSize      256
#define t_strSize       256
#define t_idTableSize   400


char   t_lineBuffer [t_lineSize];       /* current line                  */
char  *t_ptr;                           /* current position within line  */

char   t_strBuffer [t_strSize+1];       /* for string literal tokens     */

short  t_lineNumber;
char   t_lineListed;
char   t_listing;                       /* flag: listing file requested? */
short  t_lastId;                        /* last accepted identifier      */


char   t_upper  [256];                  /* character classes             */
char   t_blank  [256];
char   t_idStart[256];
char   t_idCont [256];
char   t_digit  [256];
short  t_punct  [256];
char   t_punct_multi [256];



struct t_tokenType t_token;             /* current token                 */



#define t_idBufferSize 5000             /* identifier string space       */
char    t_idBuffer[t_idBufferSize];
short   t_idBufferNext;

                                        /* identifier table              */
struct t_idTableType t_idTable[t_idTableSize];
short  t_idTableNext;



struct t_kwTableType {                  /* keyword table                 */
  char *name;                           /* (later loaded into id table)  */
  short code;
} t_kwTable[] = {
    "Alt",           pAlt,
    "End",           pEnd,
    "Integer4",      pInteger4,
    "Is",            pIs,
    "List",          pList,
    "Node",          pNode,
    "Opt",           pOpt,
    "Pri",           pPri,
    "Root",          pRoot,
    "Schema",        pSchema,
    "", 0
};



/*
*****************************************************************************
*
* t_initScanner
*
* Description:
*   Sets up scanner variables including identifier array.
*
*****************************************************************************
*/


t_initScanner ()
{
    register short      i;
    short               t_addId();


    /*  Global vars initialized to zero automatically  */

    for (i=0; i<256; i++)
        t_upper[i] = i;

    for (i='a'; i<='z'; i++)
    {
        t_upper[i] = i - ('a'-'A');
        t_idStart[i] = 1;
        t_idCont[i] = 1;
    }

    for (i='A'; i<='Z'; i++)
    {
        t_idStart[i] = 1;
        t_idCont[i] = 1;
    }

    t_idStart['_'] = 1;
    t_idCont['_']  = 1;
    t_idCont['$']  = 1;

    for (i='0'; i<='9'; i++)
    {
        t_digit[i] = 1;
        t_idCont[i] = 1;
    }

    t_blank[' ']  = 1;
    t_blank['\t'] = 1;
    t_blank['\n'] = 1;


    for (i=0; i<256; i++)            /* single-character tokens */
        t_punct[i] = pINVALID;

    t_punct[':'] = pCOLON;    t_punct_multi[':'] = 1;
    t_punct[','] = pCOMMA;
    t_punct['('] = pLPAREN;
    t_punct[')'] = pRPAREN;
    t_punct['['] = pLSQUARE;
    t_punct[']'] = pRSQUARE;
                              t_punct_multi['='] = 1;


    t_idTableNext = 0;               /* set up identifier table */
    t_addId ("NONAME", 0);
    for (i=0; *t_kwTable[i].name; i++)
        (void) t_addId (t_kwTable[i].name, t_kwTable[i].code);


    t_lineBuffer[0] = '\0';          /* set up scanner */
    t_ptr = t_lineBuffer;
    t_lineNumber = 0;
    t_lineListed = 1;
    t_token.code = pINVALID;
    t_token.accepted = 1;

}  


/*
*****************************************************************************
*
* t_getNextToken
*
* Description:
*   Read the next token from the source file.
*
*****************************************************************************
*/

t_getNextToken ()
{
    char        *p;
    short        lit;


    t_token.accepted = 0;

    if (t_token.code == pEOF)                 /* trying to read past EOF */
        t_fatal ("Unexpected end of file"); 


    /* skip to start of token */

    while (1)
    {
        while (t_blank[*t_ptr])
            t_ptr++;

        if (!*t_ptr)                                       /* end of line */
        {
            if (t_listing && !t_lineListed)
                fprintf(t_lst,"      %s",t_lineBuffer);
            if (!fgets(t_lineBuffer,t_lineSize,t_src))
            {
                t_token.code = pEOF;
                return;
            }
            t_ptr = t_lineBuffer;
            t_lineNumber++;
            t_lineListed = 0;
        }

        else if (t_ptr[0]=='%')           /* comment */
        {
            if (t_listing && !t_lineListed)
                fprintf (t_lst, "      %s", t_lineBuffer);
            if (!fgets(t_lineBuffer, t_lineSize, t_src))
            {
                t_token.code = pEOF;
                return;
            }
            t_ptr = t_lineBuffer;
            t_lineNumber++;
            t_lineListed = 0;
        }

        else
            break;
    }


    /* Read token.  Screen identifiers, convert literals. */

    t_token.lineNumber = t_lineNumber;
    t_token.colNumber  = t_ptr - t_lineBuffer;
    p = t_token.name;

    if (t_idStart[*t_ptr])             /* identifier */
    {
        while (t_idCont[*t_ptr] && p-t_token.name < t_nameSize)
            *p++ = *t_ptr++;
        *p = '\0';
        t_token.namelen = p-t_token.name;
        t_token.code = pIDENTIFIER;
        t_lookupId();
    }

    else if (t_digit[*t_ptr])          /* integer literal */
    {
        lit = 0;
        while (t_digit[*t_ptr])
        {
            lit = lit*10 - '0' + *t_ptr;
            *p++ = *t_ptr++;
        }
        *p = '\0';
        t_token.val = lit;
        t_token.code = pINVALID;   /* don't need this, currently */
    }

    else                               /* some form of punctuation */
    {
        if (t_punct_multi[*t_ptr])
        {
            switch (*t_ptr++)
            {
                case ':' :  if ((*t_ptr == ':') && (t_ptr[1] == '='))
                            {
                                t_token.code = pDERIVES;
                                t_ptr += 2;
                            }
                            else t_token.code = pCOLON;
                            break;
                case '=' :  if (*t_ptr == '>')
                            {
                                t_token.code = pCONTAINS;
                                t_ptr++;
                            }
                            else t_token.code = pINVALID;
                            break;
                 default :  t_fatal("scanner punctuation internal error");
            }
        }
        else
        {
            t_token.code = t_punct[*t_ptr++];
        }
        if (t_token.code == pINVALID)
        {
          --t_ptr;
          t_token.name[0] = *t_ptr++;
          t_token.name[1] = '\0';
          t_error ("Invalid character");
        }
    }
}


/*
*****************************************************************************
*
* t_addId
*
* Description:
*   Add an identifier string to the identifier table, along with a code
*   which distinguishes among keywords.
*   Returns the identifier index, used to reference this identifier.
*
* Design notes:
*   A common string area is used to hold the identifiers.  This may be
*   overkill.
*
*****************************************************************************
*/

short t_addId (name, code)
char          *name;
short          code;
{
    short          namelen;

    if (t_idTableNext >= t_idTableSize)
        t_fatal("id table overflow");

    namelen = strlen(name);
    t_idTable[t_idTableNext].namelen = namelen;
    if (t_idBufferNext+namelen >= t_idBufferSize)
        t_fatal("id name buffer overflow");

    t_idTable[t_idTableNext].name = &t_idBuffer[t_idBufferNext];
    strcpy(&t_idBuffer[t_idBufferNext],name);
    t_idBufferNext += namelen + 1;
    t_idTable[t_idTableNext].code = code;

    return(t_idTableNext++);
}


/*
*****************************************************************************
*
* t_lookupId
*
* Description:
*   Searches the identifier table for a given identifier string.
*   If it is not found, the identifier is added to the table.
*
*   Stores the identifier information (index, and any keyword code)
*   in the appropriate fields of t_token.
*
*   NOTE: index 0 of the table is not used.  It denotes an error condition
*         in the programs that use this.
*
*****************************************************************************
*/

t_lookupId ()
{
    short              i,  t_addId();
    register char     *a, *b;

    for (i=1; i < t_idTableNext; i++)
    {
        if (t_token.namelen == t_idTable[i].namelen)
        {
            a = t_token.name;
            b = t_idTable[i].name;
            while ((*a++) == (*b++))
            {
                if (!*a)    /* match */
                {
                    t_token.code = t_idTable[i].code;  /* for built-in ids */
                    t_token.val  = i;                  /* for user ids */
                    return;
                }
            }
        }
    }

    t_token.val = t_addId (t_token.name, pIDENTIFIER);
}


/*  return description of token code  */

char *t_getCodeName (code)
int code;
{
    static char   buffer [50];
    char         *p;
    int           i;

    p = NULL;

    if (code == pIDENTIFIER)
        p = "<identifier>";
    else
    {
        for (i = 0; i < 256; i++)
            if (code == t_punct[i])
            {
                sprintf (buffer, "%c", (char) i);
                p = buffer;
                break;
            }
        if (p == NULL)
        {
            for (i = 0; i < t_idTableNext; i++)
                if (code == t_idTable[i].code)
                {
                    strcpy (buffer, t_idTable[i].name);
                    p = buffer;
                    break;
                }
        }
        if (p == NULL)
        {
            sprintf (buffer, "<code %d>", code);       /* multiple-char punctuation too */
            p = buffer;
        }
    }
    return (p);
}


