/*

  ssl.c    -- Amiga Syntax/Semantic Language Processor

              This is the table walker for the ssl processor
              written in ssl.

  HISTORY
    27Aug89    Hardcoded SSL processor written in C
    31Aug89    SSL processor rewritten in SSL
    02Sep89    Wrote table walker shell and SSL instructions
    07Sep89    Finished implementing semantic operations
    09Sep89    Produce optional listing file
    18Oct89    Added 'title' section
    02Feb90    Added debugger output (line # table for now)
    21Feb90    Added hex constants (0xABCD)
    20Mar91    Increased size of string shortform table
    26Mar91    Fixed bug in handling of statement ">>value".
               Failed inside choice due to stuff on IS stack above the
               rule id, put there by choice.  Fixed with new mechanism
               "oIdentISPushBottom".  (KLUDGE!)
    24Apr91    Increased id size from 31 chars to 50 chars
               Increased patch stack sizes
               Increased output table size
               Make ssl source case sensitive, but optionally insensitive
    04May91    Increased id table size from 300 to 600.
               Moved id names and string shortforms into separately allocated
               memory, and changed length limit from 50 to 256 chars.
               Added -c option to generate C code for the table.
               Use the automatically generated C code table in this program.
    05May91    Added "include" feature.
    21May91    Added mechanism next_error.
               Added table optimization: reduce chains of jumps into one jump.
    04Jun91    Write rule addresses to code file.

*/

int      case_sensitive = 1;


/*  Define USE_C_TABLE to include C version of code table (in ssl.tbl).
 *  Undefine it to read the table from the file ssl.tbl.
 *  (In the second case, the data is just a dump of integers, no commas)
 *  (I should ALWAYS generate C form, but allow readers to read the
 *  C form at run time if desired).
 */

#define  USE_C_TABLE



#include <stdio.h>
#include <string.h>

#ifdef AMIGA
#include <dos.h>
#endif AMIGA

#define  SSL_INCLUDE_ERR_TABLE
#include "ssl.h"

/* I only care about this if I want to read at run time */
#define TABLE_FILE "ssl.tbl"   /* This is the ssl code to execute */


/* The scanner is hardcoded in C.

   Scanner variables start with 't_'.
   Table walker variables start with 'w_'.
   Convention: 
     Semantic operations start with 'o'.
     Corresponding data structures start with 'd'.

   The code to walk is in w_codeTable (pointer=w_pc).
   The output buffer is w_outTable (pointer=w_here).

   A token is read and screened when oInput, oInputAny, or oInputChoice
   is encountered.  When an id is accepted, set t_lastId to its code.

   The various stacks here don't use the [0] entry.  The stack variable
   points to the top entry.  Overflow is always checked; underflow is
   never checked.

   The first line in the table file is the title (version) of the processor.
   The next word in the table file is the #entries.  The table can be
   modified and reused without having to recompile the corresponding
   semantic operations code, as long as 1) only the rules/title are changed
   (not the inputs, outputs, types, errors, or mechanisms), and
   2) the size of the table does not exceed the space compiled for it
   (initially, a margin of 50 unused words is allocated).  This second
   condition is checked automatically, but (1) is not.
*/

#ifdef   USE_C_TABLE

#include "ssl.tbl"

#else    USE_C_TABLE

short    w_codeTable [w_codeTableSize];     /* size defined in .h file */

#endif   USE_C_TABLE


FILE *t_src,*t_out,*t_hdr,*t_doc,*t_lst,*t_dbg;
FILE *t_src_includer;

int     t_including_file;

#define t_lineSize 256
#define t_bufferSize 256           /* also used for strlits */
#define t_idSize (t_bufferSize-1)
#define t_idTableSize 600

char   t_lineBuffer[t_lineSize];
char   *t_ptr;
short  t_lineNumber;
char   t_lineListed;
short  t_lastId;                   /* code of last id accepted */

/* user options */

char   t_listing;                  /* do we want a listing file? */
char   t_debugOut;                 /* do we want debugger output? */
char   t_generate_C;               /* generate a table in C code? */
char   option_optimize;       /* perform optimization postproces? */
char   option_verbose;        /* verbose output */

/* token kinds and id kinds are declared in the SSL code, not here */

char t_upper[256],t_blank[256],t_idStart[256],t_idCont[256];
char t_digit[256];
short t_punct[256];

struct t_tokenType {
  char accepted;
  char buffer[t_bufferSize+1];     /* used to hold StrLits (and ids) */
  short buflen;
  short val;                       /* user id code, int lit, etc */
  short kind;                      /* pIdent, pIntLit, keyword code, etc */
  short lineNumber;
} t_token;

struct t_idTableType {
  char  *buffer;           /* name is stored in separately allocated memory */
  short buflen;
  short kind;
  short idKind;
  char declared;
  char choice;
  char param;
  short val;                       /* code #, or rule address */
  short idType;
  short paramType;
} t_idTable[t_idTableSize];
short t_idTableNext;

#define t_getToken   if (t_token.accepted) t_getNextToken

/* Variables for table walker */

#define w_stackSize 200
short w_stack[w_stackSize],w_sp;   /* Call stack */
short w_pc;                        /* Program counter */
short w_result, w_param;           /* For oSetResult, oSetParam */
short w_options;                   /* # choice options */
char w_errBuffer[256];             /* build up err messages here */

#ifndef USE_C_TABLE
char w_title_string[t_lineSize];   /* Title of this SSL processor */
#endif  USE_C_TABLE

char w_targetTitle[t_lineSize];    /* Title of processor being compiled */

