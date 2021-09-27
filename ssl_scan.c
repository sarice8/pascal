/**/ 
static char sccsid[] = "%W% %G% %U% %P%";
/**/

/*
*****************************************************************************
*
*   Syntax/Semantic Language Runtime Scanner Module
*
*   by Steve Rice
*
*   Aug 18, 1993
*
*****************************************************************************
*
*   ssl_scan.c          Runtime scanner (for use by SSL applications)
*
*   HISTORY
*----------------------------------------------------------------------------
*   08/18/93 | Steve  | Based on scanner in my Schema compiler
*            |        | 
*
*****************************************************************************
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef AMIGA
#include <dos.h>
#endif // AMIGA


/*  Definitions for scanner and runtime module  */
#include "ssl_rt.h"


#define ssl_line_size          256
char    ssl_line_buffer [ssl_line_size];

char    ssl_strlit_buffer [SSL_STRLIT_SIZE+1];

struct  ssl_id_table_struct ssl_id_table [ssl_id_table_size];
short   ssl_id_table_next;


#define ssl_id_buffer_size     5000        /* initial identifier string space       */
char   *ssl_id_buffer;                     /* Mutliple buffers may be allocated */
short   ssl_id_buffer_next;


short  ssl_line_number;
char   ssl_line_listed;
short  ssl_last_id;                     /* last accepted identifier      */


struct ssl_token_struct ssl_token;      /* current token                 */


/* ------- Private scanner variables ------- */

static char   *s_src_ptr;                   /* current position within line  */

static struct ssl_token_table_struct   *s_keyword_table;
static struct ssl_token_table_struct   *s_operator_table;
static struct ssl_special_codes_struct *s_special_codes;

/*  Character classes  */

static char   s_upper  [256];
static char   s_blank  [256];
static char   s_id_start[256];
static char   s_id_cont [256];
static char   s_digit  [256];
static short  s_punct  [256];
static char   s_punct_multi [256];

void   s_lookup_id ();
void   s_hit_eof ();

static int    s_including_file;   /* "Include" application source files */
static FILE  *s_src_includer;

/* ---- End of private variables ----- */


/*  Public functions  */

void
ssl_init_scanner (keyword_table, operator_table, special_codes)
struct ssl_token_table_struct        *keyword_table;
struct ssl_token_table_struct        *operator_table;
struct ssl_special_codes_struct      *special_codes;
{
    int       i;

    /* Set up initial identifier name buffer */
    ssl_id_buffer = (char *)malloc(ssl_id_buffer_size);
    ssl_id_buffer_next = 0;

    s_keyword_table = keyword_table;
    s_operator_table = operator_table;
    s_special_codes = special_codes;

    /* Set up scanner tables */

    for (i=0; i<256; i++)
        s_upper[i] = i;

    for (i='a'; i<='z'; i++)
    {
        s_upper[i] = i - ('a'-'A');
        s_id_start[i] = 1;
        s_id_cont[i] = 1;
    }

    for (i='A'; i<='Z'; i++)
    {
        s_id_start[i] = 1;
        s_id_cont[i] = 1;
    }

    s_id_start['_'] = 1;
    s_id_cont['_']  = 1;
    s_id_cont['$']  = 1;

    for (i='0'; i<='9'; i++)
    {
        s_digit[i] = 1;
        s_id_cont[i] = 1;
    }

    s_blank[' ']  = 1;
    s_blank['\t'] = 1;
    s_blank['\n'] = 1;

    /*  Set up puctuation (operator) tables  */

    for (i = 0; i < 256; i++)
        s_punct[i] = s_special_codes->invalid;

    for (i = 0; s_operator_table[i].name != NULL; i++)
    {
        if (strlen(s_operator_table[i].name) > 1)
        {
            /* Indicate that first character is a potential
               multiple-character operator.  Will look up in
               s_operator_table[] at scan time, for now. */
            s_punct_multi[s_operator_table[i].name[0]] = 1;
        }
        else
        {
            /* Single-character punctuation */
            s_punct[s_operator_table[i].name[0]] = s_operator_table[i].code;
        }
    }

