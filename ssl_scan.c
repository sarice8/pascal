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

char    ssl_strlit_buffer [SSL_STRLIT_SIZE+2];

struct  ssl_id_table_struct ssl_id_table [ssl_id_table_size];
int     ssl_id_table_next;


#define ssl_id_buffer_size     5000        /* identifier string space       */
char   *ssl_id_buffer;                     /* Multiple buffers may be allocated */
int     ssl_id_buffer_next;


int    ssl_line_number;
char   ssl_line_listed;

int    ssl_last_id;                     /* last accepted identifier      */
char   ssl_last_id_text [SSL_NAME_SIZE+1];



struct ssl_token_struct ssl_token;      /* current token                 */


/* ------- Private scanner variables ------- */

static char   *s_src_ptr;                   /* current position within line  */

static struct ssl_token_table_struct   *s_keyword_table;
static struct ssl_token_table_struct   *s_operator_table;
static struct ssl_special_codes_struct *s_special_codes;
static int    s_code_charlit;
static int    s_enable_pascal_char_codes = 0;
static int    s_code_double_lit;

/*  Character classes  */

static char   s_upper  [256];
static char   s_blank  [256];
static char   s_id_start[256];
static char   s_id_cont [256];
static char   s_digit  [256];
static int    s_punct  [256];
static char   s_punct_multi [256];
static char   s_potential_comment_start[256];

/* Comment brackets */

typedef struct ssl_comment_bracket_struct {
  char start[16];
  char end[16];   // "" for comment to end-of-line
} ssl_comment_bracket;

#define SSL_COMMENT_BRACKETS_SIZE 5
ssl_comment_bracket ssl_comment_brackets[SSL_COMMENT_BRACKETS_SIZE];
int ssl_num_comment_brackets = 0;
int ssl_using_default_comment_brackets = 0;


void   s_lookup_id ();
void   s_hit_eof ();
void   s_restart_scanner_input_position ();
void   s_parse_strlit();


static int    s_end_include_at_eof = 1;

typedef struct ssl_include_info_struct {
    FILE*        file;
    char*        line_text;
    int          col;
    int          line_number;
} ssl_include_info;

// The include stack records where we should resume after the include.
#define SSL_INCLUDE_STACK_SIZE 40
ssl_include_info ssl_include_stack[SSL_INCLUDE_STACK_SIZE];
int ssl_include_sp = 0;


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
    s_code_charlit = s_special_codes->invalid;
    s_code_double_lit = s_special_codes->invalid;

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

    // By default, scanner accepts comments starting from '%' to end of line.
    // Applications can override this by calling ssl_scanner_init_comment() themselves.
    ssl_scanner_init_comment( "%", "" );
    ssl_using_default_comment_brackets = 1;
}


void
ssl_set_code_charlit( int code_charlit )
{
  s_code_charlit = code_charlit;
}


// If enabled, allow character codes of the form   #nnn
// These may be used standalone, or adjacent to each other or 'text'
// to form strlits.
//
void
ssl_enable_pascal_char_codes( int enable )
{
  s_enable_pascal_char_codes = enable;
}


// If set, allow floating point literals (double precision).
//
void
ssl_set_code_double_lit( int code_double_lit )
{
  s_code_double_lit = code_double_lit;
}


void
ssl_scanner_init_comment( const char* start_str, const char* end_str )
{
  if ( !start_str || !end_str ) {
    ssl_fatal( "ssl_scan.c: unexpected null passed to ssl_scanner_init_comment\n" );
  }

  if ( ssl_using_default_comment_brackets ) {
    // erase default configuration
    memset( s_potential_comment_start, '\0', 256 );
    ssl_num_comment_brackets = 0;
    ssl_using_default_comment_brackets = 0;
  }

  if ( ssl_num_comment_brackets == SSL_COMMENT_BRACKETS_SIZE ) {
    ssl_fatal( "ssl_scan.c: too many comment brackets defined\n" );
  }

  strcpy( ssl_comment_brackets[ ssl_num_comment_brackets ].start, start_str );
  strcpy( ssl_comment_brackets[ ssl_num_comment_brackets ].end, end_str );
  ++ssl_num_comment_brackets;

  s_potential_comment_start[ start_str[0] ] = 1;
}


/*  Used by some compilers to pass multiple times through input text.  */
void
ssl_reset_input ()
{
    fclose (ssl_src_file);

    while ( ssl_include_sp ) {
        ssl_end_include();
    }

    if ((ssl_src_file=fopen(ssl_input_filename, "r"))==NULL)
    {
        printf ("SSL: Can't open source file %s\n", ssl_input_filename);
        exit (-1);
    }

    s_restart_scanner_input_position ();
}


/*
 *  Should include files end automatically at eof?  Default true.
 */
void ssl_end_include_at_eof( int end_include_at_eof )
{
  s_end_include_at_eof = end_include_at_eof;
}


