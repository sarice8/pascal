/*

  scanner.c    --  first pass of Steve's Pascal compiler

  HISTORY
    28Jul89    First version

  Uses 'level 2' ANSI C file routines.

*/

#include <stdio.h>
#include <string.h>
#include <dos.h>

/* (uses dos.h for onbreak function) */

FILE *src,*out;

#define match(a,b,n) (!strncmp(a,b,n))

#define STR_SIZE 256
  
char   str1[STR_SIZE];
char   *tptr;
short  line_number;

enum kindType {alphaKind, numberKind, punctKind, strKind, stopKind};

struct tokenType {
  char text[STR_SIZE];
  enum kindType kind;
} token;

char   upper[256],blank[256],alphaStart[256],alphaCont[256],number[256];

main(argc,argv)
int argc;
char *argv[];
{
  int hit_break();

  if (argc<2) {
    printf("Usage:   scanner <file>\n");
    return(-1);
  }

  if((src=fopen(argv[1],"r"))==NULL) return(-1);
  if((out=fopen("ram:a.out","w"))==NULL) exit(-1);

  onbreak(&hit_break);

  init();

  get_token();
  while (token.kind != stopKind) {
    process_token();
    get_token();
  }

  fclose(src);
  fclose(out);
}

init ()
{
  register short i;

  /* C guarantees static vars to start with 0, so here I don't
       bother to initialize all of blank,alphaStart,alphaCont,number */

  for (i=0;i<256;i++)
    upper[i] = i;
  for (i='a';i<='z';i++) {
    upper[i] = i - ('a'-'A');
    alphaStart[i] = 1;
    alphaCont[i] = 1;
  }
  for (i='A';i<='Z';i++) {
    alphaStart[i] = 1;
    alphaCont[i] = 1;
  }
  alphaStart['_'] = 1;
  alphaCont['_'] = 1;
  for (i='0';i<='9';i++) {
    number[i] = 1;
    alphaCont[i] = 1;
  }
  blank[' '] = 1;
  blank['\t'] = 1;
  blank['\n'] = 1;
  str1[0] = '\0';
  tptr = str1;
  line_number = 0;
}  

get_token()
{
  char more,*p,readln();

  /* skip to start of token */

  while (1) {
    while (blank[*tptr])
      tptr++;

    if (*tptr=='\0') {
      more = readln(str1);
      if (!more) {
        token.kind = stopKind;
        return;
      }
      tptr = str1;
      line_number++;
    } else if (*tptr=='{') {
      tptr++;
      while (*tptr!='}') {
        if (*tptr=='\0') {
          more = readln(str1);
          if (!more) {
            token.kind = stopKind;
            return;
          }
          tptr = str1;
          line_number++;
        } else {
          tptr++;
        }
      }
      tptr++;
    } else {
      break;
    }
  }

  /* copy token */

  p = token.text;
  if (alphaStart[*tptr]) {
    while (alphaCont[*tptr])
      *p++ = upper[*tptr++];
    *p = '\0';
    token.kind = alphaKind;
  } else if (number[*tptr]) {
    while (number[*tptr])
      *p++ = *tptr++;
    *p = '\0';
    token.kind = numberKind;
  } else if (*tptr=='\'') {
    tptr++;
    token.kind = strKind;
    while (1) {
      if (*tptr=='\'') {
        if (*++tptr=='\'') {
          *p++ = '\'';
          tptr++;
        } else {
          *p = '\0';
          break;
        }
      } else if (*tptr=='\0') {
        *p = '\0';
        break;
      } else {
        *p++ = *tptr++;
      }
    }
  } else {
    *p++ = *tptr++;
    *p = '\0';
    token.kind = punctKind;
  }
}

process_token()
{
  writeln(token.text);
}

/* returns 0 if EOF, else 1.  str1 gets the '\0'-terminated string
   (usually with \n, except on last line). */

char readln(str1)
char *str1;
{
  char *ptr;
  ptr = fgets(str1,STR_SIZE,src);
  if (ptr==NULL)
    return(0);
  return(1);
}

writeln(str1)
char *str1;
{
  fprintf(out,"%s\n",str1);
}

hit_break()
{
  printf("Breaking...\n");
  fclose(src);
  fclose(out);
  return(1);
}