#define w_outTableSize 5000
short w_outTable[w_outTableSize];  /* Emit into this table */
short w_here;                      /* Output position */
short w_errorCount;                /* Number of error signals emitted */
#define w_lineTableSize 3000       /* Max # lines for debug table */
short w_lineTable[w_lineTableSize];/* Line# of every instruction */

/*************** Variables for semantic mechanisms ***************/

short dTemp;                       /* multi-purpose */

#define dCSsize 30
short dCS[dCSsize], dCSptr;        /* count stack */

short dNextError;                  /* for mechanism next_error */

#define dVSsize 30
short dVS[dVSsize], dVSptr;        /* value stack */

#define dISsize 30
short dIS[dISsize], dISptr;

#define dStrTableSize 100
struct dStrTableType {
  char *buffer;                   /* text is stored in separately allocated memory */
  short val;
} dStrTable[dStrTableSize];
short dStrTableNext;

/* code patch stacks */

#if 0    /* These are the original values -- 24apr91 */
#define dMarkSize 10               /* marks in the following patch stacks */
#define dPatchCTAsize 10           /*     (not all the stacks need marks) */
#define dPatchCTsize 200
#define dPatchCEsize 100
#define dPatchCsize 200
#define dPatchLsize 10
#define dPatchBsize 30
#endif 0

/* These are the new values -- 24apr91 */

#define dMarkSize 20               /* marks in the following patch stacks */
#define dPatchCTAsize 20           /*     (not all the stacks need marks) */
#define dPatchCTsize 500
#define dPatchCEsize 300
#define dPatchCsize 800
#define dPatchLsize 20
#define dPatchBsize 50


short dPatchCTA[dPatchCTAsize];    /* choice table addr ptrs */
short dPatchCT[dPatchCTsize];      /* choice table built here */
short dPatchCE[dPatchCEsize];      /* choice exits */
short dPatchC[dPatchCsize];        /* calls to undefined rules */
short dPatchL[dPatchLsize];        /* start of loop */
short dPatchB[dPatchBsize];        /* breaks out of loop */

/* array pointing to patch stacks */

struct dPSType {
  short *stack;
  short ptr;
  short size;
  short mark[dMarkSize];
  short markPtr;
} dPS[6];

/* ----------------------------------------------------------------- */


/* ----------------------------------------------------------------- */


main(argc,argv)
int argc;
char *argv[];
{
  int t_hitBreak();
  short arg;
  char *p;

  /* Prepare Files */

  t_listing = 0;
  t_debugOut = 0;
  t_generate_C = 0;
  option_optimize = 0;
  option_verbose = 0;

  for (arg=1; arg<argc; arg++) {
    if (*argv[arg] != '-')
      break;
    for (p=argv[arg]+1; *p; p++) {
      if (*p=='l')
        t_listing = 1;
      else if (*p=='d')
        t_debugOut = 1;
      else if (*p=='c')
        t_generate_C = 1;
      else if (*p=='o')
        option_optimize = 1;
      else if (*p=='v')
        option_verbose = 1;
      else {
        printf ("Unknown option '%c'\n", *p);
        exit (10);
      }
    }
  }
  if (arg>=argc) {
    printf("Usage:  ssl [-l] [-d] [-o] [-c] [-v] file\n");
    printf("        -l: produce listing file\n");
    printf("        -d: produce debugging file\n");
    printf("        -o: optimize code table\n");
    printf("        -c: produce C code for table\n");
    printf("        -v: verbose\n");
    exit(10);
  }

#ifndef USE_C_TABLE
    read_table (argv[0]);
#endif USE_C_TABLE

  sprintf(t_lineBuffer,"%s.ssl",argv[arg]);
  if((t_src=fopen(t_lineBuffer,"r"))==NULL) {
    printf("Can't open source file %s\n",t_lineBuffer);
    exit(10);
  }
  sprintf(t_lineBuffer,"ram_%s.tbl",argv[arg]);
  if((t_out=fopen(t_lineBuffer,"w"))==NULL) {
    printf("Can't open output file %s\n",t_lineBuffer);
    exit(10);
  }
  sprintf(t_lineBuffer,"ram_%s.h",argv[arg]);
  if((t_hdr=fopen(t_lineBuffer,"w"))==NULL) {
    printf("Can't open output file %s\n",t_lineBuffer);
    exit(10);
  }
  sprintf(t_lineBuffer,"ram_%s.doc",argv[arg]);
  if((t_doc=fopen(t_lineBuffer,"w"))==NULL) {
    printf("Can't open output file %s\n",t_lineBuffer);
    exit(10);
  }
  sprintf(t_lineBuffer,"ram_%s.lst",argv[arg]);
  if (t_listing) {
    if((t_lst=fopen(t_lineBuffer,"w"))==NULL) {
      printf("Can't open output file %s\n",t_lineBuffer);
      exit(10);
    }
  }
  sprintf(t_lineBuffer,"ram_%s.dbg",argv[arg]);
  if (t_debugOut) {
    if((t_dbg=fopen(t_lineBuffer,"w"))==NULL) {
      printf("Can't open output file %s\n",t_lineBuffer);
      exit(10);
    }
  }

  printf("%s\n", w_title_string);

#ifdef AMIGA
  onbreak(&t_hitBreak);
#endif AMIGA

  t_initScanner();

  w_pc = 0;                        /* Initialize walker */
  w_sp = 0;
  w_here = 0;
  w_errorCount = 0;

  w_initSemanticOperations();

  w_walkTable();

  if (w_errorCount)
    printf("\nSSL: %d error(s)\n",w_errorCount);

  if (option_optimize && (w_errorCount == 0))
    w_optimize_table();

  t_cleanup();

  if (w_errorCount)
    exit (1);
  else
    exit (0);
}