    s_including_file = 0;
}


void
ssl_include_filename (include_filename)
char                 *include_filename;
{
    if (s_including_file)
        ssl_fatal ("Nested includes are not yet supported");
    s_including_file = 1;
    s_src_includer = ssl_src_file;
    ssl_src_file = fopen (include_filename, "r");
    if (ssl_src_file == NULL)
    {
        sprintf (ssl_error_buffer, "Can't open include file '%s'", include_filename);
        ssl_fatal (ssl_error_buffer);
    }
    ssl_line_buffer[0] = '\0';
    s_src_ptr = ssl_line_buffer;
    ssl_line_listed = 1;
}


/*  Private functions  */

/*  Called by SSL (ssl_restart()) to reset scanner tables
    to their initial condition (i.e. the condition set by
    ssl_init_scanner: reopening source, resetting identifier table
    from the application keyword table.
    NOTE, will always be called before first run, so can wait until here
    to put keywords into ident table, rather than in ssl_init_scanner. */

void
ssl_restart_scanner ()
{
    int    i;

    /*  reset ident table  */
    ssl_id_table_next = 1;              /* id 0 unused */

    for (i=0; s_keyword_table[i].name != NULL; i++)
        (void) ssl_add_id (s_keyword_table[i].name, s_keyword_table[i].code);


    ssl_line_buffer[0] = '\0';          /* set up scanner */
    s_src_ptr = ssl_line_buffer;
    ssl_line_number = 0;
    ssl_line_listed = 1;
    ssl_token.code = s_special_codes->invalid;  /* Should not be 'eof' */
    ssl_token.accepted = 1;

}  


/*
*****************************************************************************
*
* ssl_get_next_token
*
* Description:
*   Read the next token from the source file.
*
*****************************************************************************
*/