void
ssl_include_filename( const char* include_filename )
{
    // Record where we should resume when finished the include
    if ( ++ssl_include_sp >= SSL_INCLUDE_STACK_SIZE ) {
        ssl_fatal( "ssl include stack overflow\n" );
    }
    ssl_include_stack[ssl_include_sp].file = ssl_src_file;
    ssl_include_stack[ssl_include_sp].line_text = strdup( ssl_line_buffer );
    ssl_include_stack[ssl_include_sp].col = ( s_src_ptr - ssl_line_buffer );
    ssl_include_stack[ssl_include_sp].line_number = ssl_line_number;

    ssl_src_file = fopen (include_filename, "r");
    if (ssl_src_file == NULL)
    {
        sprintf (ssl_error_buffer, "Can't open include file '%s'", include_filename);
        ssl_fatal (ssl_error_buffer);
    }
    ssl_line_buffer[0] = '\0';
    s_src_ptr = ssl_line_buffer;
    ssl_line_listed = 1;
    ssl_line_number = 0;
}


/*  Stop reading the current include file, and resume the earlier file.
 *  By default this happens automatically when eof is reached,
 *  but the application can request to do it manually at an appropriate point.
 */
void
ssl_end_include()
{
    if ( ssl_include_sp == 0 ) {
        ssl_fatal( "ssl_end_include: was not including a file\n" );
    }

    fclose (ssl_src_file);

    ssl_src_file = ssl_include_stack[ssl_include_sp].file;
    strcpy( ssl_line_buffer, ssl_include_stack[ssl_include_sp].line_text );
    s_src_ptr = ssl_line_buffer + ssl_include_stack[ssl_include_sp].col;
    ssl_line_number = ssl_include_stack[ssl_include_sp].line_number;
    free( ssl_include_stack[ssl_include_sp].line_text );
    --ssl_include_sp;

    ssl_line_listed = 1;
    ssl_get_next_token();
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

    s_restart_scanner_input_position ();
}


/*  Set scanner input position to start of file (except that
    actual file variable is assumed to be there already)  */