/*  Read ssl code (if not using the USE_C_TABLE option)  */

read_table (progname)
char       *progname;
{
  int entries,temp;                /* I can't seem to read a 'short' */

  if((t_src=fopen(TABLE_FILE,"r"))==NULL) {
    printf("Can't open table file %s\n",TABLE_FILE);
    exit(10);
  }
  
  fgets(w_title_string,t_lineSize,t_src);
  fscanf(t_src,"%d",&entries);
  if (entries>w_codeTableSize) {
    printf("Table file too big -- recompile %s.c\n", progname);
    exit(10);
  }

  for (w_pc=0; w_pc<entries; w_pc++) {
    fscanf(t_src,"%d",&temp);
    w_codeTable[w_pc] = temp;
  }
  fclose(t_src);
}

/*********** S e m a n t i c   O p e r a t i o n s ***********/

w_errorSignal(err)
short err;
{
  short i;
  i = err;
  if (w_errTable[i].val != i)
    for (i=0; i<w_errTableSize && w_errTable[i].val != err; i++);
  sprintf(w_errBuffer,"Error #%d (%s)",err,w_errTable[i].msg);
  t_error(w_errBuffer);
}

w_initSemanticOperations()
{
  /* link up patch stacks */

  dPS[patchChoiceTableAddr].stack = dPatchCTA;
  dPS[patchChoiceTableAddr].size = dPatchCTAsize;
  dPS[patchChoiceTable].stack = dPatchCT;
  dPS[patchChoiceTable].size = dPatchCTsize;
  dPS[patchChoiceExit].stack = dPatchCE;
  dPS[patchChoiceExit].size = dPatchCEsize;
  dPS[patchCall].stack = dPatchC;
  dPS[patchCall].size = dPatchCsize;
  dPS[patchLoop].stack = dPatchL;
  dPS[patchLoop].size = dPatchLsize;
  dPS[patchBreak].stack = dPatchB;
  dPS[patchBreak].size = dPatchBsize;

  strcpy(w_targetTitle,"Compiling");    /* default title */
}