void
ssl_get_next_token ()
{
    char        *p;
    short        lit;
    int          i;
    int          longest_match_idx;
    int          longest_match_len;
    int          len;


    ssl_token.accepted = 0;

    if (ssl_token.code == s_special_codes->eof)                 /* trying to read past EOF */
        ssl_fatal ("Unexpected end of file"); 


    /* skip to start of token */

    while (1)
    {
        while (s_blank[*s_src_ptr])
            s_src_ptr++;

        if (!*s_src_ptr)                                       /* end of line */
        {
            if (ssl_listing_callback && !ssl_line_listed)
                (*ssl_listing_callback) (ssl_line_buffer, 0);

            if (!fgets(ssl_line_buffer,ssl_line_size,ssl_src_file))
            {
                s_hit_eof();
                return;
            }
            s_src_ptr = ssl_line_buffer;
            ssl_line_number++;
            ssl_line_listed = 0;
        }

        else if (s_src_ptr[0]=='%')           /* comment */
        {
            if (ssl_listing_callback && !ssl_line_listed)
                (*ssl_listing_callback) (ssl_line_buffer, 0);
            if (!fgets(ssl_line_buffer, ssl_line_size, ssl_src_file))
            {
                s_hit_eof();
                return;
            }
            s_src_ptr = ssl_line_buffer;
            ssl_line_number++;
            ssl_line_listed = 0;
        }

        else
            break;
    }


    /* Read token.  Screen identifiers, convert literals. */

    ssl_token.lineNumber = ssl_line_number;
    ssl_token.colNumber  = s_src_ptr - ssl_line_buffer;
    p = ssl_token.name;

    if (s_id_start[*s_src_ptr])             /* identifier */
    {
        while (s_id_cont[*s_src_ptr] && p-ssl_token.name < SSL_NAME_SIZE)
            *p++ = *s_src_ptr++;
        *p = '\0';
        ssl_token.namelen = p-ssl_token.name;
        ssl_token.code = s_special_codes->ident;
        s_lookup_id();
    }

    else if (s_digit[*s_src_ptr])          /* integer literal */
    {
        lit = 0;
        if (*s_src_ptr == '0' && s_src_ptr[1]=='x')
        {
            /* Hex literal:  0xABCD */
            s_src_ptr += 2;
            while (1)
            {
                if (s_digit[*s_src_ptr])
                    lit = lit*16 - '0' + *s_src_ptr;
                else if (*s_src_ptr >= 'A' && *s_src_ptr <= 'F')
                    lit = lit*16 - 'A' + *s_src_ptr + 10;
                else if (*s_src_ptr >= 'a' && *s_src_ptr <= 'f')
                    lit = lit*16 - 'a' + *s_src_ptr + 10;
                else
                    break;
                *p++ = *s_src_ptr++;
            }
            *p = '\0';
        }
        else
        {
            while (s_digit[*s_src_ptr])
            {
                lit = lit*10 - '0' + *s_src_ptr;
                *p++ = *s_src_ptr++;
            }
            *p = '\0';
        }
        ssl_token.val = lit;
        ssl_token.code = s_special_codes->intlit;
    }

    else if (*s_src_ptr == '\'')
    {
        s_src_ptr++;
        ssl_token.code = s_special_codes->strlit;
        p = ssl_strlit_buffer;
        while (p - ssl_strlit_buffer < SSL_STRLIT_SIZE)
        {
            if (*s_src_ptr == '\'')
            {
                if (*++s_src_ptr == '\'')
                {
                    *p++ = '\'';
                    s_src_ptr++;
                }
                else
                    break;
            }
            else if (*s_src_ptr == '\0')
                break;
            else
                *p++ = *s_src_ptr++;
        }
        *p = '\0';
        ssl_token.namelen = p-ssl_strlit_buffer;
    }

    else                               /* some form of punctuation */
    {
        if (s_punct_multi[*s_src_ptr])  /* Start of multi-character punct token */
        {
            /* This will be improved */

            longest_match_len = 0;
            for (i = 0; s_operator_table[i].name != NULL; i++)
            {
                if (s_operator_table[i].name[0] != *s_src_ptr)
                    continue;

                len = strlen(s_operator_table[i].name);
                if (len <= longest_match_len)
                    continue;

                if (strncmp(s_src_ptr, s_operator_table[i].name, len) == 0)
                {
                    longest_match_idx = i;
                    longest_match_len = len;
                }
            }
            if (longest_match_len > 0)
            {
                ssl_token.code = s_operator_table[longest_match_idx].code;
                s_src_ptr += longest_match_len;
            }
            else
            {
                ssl_token.code = s_punct[*s_src_ptr++];
            }
        }
        else
        {
            ssl_token.code = s_punct[*s_src_ptr++];
        }

        if (ssl_token.code == s_special_codes->invalid)
        {
          --s_src_ptr;
          ssl_token.name[0] = *s_src_ptr++;
          ssl_token.name[1] = '\0';
          ssl_error ("SSL scanner: Invalid token");
          /* Note, may be more than character, if application rejected token type by
             setting code to invalid (e.g. intlit, strlit, ...) */
        }
    }
}


/*  Token accepted.  Do listing, remeber last identifier.  */

void
ssl_accept_token ()
{
    ssl_token.accepted = 1;

    if (ssl_listing_callback && !ssl_line_listed)
    {
        (*ssl_listing_callback) (ssl_line_buffer, 1);
        ssl_line_listed = 1;
    }  

    if (ssl_token.code == s_special_codes->ident)
        ssl_last_id = ssl_token.val;
}


/*  Reached end of source file.  May have just been end of 'include' file.  */
void
s_hit_eof ()
{
    if (!s_including_file)
    {
        ssl_token.code = s_special_codes->eof;
    }
    else
    {
        fclose (ssl_src_file);
        s_including_file = 0;
        ssl_src_file = s_src_includer;
        ssl_line_buffer[0] = '\0';
        s_src_ptr = ssl_line_buffer;
        ssl_line_listed = 1;
        ssl_get_next_token();
    }
}

/* Used by debugger: get current input file position */

void
ssl_get_input_position (line_ptr, col_ptr)
short                  *line_ptr;
short                  *col_ptr;
{
    if (ssl_token.accepted == 0)
    {
        *line_ptr = ssl_token.lineNumber;
        *col_ptr  = ssl_token.colNumber;
    }
    else
    {
        *line_ptr = ssl_line_number;
        *col_ptr  = s_src_ptr - ssl_line_buffer;
    }
}