void
s_restart_scanner_input_position ()
{
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
    int          lit;
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

        else if ( s_potential_comment_start[ s_src_ptr[0] ] )
        {
            // might be start of a comment
            int bracket;
            for ( bracket = 0; bracket < ssl_num_comment_brackets; ++bracket ) {
              if ( strncmp( s_src_ptr, ssl_comment_brackets[ bracket ].start,
                            strlen( ssl_comment_brackets[ bracket ].start ) ) == 0 ) {
                break;
              }
            }

            if ( bracket == ssl_num_comment_brackets ) {
              // Not a comment.  Done looking through whitespace/comments.
              break;
            }

            // If no closing bracket defined, this is a comment to end-of-line
            if ( ssl_comment_brackets[ bracket ].end[0] == '\0' ) {

              // Comment to end-of-line

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

            } else {

              // Comment until the close bracket, potentially across multiple lines.

              s_src_ptr += strlen( ssl_comment_brackets[ bracket ].start );
              while ( strncmp( s_src_ptr, ssl_comment_brackets[ bracket ].end,
                               strlen( ssl_comment_brackets[ bracket ].end ) ) != 0 ) {
                if ( !*s_src_ptr++ ) {
                  if (ssl_listing_callback && !ssl_line_listed)
                      (*ssl_listing_callback) (ssl_line_buffer, 0);
                  if (!fgets(ssl_line_buffer, ssl_line_size, ssl_src_file)) {
                      s_hit_eof();
                      return;
                  }
                  s_src_ptr = ssl_line_buffer;
                  ssl_line_number++;
                  ssl_line_listed = 0;
                }

              }
              s_src_ptr += strlen( ssl_comment_brackets[ bracket ].end );
            }
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

    else if (s_digit[*s_src_ptr])          /* integer or floatint point literal */
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
            ssl_token.val = lit;
            ssl_token.code = s_special_codes->intlit;
        }
        else
        {
            while (s_digit[*s_src_ptr])
            {
                lit = lit*10 - '0' + *s_src_ptr;
                *p++ = *s_src_ptr++;
            }

            if ( *s_src_ptr == '.' && s_digit[s_src_ptr[1]] &&
                 ( s_code_double_lit != s_special_codes->invalid ) ) {
              // floating point literal
              *p++ = *s_src_ptr++;
              while ( s_digit[*s_src_ptr] ) {
                  *p++ = *s_src_ptr++;
              }
              if ( *s_src_ptr == 'E' || *s_src_ptr == 'e' ) {
                  *p++ = *s_src_ptr++;
                  if ( *s_src_ptr == '+' || *s_src_ptr == '-' ) {
                      *p++ = *s_src_ptr;
                  }
                  while ( s_digit[*s_src_ptr] ) {
                      *p++ = *s_src_ptr++;
                  }
              }
              *p = '\0';
              // TO DO: I think I'm supposed to auto-detect whether the literal could be single
              // rather than double.  For now I always produce double.
              // TO DO: ideally I wouldn't rely on the c library for this, but whatever
              ssl_token.double_val = atof( ssl_token.name );
              ssl_token.code = s_code_double_lit;
            } else {
              *p = '\0';
              ssl_token.val = lit;
              ssl_token.code = s_special_codes->intlit;
            }
        }
    }

    else if ( *s_src_ptr == '\'' ||
              ( s_enable_pascal_char_codes && *s_src_ptr == '#' ) )
    {
        s_parse_strlit();

        // For Pascal: a string of length 1 is actually a charlit
        if ( ssl_token.namelen == 1 &&
             s_code_charlit != s_special_codes->invalid ) {
          ssl_token.code = s_code_charlit;
          ssl_token.val = (int) ssl_strlit_buffer[0];
        }
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


// Pascal strlit:
// A strit consists of a sequence of 'text' and #nnn character code bytes,
// with no intervening space.   Within 'text', a double '' means the ' character.
//
void
s_parse_strlit()
{
    char* p = ssl_strlit_buffer;
    ssl_token.code = s_special_codes->strlit;

    while ( 1 ) {
        if ( *s_src_ptr == '\'' ) {
            s_src_ptr++;

            while ( 1 ) {
                if ( *s_src_ptr == '\'' ) {
                    if ( *++s_src_ptr == '\'' ) {
                        if ( p - ssl_strlit_buffer < SSL_STRLIT_SIZE ) {
                            *p++ = '\'';
                        }
                        ++s_src_ptr;
                    } else {
                        break;
                    }
                } else if ( *s_src_ptr == '\0' ) {
                    break;
                } else {
                    if ( p - ssl_strlit_buffer < SSL_STRLIT_SIZE ) {
                        *p++ = *s_src_ptr;
                    }
                    ++s_src_ptr;
                }
            }

        } else if ( s_enable_pascal_char_codes && *s_src_ptr == '#' ) {
           int code = 0;
           s_src_ptr++;
           while ( *s_src_ptr >= '0' && *s_src_ptr <= '9' ) {
               code *= 10;
               code += *s_src_ptr - '0';
               ++s_src_ptr;
           }
           if ( p - ssl_strlit_buffer < SSL_STRLIT_SIZE ) {
               *p++ = (char) code;
           }

        } else {
            break;
        }
    }

    *p = '\0';
    ssl_token.namelen = p-ssl_strlit_buffer;
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
    {
        ssl_last_id = ssl_token.val;
        strcpy (ssl_last_id_text, ssl_token.name);  /* Keep text -- could do better */
    }
}


// Unaccept the current token.
// The next call to get_token() will return the same token.
//
void
ssl_unaccept_token()
{
    ssl_token.accepted = 0;
}


/*  Reached end of source file.  May have just been end of 'include' file.  */
void
s_hit_eof ()
{
    if ( ssl_include_sp && s_end_include_at_eof ) {
        ssl_end_include();
    } else {
        ssl_token.code = s_special_codes->eof;
    }
}


/* Used by debugger: get current input file position */

void
ssl_get_input_position (int* line_ptr, int* col_ptr)
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

int
ssl_add_id( const char* name, int code )
{
    int          namelen;

    if (ssl_id_table_next >= ssl_id_table_size)
        ssl_fatal("id table overflow");

    namelen = strlen(name);
    ssl_id_table [ssl_id_table_next].namelen = namelen;
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


/*  ssl_lookup_id
 *  Look up an identifier name in the scanner's identifier table.
 *  If not found, it is added to the table, with the given code (typically pIDENTIFIER).
 *  Returns the identifier value.
 */
int
ssl_lookup_id( const char* name, int code )
{
    int i;
    const char* a;
    const char* b;

    int namelen = strlen( name );

    if (ssl_case_sensitive)
    {
        for (i=1; i < ssl_id_table_next; i++)
        {
            if (namelen == ssl_id_table [i].namelen)
            {
                a = name;
                b = ssl_id_table [i].name;
                while ((*a++) == (*b++))
                {
                    if (!*a)    /* match */
                    {
                        return i;
                    }
                }
            }
        }
    }
    else
    {
        for (i=1; i < ssl_id_table_next; i++)
        {
            if (namelen == ssl_id_table [i].namelen)
            {
                a = name;
                b = ssl_id_table [i].name;
                while (s_upper[*a++] == s_upper[*b++])
                {
                    if (!*a)    /* match */
                    {
                        return i;
                    }
                }
            }
        }
    }
    return ssl_add_id( name, code );
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
    ssl_token.val = ssl_lookup_id( ssl_token.name, s_special_codes->ident );
    ssl_token.code = ssl_id_table[ ssl_token.val ].code;
}


/*  return description of token code  */

const char*
ssl_get_code_name ( int code )
{
    static char   buffer [50];
    const char*   p;
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


const char*
ssl_get_id_string( long id )
{
    if (id > 0)
        return (ssl_id_table [id].name);
    else
        return ("<no_name>");   /* Useful for error conditions */
}