w_walkTable()
{
   while (1) {
     switch (w_codeTable[w_pc++]) {

       /* SSL Instructions */

       case oJumpForward :
              w_pc += w_codeTable[w_pc];
              continue;
       case oJumpBack :
              w_pc -= w_codeTable[w_pc];
              continue;
       case oInput :
              t_getToken();
              if (t_token.kind != w_codeTable[w_pc++])
                t_error ("SSL: Syntax error");
              accept_token();
              continue;
       case oInputAny :
              t_getToken();
              accept_token();
              continue;
       case oEmit :
              dTemp = w_codeTable[w_pc++];
              switch (dTemp) {
                case iConstant :   w_outTable[w_here++] = dVS[dVSptr];
                                   continue;
                case iIdentVal :   w_outTable[w_here++] =
                                       t_idTable[t_lastId].val;
                                   continue;
                case iIdentISVal : w_outTable[w_here++] =
                                       t_idTable[dIS[dISptr]].val;
                                   continue;
                default :          w_outTable[w_here++] = dTemp;
                                   continue;
              }
       case oError :
              w_errorSignal(w_codeTable[w_pc++]);
              continue;
       case oInputChoice :
              t_getToken();
              w_pc += w_codeTable[w_pc];
              w_options = w_codeTable[w_pc++];
              while (w_options--) {
                if (w_codeTable[w_pc++] == t_token.kind) {
                  w_pc -= w_codeTable[w_pc];
                  accept_token();
                  break;
                }
                w_pc++;
              }
              continue;
       case oCall :
              if (++w_sp==w_stackSize) t_fatal ("SSL: call stack overflow");
              w_stack[w_sp] = w_pc+1;
              w_pc = w_codeTable[w_pc];
              continue;
       case oReturn :
              if (w_sp) {
                w_pc = w_stack[w_sp--];
                continue;
              } else
                return;            /* done walking table */
       case oSetResult :
              w_result = w_codeTable[w_pc++];
              continue;
       case oChoice :
              w_pc += w_codeTable[w_pc];
              w_options = w_codeTable[w_pc++];
              while (w_options--) {
                if (w_codeTable[w_pc++] == w_result) {
                  w_pc -= w_codeTable[w_pc];
                  break;
                }
                w_pc++;
              }
              continue;
       case oEndChoice :           /* choice or inputChoice didn't match */
              t_error ("SSL: syntax error");
              t_token.accepted = 1;   /* need full err recovery */
              continue;
       case oSetParameter :
              w_param = w_codeTable[w_pc++];
              continue;

       /* Mechanism count */

       case oCountPush :
              if (++dCSptr==dCSsize) t_fatal("CS overflow");
              dCS[dCSptr] = w_param;
              continue;
       case oCountPushIntLit :
              if (++dCSptr==dCSsize) t_fatal("CS overflow");
              dCS[dCSptr] = t_token.val;
              continue;
       case oCountPop :
              dCSptr--;
              continue;
       case oCountInc :
              dCS[dCSptr]++;
              continue;
       case oCountDec :
              dCS[dCSptr]--;
              continue;
       case oCountZero :
              w_result = !dCS[dCSptr];
              continue;

       /* Mechanism next_error */

       case oNextErrorPushCount :
              if (++dCSptr==dCSsize) t_fatal("CS overflow");
              dCS[dCSptr] = dNextError;
              continue;
       case oNextErrorPopCount :
              dNextError = dCS[dCSptr--];
              continue;

       /* Mechanism value */

       case oValuePushKind :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = w_param;
              continue;
       case oValuePushVal :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = t_idTable[t_lastId].val;
              continue;
       case oValuePushISVal :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = t_idTable[dIS[dISptr]].val;
              continue;
       case oValuePushIdent :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = t_lastId;
              continue;
       case oValuePushType :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = t_idTable[t_lastId].idType;
              continue;
       case oValueChooseKind :
              w_result = dVS[dVSptr];
              continue;
       case oValuePushCount :
              if (++dVSptr==dVSsize) t_fatal("VS overflow");
              dVS[dVSptr] = dCS[dCSptr];
              continue;
       case oValueSwap :
              dTemp = dVS[dVSptr];
              dVS[dVSptr] = dVS[dVSptr-1];
              dVS[dVSptr-1] = dTemp;
              continue;
       case oValuePop :
              dVSptr--;
              continue;

       /* Mechanism ident */

       case oIdentSetDeclared :
              if (t_idTable[t_lastId].declared)
                t_error("Name already declared");
              t_idTable[t_lastId].declared = 1;
              continue;
       case oIdentSetKind :
              t_idTable[t_lastId].idKind = w_param;
              continue;
       case oIdentSetKindVS :
              t_idTable[t_lastId].idKind = dVS[dVSptr];
              continue;
       case oIdentSetType :
              t_idTable[t_lastId].idType = dVS[dVSptr];
              continue;
       case oIdentSetValCount :
              t_idTable[t_lastId].val = dCS[dCSptr];
              continue;
       case oIdentSetValHere :
              t_idTable[t_lastId].val = w_here;
              continue;
       case oIdentSetChoice :
              t_idTable[t_lastId].choice = w_param;
              continue;
       case oIdentChooseKind :
              w_result = t_idTable[t_lastId].idKind;
              continue;
       case oIdentChooseParam :
              w_result = t_idTable[t_lastId].param;
              continue;
       case oIdentChooseChoice :
              w_result = t_idTable[t_lastId].choice;
              continue;
       case oIdentChooseDeclared :
              w_result = t_idTable[t_lastId].declared;
              continue;
       case oIdentMatchType :
              w_result = t_idTable[t_lastId].idType ==
                         t_idTable[dIS[dISptr]].idType;
              continue;
       case oIdentMatchParamType :
              w_result = t_idTable[t_lastId].idType ==
                         t_idTable[dIS[dISptr]].paramType;
              continue;
       case oIdentISPush :
              if (++dISptr==dISsize) t_fatal("IS overflow");
              dIS[dISptr] = t_lastId;
              continue;
       case oIdentISPushBottom :
              if (++dISptr==dISsize) t_fatal("IS overflow");
              dIS[dISptr] = dIS[1];
              continue;
       case oIdentISPop :
              dISptr--;
              continue;
       case oIdentSetISParamType :
              t_idTable[dIS[dISptr]].paramType = dVS[dVSptr];
              continue;
       case oIdentSetISParam :
              t_idTable[dIS[dISptr]].param = 1;
              continue;
       case oIdentSetISChoice :
              t_idTable[dIS[dISptr]].choice = w_param;
              continue;
       case oIdentSetISType :
              t_idTable[dIS[dISptr]].idType = dVS[dVSptr];
              continue;
       case oIdentChooseISKind :
              w_result = t_idTable[dIS[dISptr]].idKind;
              continue;
       case oIdentChooseISChoice :
              w_result = t_idTable[dIS[dISptr]].choice;
              continue;
       case oIdentChooseISParam :
              w_result = t_idTable[dIS[dISptr]].param;
              continue;
       case oIdentMatchISType :
              w_result = t_idTable[dIS[dISptr-1]].idType ==
                         t_idTable[dIS[dISptr]].idType;
              continue;

       /* Mechanism shortForm */

       case oShortFormAdd :
              if (dStrTableNext==dStrTableSize)
                t_fatal("String table overflow");
              dStrTable[dStrTableNext].buffer = (char *) malloc (strlen(t_token.buffer)+1);
              strcpy(dStrTable[dStrTableNext].buffer,t_token.buffer);
              dStrTable[dStrTableNext++].val = t_lastId;
              continue;
       case oShortFormLookup :
              if (++dISptr==dISsize) t_fatal("IS overflow");
              dIS[dISptr] = 0;
              for (dTemp=0; dTemp<dStrTableNext; dTemp++)
                if (!strcmp(t_token.buffer,dStrTable[dTemp].buffer)) {
                  dIS[dISptr] = dStrTable[dTemp].val;
                  break;
                }
              if (!dIS[dISptr])
                t_error("String shortform not defined");
              continue;

       /* Mechanism patch */

       case oPatchMark :
              if (++dPS[w_param].markPtr==dMarkSize)
                    t_fatal("mark overflow");
              dPS[w_param].mark[dPS[w_param].markPtr] = dPS[w_param].ptr;
              continue;
       case oPatchAtMark :
              w_result = dPS[w_param].mark[dPS[w_param].markPtr] ==
                         dPS[w_param].ptr;
              if (w_result)
                dPS[w_param].markPtr--;
              continue;
       case oPatchPushHere :
              if (++dPS[w_param].ptr==dPS[w_param].size)
                    t_fatal("patch overflow");
              dPS[w_param].stack[dPS[w_param].ptr] = w_here;
              continue;
       case oPatchPushIdent :
              if (++dPS[w_param].ptr==dPS[w_param].size)
                    t_fatal("patch overflow");
              dPS[w_param].stack[dPS[w_param].ptr] = t_lastId;
              continue;
       case oPatchPushValue :
              if (++dPS[w_param].ptr==dPS[w_param].size)
                    t_fatal("patch overflow");
              dPS[w_param].stack[dPS[w_param].ptr] = dVS[dVSptr];
              continue;
       case oPatchAnyEntries :
              w_result = dPS[w_param].ptr > 0;
              continue;              
       case oPatchPopFwd :
              dTemp = dPS[w_param].stack[dPS[w_param].ptr--];
              w_outTable[dTemp] = w_here - dTemp;
              continue;
       case oPatchPopBack :
              w_outTable[w_here] = w_here -
                    dPS[w_param].stack[dPS[w_param].ptr--];
              w_here++;
              continue;
       case oPatchPopValue :
              w_outTable[w_here++] = dPS[w_param].stack[dPS[w_param].ptr--];
              continue;
       case oPatchPopCall :
              dTemp = dPS[w_param].stack[dPS[w_param].ptr--];
              w_outTable[dPS[w_param].stack[dPS[w_param].ptr--]] =
                    t_idTable[dTemp].val;
              continue;

       /* Mechanism titleMech */

       case oTitleSet :
              strcpy(w_targetTitle,t_token.buffer);
              continue;

       /* Mechanism doc */

       case oDocNewRule :
              if (option_verbose)
                  printf("Rule %s\n",t_token.buffer);
              fprintf(t_doc,"Rule %s\n",t_token.buffer);
              continue;
       case oDocCheckpoint :
              printf("Checkpoint %d (sp=%d) ",w_pc-1,w_sp);
              fprintf(t_doc,"Checkpoint %d (sp=%d) ",w_pc-1,w_sp);
              t_printToken();
              continue;

       /* Mechanism include_mech */

       case oInclude :
              if (t_including_file)
                  t_fatal ("Nested includes are not yet supported");
              t_including_file = 1;
              t_src_includer = t_src;
              t_src = fopen (t_token.buffer, "r");
              if (t_src == NULL)
              {
                  sprintf (w_errBuffer, "Can't open include file '%s'", t_token.buffer);
                  t_fatal (w_errBuffer);
              }
              t_lineBuffer[0] = '\0';
              t_ptr = t_lineBuffer;
              t_lineListed = 1;
              continue;

       default:
              w_pc--;
              sprintf(w_errBuffer,"SSL: Bad instruction #%d at %d",
                      w_codeTable[w_pc],w_pc);
              t_fatal(w_errBuffer);
     } /* switch */
   }
}