/*
*****************************************************************************
*
* ssl_add_id
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

short ssl_add_id (name, code)
char          *name;
short          code;
{
    short          namelen;

    if (ssl_id_table_next >= ssl_id_table_size)
        ssl_fatal("id table overflow");

    namelen = strlen(name);
    ssl_id_table [ssl_id_table_next].namelen = namelen;
    if (ssl_id_buffer_next+namelen >= ssl_id_buffer_size)
    if (ssl_id_buffer_next+namelen >= ssl_id_buffer_size)
    {
        ssl_id_buffer = (char *)malloc(ssl_id_buffer_size);
        ssl_id_buffer_next = 0;
    }

    ssl_id_table [ssl_id_table_next].name = &ssl_id_buffer[ssl_id_buffer_next];
    strcpy(&ssl_id_buffer[ssl_id_buffer_next],name);
    ssl_id_buffer_next += namelen + 1;
    ssl_id_table [ssl_id_table_next].code = code;

    return(ssl_id_table_next++);
}


/*
*****************************************************************************
*
* s_lookup_id
*
* Description:
*   Searches the identifier table for a given identifier string.
*   If it is not found, the identifier is added to the table.
*
*   Stores the identifier information (index, and any keyword code)
*   in the appropriate fields of ssl_token.
*
*   NOTE: index 0 of the table is not used.  It denotes an error condition
*         in the programs that use this.
*
*****************************************************************************
*/

void
s_lookup_id ()
{
    short              i;
    register char     *a, *b;

    if (ssl_case_sensitive)
    {
        for (i=1; i < ssl_id_table_next; i++)
        {
            if (ssl_token.namelen == ssl_id_table [i].namelen)
            {
                a = ssl_token.name;
                b = ssl_id_table [i].name;
                while ((*a++) == (*b++))
                {
                    if (!*a)    /* match */
                    {
                        ssl_token.code = ssl_id_table [i].code;  /* for built-in ids */
                        ssl_token.val  = i;                  /* for user ids */
                        return;
                    }
                }
            }
        }
    }
    else
    {
        for (i=1; i < ssl_id_table_next; i++)
        {
            if (ssl_token.namelen == ssl_id_table [i].namelen)
            {
                a = ssl_token.name;
                b = ssl_id_table [i].name;
                while (s_upper[*a++] == s_upper[*b++])
                {
                    if (!*a)    /* match */
                    {
                        ssl_token.code = ssl_id_table [i].code;  /* for built-in ids */
                        ssl_token.val  = i;                  /* for user ids */
                        return;
                    }
                }
            }
        }
    }

    ssl_token.val = ssl_add_id (ssl_token.name, s_special_codes->ident);
}


/*  return description of token code  */

char*
ssl_get_code_name ( short code )
{
    static char   buffer [50];
    char         *p;
    int           i;

    p = NULL;

    if (code == s_special_codes->ident)
    {
        p = "<identifier>";
    }
    else
    {
        for (i = 0; s_keyword_table[i].name != NULL; i++)
        {
            if (code == s_keyword_table[i].code)
            {
                p = s_keyword_table[i].name;
                break;
            }
        }

        if (p == NULL)
        {
            for (i = 0; s_operator_table[i].name != NULL; i++)
            {
                if (code == s_operator_table[i].code)
                {
                    p = s_operator_table[i].name;
                    break;
                }
            }
        }

        if (p == NULL)
        {
            if (code == s_special_codes->invalid)
                p = "<invalid>";
            else if (code == s_special_codes->eof)
                p = "<eof>";
            else if (code == s_special_codes->intlit)
                p = "<integer>";
            else if (code == s_special_codes->strlit)
                p = "<string>";
            else
            {
                sprintf (buffer, "<code %d>", code);
                p = buffer;
            }
        }
    }
    return (p);
}


char *ssl_get_id_string (id)
long id;
{
    if (id > 0)
        return (ssl_id_table [id].name);
    else
        return ("<no_name>");   /* Useful for error conditions */
}