accept_token()
{
  t_token.accepted = 1;
  if (!t_lineListed) {
    t_lineListed = 1;
    if (t_token.lineNumber > w_lineTableSize)
      t_fatal ("Too many source lines for debug line table");
    w_lineTable[t_token.lineNumber] = w_here;
    if (t_listing)
      fprintf(t_lst,"%4d: %s",w_here,t_lineBuffer);
  }
  if (t_token.kind == pIdent)
    t_lastId = t_token.val;
}

w_traceback()
{
  short i;
  printf("SSL Traceback:\n");
  fprintf(t_doc,"SSL Traceback:\n");
  printf("  %d",w_pc);
  fprintf(t_doc,"  %d",w_pc);
  for (i=w_sp;i>0;i--) {
    printf("  called from\n  %d",w_stack[i]-2);
    fprintf(t_doc,"  called from\n  %d",w_stack[i]-2);
  }
  printf("\n");
  fprintf(t_doc,"\n");
}

/************************ S c a n n e r *********************/

t_initScanner ()
{
  register short i;
  short firstOp,op,opVal;

  /* C guarantees static vars to start with 0, so here I don't
       bother to initialize all of blank,idStart,idCont,digit */

  for (i=0;i<256;i++)
    t_upper[i] = i;
  for (i='a';i<='z';i++) {
    t_upper[i] = i - ('a'-'A');
    t_idStart[i] = 1;
    t_idCont[i] = 1;
  }
  for (i='A';i<='Z';i++) {
    t_idStart[i] = 1;
    t_idCont[i] = 1;
  }
  t_idStart['_'] = 1;
  t_idCont['_'] = 1;
  for (i='0';i<='9';i++) {
    t_digit[i] = 1;
    t_idCont[i] = 1;
  }
  t_blank[' '] = 1;
  t_blank['\t'] = 1;
  t_blank['\n'] = 1;

  /* single-character tokens */
  for (i=0;i<256;i++)
    t_punct[i] = pInvalid;
  t_punct['{'] = pLCurly;
  t_punct['}'] = pRCurly;
  t_punct['['] = pLSquare;
  t_punct[']'] = pRSquare;
  t_punct['|'] = pBar;
  t_punct['@'] = pCall;
  t_punct['*'] = pStar;
  t_punct['?'] = pQuestion;
  t_punct['.'] = pEmit;
  t_punct['='] = pEquals;
  t_punct[':'] = pColon;
  t_punct[';'] = pSemiColon;
  t_punct[','] = pComma;
  t_punct['('] = pLParen;
  t_punct[')'] = pRParen;
  t_punct['#'] = pErr;

  t_lineBuffer[0] = '\0';
  t_ptr = t_lineBuffer;
  t_lineNumber = 0;
  t_lineListed = 1;
  t_idTableNext = 0;
  t_token.kind = pColon;           /* dummy */
  t_token.accepted = 1;
  t_addId("title",pTitle);
  t_addId("input",pInput);
  t_addId("output",pOutput);
  t_addId("type",pType);
  t_addId("error",pError);
  t_addId("include",pInclude);
  t_addId("mechanism",pMechanism);
  t_addId("rules",pRules);
  t_addId("end",pEnd);
  /* add basic operations */
  firstOp = t_idTableNext;
  t_addId("oJumpForward",pIdent);
  t_addId("oJumpBack",pIdent);
  t_addId("oInput",pIdent);
  t_addId("oInputAny",pIdent);
  t_addId("oEmit",pIdent);
  t_addId("oError",pIdent);
  t_addId("oInputChoice",pIdent);
  t_addId("oCall",pIdent);
  t_addId("oReturn",pIdent);
  t_addId("oSetResult",pIdent);
  t_addId("oChoice",pIdent);
  t_addId("oEndChoice",pIdent);
  t_addId("oSetParameter",pIdent);
  t_addId("oBreak",pIdent);
  opVal = 0;
  for (op=firstOp;op<t_idTableNext;op++) {
    t_idTable[op].val = opVal++;
    t_idTable[op].idKind = kOp;
    t_idTable[op].declared = 1;    /* other fields 0 */
  }

  t_including_file = 0;
}  

t_getNextToken()
{
  char more,*p,t_readln();
  short lit;

  t_token.accepted = 0;

  /* error if we try to read past the end of file */
  if (t_token.kind == pEof)              /* last token was eof */
    t_fatal ("Unexpected end of file"); 

  /* skip to start of token */

  while (1) {
    while (t_blank[*t_ptr])
      t_ptr++;
    if (*t_ptr=='\0') {
      if (!t_lineListed) {
        w_lineTable[t_lineNumber] = -1;
        if (t_listing)
          fprintf(t_lst,"      %s",t_lineBuffer);
      }
      more = t_readln(t_lineBuffer);
      if (!more) { t_hit_Eof(); return; }
      t_ptr = t_lineBuffer;
      t_lineNumber++;
      t_lineListed = 0;
    } else if (*t_ptr=='%') {
      if (!t_lineListed) {
        w_lineTable[t_lineNumber] = -1;
        if (t_listing)
          fprintf(t_lst,"      %s",t_lineBuffer);
      }
      more = t_readln(t_lineBuffer);
      if (!more) {  t_hit_Eof(); return;  }
      t_ptr = t_lineBuffer;
      t_lineNumber++;
      t_lineListed = 0;
    } else {
      break;
    }
  }

  /* copy token, screen ids and integers */

  t_token.lineNumber = t_lineNumber;
  p = t_token.buffer;
  if (t_idStart[*t_ptr]) {
    while (t_idCont[*t_ptr] && p-t_token.buffer < t_idSize)
      *p++ = *t_ptr++;
    *p = '\0';
    t_token.buflen = p-t_token.buffer;
    t_token.kind = pIdent;
    t_lookupId();
  } else if (*t_ptr=='0' && *(t_ptr+1)=='x') {   /* hex number */
    t_ptr += 2;
    lit = 0;
    while (1) {
      if (t_digit[*t_ptr])
        lit = lit*16 - '0' + *t_ptr;
      else if (*t_ptr >= 'A' && *t_ptr <= 'F')
        lit = lit*16 - 'A' + *t_ptr + 10;
      else if (*t_ptr >= 'a' && *t_ptr <= 'f')
        lit = lit*16 - 'a' + *t_ptr + 10;
      else
        break;
      *p++ = *t_ptr++;
    }
    *p = '\0';
    t_token.val = lit;
    t_token.kind = pIntLit;
  } else if (t_digit[*t_ptr]) {
    lit = 0;
    while (t_digit[*t_ptr]) {
      lit = lit*10 - '0' + *t_ptr;
      *p++ = *t_ptr++;
    }
    *p = '\0';
    t_token.val = lit;
    t_token.kind = pIntLit;
  } else if (*t_ptr == '-') {
    t_ptr++;
    lit = 0;
    while (t_digit[*t_ptr]) {
      lit = lit*10 - '0' + *t_ptr;
      *p++ = *t_ptr++;
    }
    *p = '\0';
    t_token.val = -lit;
    t_token.kind = pIntLit;
  } else if (*t_ptr=='\'') {
    t_ptr++;
    t_token.kind = pStrLit;
    while (p-t_token.buffer < t_bufferSize) {
      if (*t_ptr=='\'') {
        if (*++t_ptr=='\'') {
          *p++ = '\'';
          t_ptr++;
        } else {
          break;
        }
      } else if (*t_ptr=='\0') {
        break;
      } else {
        *p++ = *t_ptr++;
      }
    }
    *p = '\0';
    t_token.buflen = p-t_token.buffer;
  } else if (*t_ptr=='>') {
    t_ptr++;
    if (*t_ptr=='>') {
      t_ptr++;
      t_token.kind = pReturn;
    } else {
      t_token.kind = pBreak;
    }
  } else {
    t_token.kind = t_punct[*t_ptr++];
    if (t_token.kind == pInvalid) {
      --t_ptr;
      t_token.buffer[0] = *t_ptr++;
      t_token.buffer[1] = '\0';
      t_error ("SSL: invalid character");
    }
  }
}

t_hit_Eof ()
{
    if (t_including_file)
    {
        fclose (t_src);
        t_including_file = 0;
        t_src = t_src_includer;
        t_lineBuffer[0] = '\0';
        t_ptr = t_lineBuffer;
        t_lineListed = 1;
        t_getNextToken();
    }
    else
    {
        t_token.kind = pEof;
    }
}


t_addId(buffer,kind)
char *buffer;
short kind;
{
  short  name_length;

  name_length = strlen (buffer);
  t_idTable[t_idTableNext].buflen = name_length;
  t_idTable[t_idTableNext].buffer = (char *) malloc (name_length+1);
  strcpy(t_idTable[t_idTableNext].buffer,buffer);
  t_idTable[t_idTableNext].kind = kind;
  t_idTable[t_idTableNext].idKind = kUnknown;  /* for user-id's */
  if (++t_idTableNext >= t_idTableSize)
    t_fatal("identifier table overflow");
}

t_lookupId()
{
  short i;
  register char *a,*b;

  if (case_sensitive)
  {
    for (i=0;i<t_idTableNext;i++) {
      if (t_token.buflen==t_idTable[i].buflen) {
        a = t_token.buffer;
        b = t_idTable[i].buffer;
        while (*a++ == *b++) {
          if (!*a) {   /* match */
            t_token.kind = t_idTable[i].kind;  /* for built-in ids */
            t_token.val = i;                   /* for user-defined ids */
            return;
          }
        }
      }
    }
  }
  else /* case insensitive */
  {
    for (i=0;i<t_idTableNext;i++) {
      if (t_token.buflen==t_idTable[i].buflen) {
        a = t_token.buffer;
        b = t_idTable[i].buffer;
        while (t_upper[*a++]==t_upper[*b++]) {
          if (!*a) {   /* match */
            t_token.kind = t_idTable[i].kind;  /* for built-in ids */
            t_token.val = i;                   /* for user-defined ids */
            return;
          }
        }
      }
    }
  }

  t_token.val = t_idTableNext;
  t_addId(t_token.buffer,pIdent);
}

char t_readln(lineBuffer)
char *lineBuffer;
{
  char *ptr;
  ptr = fgets(lineBuffer,t_lineSize,t_src);
  if (ptr==NULL)
    return(0);
  return(1);
}

t_hitBreak()
{
  printf("Breaking...\n");
  t_cleanup();
  return(1);
}

t_cleanup()
{
  t_dumpTables();
  fclose(t_src);
  fclose(t_out);
  fclose(t_hdr);
  fclose(t_doc);
  if (t_listing) fclose(t_lst);
  if (t_debugOut) fclose(t_dbg);
}

t_fatal(msg)
char *msg;
{
  printf("[FATAL] ");
  fprintf(t_doc,"[FATAL] ");
  t_error (msg);
  w_traceback();
  t_cleanup();
  exit(-3);
}

t_printToken()
{
  printf("(Token=");
  fprintf(t_doc,"(Token=");
  if (t_token.kind==pIdent) {
    printf("%s)\n",t_token.buffer);
    fprintf(t_doc,"%s)\n",t_token.buffer);
  } else if (t_token.kind==pStrLit || t_token.kind==pInvalid) {
    printf("'%s')\n",t_token.buffer);
    fprintf(t_doc,"'%s')\n",t_token.buffer);
  } else {
    printf("<%d>)\n",t_token.kind);
    fprintf(t_doc,"<%d>)\n",t_token.kind);
  }
}

t_error(msg)
char *msg;
{
  printf("%s on line %d ",msg,t_token.lineNumber);
  fprintf(t_doc,"%s on line %d ",msg,t_token.lineNumber);
  t_printToken();
  w_errorCount++;
  printf ("SRICE: error detected near PC=%d\n", w_pc);
}

t_dumpTables()
{
  short i,count;
  struct rule_addr {
      char *name;
      short addr;
      struct rule_addr *next;
  };
  struct rule_addr *head, *curr, *prev, *new_rule;
  int rule_table_size;
  
  printf("\nWriting Files...\n");

  /* Build list of rule addresses, sorted by address (insertion sort) */

  rule_table_size = 0;
  head = NULL;
  for (i = 0; i < t_idTableNext; i++)
  {
      if (t_idTable[i].kind == pIdent &&
          t_idTable[i].idKind == kRule)
      {
          new_rule = (struct rule_addr *) malloc (sizeof(struct rule_addr));
          new_rule->name = t_idTable[i].buffer;
          new_rule->addr = t_idTable[i].val;
          new_rule->next = NULL;
          rule_table_size++;
          if (head == NULL)
              head = new_rule;
          else
          {
              curr = head;
              prev = NULL;
              while (curr != NULL && curr->addr < new_rule->addr)
              {
                  prev = curr;
                  curr = curr->next;
              }
              prev->next = new_rule;
              new_rule->next = curr;
          }
      }
  }

#if 0
  fprintf(t_doc,"Id table:\n\n");
  for (i=0; i<t_idTableNext; i++) {
    fprintf(t_doc,"%d: %s\n",i,t_idTable[i].buffer);
    fprintf(t_doc,"    iKind=%d val=%d dcl=%d >>=%d typ=%d ()=%d pTyp=%d\n",
            t_idTable[i].idKind,
            t_idTable[i].val,
            t_idTable[i].declared,
            t_idTable[i].choice,
            t_idTable[i].idType,
            t_idTable[i].param,
            t_idTable[i].paramType);
  }
  fprintf(t_doc,"\nString shortform table:\n\n");
  for (i=0; i<dStrTableNext; i++)
    fprintf(t_doc,"%s  '%s'\n",t_idTable[dStrTable[i].val].buffer,
                               dStrTable[i].buffer);
#endif 0


  /*  Write table  */

    if (t_generate_C)
    {
        fprintf (t_out, "/* Automatically generated by ssl */\n\n");
        fprintf (t_out, "char *w_title_string = \"%s\";\n\n", w_targetTitle);
        fprintf (t_out, "short w_codeTable[] = {\n\t");
        for (i = 0; i < w_here; i++)
        {
            fprintf (t_out, "%d, ", w_outTable[i]);
            if ((i & 15) == 15)
                fprintf (t_out, "\n\t");
        }
        fprintf (t_out, "\n};\n\n");
        /* output rule addresses */
        fprintf (t_out, "int w_ruleTableSize = %d;\n", rule_table_size);
        fprintf (t_out, "struct {\n  char *name;\n  short addr;\n} w_ruleTable[] = {\n");
        for (curr = head; curr != NULL; curr = curr->next)
        {
            fprintf (t_out, "\t{\"%s\",\t%d},\n", curr->name, curr->addr);
        }
        fprintf (t_out, "};\n");
    }
    else
    {
        /* output code */
        fprintf (t_out, "%s\n%d\n\n", w_targetTitle, w_here);
        for (i=0; i<w_here; i++)
            fprintf (t_out, "%d\n", w_outTable[i]);
        /* output rule addresses */
        fprintf (t_out, "%d\n", rule_table_size);
        for (curr = head; curr != NULL; curr = curr->next)
        {
            fprintf (t_out, "%s\t%d\n", curr->name, curr->addr);
        }
    }

  /*  Write debug file  */

  if (t_debugOut) {
    fprintf(t_dbg,"%d\n",t_lineNumber);
    for (i=1; i<=t_lineNumber; i++)
      fprintf(t_dbg,"%d\n",w_lineTable[i]);
  }

  /*  Write header file  */

  fprintf(t_hdr,"#define w_codeTableSize %d\n\n",w_here+50);
  for (i=0; i<t_idTableNext; i++)
    if (t_idTable[i].kind == pIdent &&
        t_idTable[i].idKind != kType &&
        t_idTable[i].idKind != kMech &&
        t_idTable[i].idKind != kRule)
      fprintf(t_hdr,"#define %s %d\n",
              t_idTable[i].buffer,
              t_idTable[i].val);
  count = 0;
  fprintf(t_hdr,"\n#ifdef SSL_INCLUDE_ERR_TABLE\n");
  fprintf(t_hdr,"\nstruct w_errTableType {\n");
  fprintf(t_hdr,"  char *msg;\n  short val;\n");
  fprintf(t_hdr,"} w_errTable[] = {\n");
  for (i=0; i<t_idTableNext; i++)
    if (t_idTable[i].kind == pIdent &&
        t_idTable[i].idKind == kError) {
      fprintf(t_hdr,"   \"%s\", %d,\n",t_idTable[i].buffer,
                                       t_idTable[i].val);
      count++;
    }
  fprintf(t_hdr,"   \"\", 0\n};\n#define w_errTableSize %d\n",count);
  fprintf(t_hdr,"\n#endif SSL_INCLUDE_ERR_TABLE\n");

}


/* Optimize generated table */

w_optimize_table ()
{
    short  pc, target, final_target;
    short  options;
    short  improved_count;
    short  improved_case_jump_count;
    short  find_ultimate_destination();

    /*  keep track of when next choice table is coming up  */
    short  choice_table [dPatchCTAsize], choice_table_sp;

    printf ("\nOptimizing table...\n");

    choice_table_sp = 0;

    /* Convert a chain of jumps into a single jump */

    improved_count = 0;
    improved_case_jump_count = 0;

    pc = 0;
    while (pc < w_here)
    {
        if (choice_table_sp > 0  &&
            choice_table [choice_table_sp] == pc)
        {
            /* skip past choice table ... maybe optimize it too */

            options = w_outTable [pc++];
            /* pc += 2 * options; */

            while (options > 0)
            {
                pc++;
                target = pc - w_outTable[pc];
                final_target = find_ultimate_destination (target);
                if ((final_target != target) &&
                    (final_target < pc))
                {
                    w_outTable[pc] = pc - final_target;
                    improved_case_jump_count++;
                    improved_count++;
                }
                pc++;
                options--;
            }

            choice_table_sp--;
            continue;
        }

        switch (w_outTable[pc++])
        {
            case oJumpForward :
                    target = pc + w_outTable[pc];
                    final_target = find_ultimate_destination (target);
                    if (final_target != target)
                    {
                        improved_count++;
                        if (final_target > pc)
                        {
                            w_outTable [pc-1] = oJumpForward;
                            w_outTable [pc] = final_target - pc;
                        }
                        else
                        {
                            w_outTable [pc-1] = oJumpBack;
                            w_outTable [pc] = pc - final_target;
                        }
                    }
                    pc++;
                    continue;

            case oJumpBack :
                    target = pc - w_outTable[pc];
                    final_target = find_ultimate_destination (target);
                    if (final_target != target)
                    {
                        improved_count++;
                        if (final_target > pc)
                        {
                            w_outTable [pc-1] = oJumpForward;
                            w_outTable [pc] = final_target - pc;
                        }
                        else
                        {
                            w_outTable [pc-1] = oJumpBack;
                            w_outTable [pc] = pc - final_target;
                        }
                    }
                    pc++;
                    continue;

            case oInputChoice :
            case oChoice :
                    choice_table [++choice_table_sp] = pc + w_outTable[pc];
                    pc++;
                    continue;

            case oInput :
            case oEmit :
            case oError :
            case oCall :
            case oSetResult :
            case oSetParameter :
                    pc++;
                    continue;

            case oInputAny :
            case oReturn :
            case oEndChoice :
                    continue;

            default :
                    continue;
        }
    }
    printf ("Reduced %d jump instructions\n", improved_count);
    if (improved_case_jump_count)
        printf ("including %d jumps directly from a case.\n",
                improved_case_jump_count);
}

short find_ultimate_destination (pc)
short                      pc;
{
    while (1)
    {
        if (w_outTable[pc] == oJumpForward)
        {
            pc++;
            pc += w_outTable[pc];
        }
        else if (w_outTable[pc] == oJumpBack)
        {
            pc++;
            pc -= w_outTable[pc];
        }
        else
            break;
    }

    return (pc);
}
