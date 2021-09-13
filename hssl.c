/*

  ssl.c  --  Amiga Syntax/Semantic Language Processor

  HISTORY
    27Aug89    First version, using scanner.c

  Notes:
    1)  This doesn't allow a type name to be referenced (in a
        semantic operation declaration) before it's defined.
    2)  In the listing file, the code addresses are not exact.
        The address that's printed should be 'codeTableNext' in effect
        when the first token is -accepted-, not just read.
        This is fixed in the ssl processor coded in ssl.
*/

#include <stdio.h>
#include <string.h>
#include <dos.h>

/* (uses dos.h for onbreak function) */

FILE *src,*out,*hdr,*doc,*lst;

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

#define lineSize 256
#define bufferSize 256
#define identSize 31
#define identTableSize 200
#define choicePatchSize 100
#define callPatchSize 200
#define breakPatchSize 10
#define codeTableSize 5000
#define choiceStackSize 200
#define safetyMargin 10
#define stringTableSize 40

short codeTable[codeTableSize+safetyMargin];
short codeTableNext;

short choiceStack[choiceStackSize+safetyMargin];
short choiceStackNext;

short choicePatch[choicePatchSize];
short choicePatchNext;

short breakPatch[breakPatchSize];
short breakPatchNext;

struct stringTableType {
  char buffer[identSize];     /* string shortform */
  short val;                  /* equivalent identifier */
} stringTable[stringTableSize];
short stringTableNext;

struct callPatchType {
  short addr;
  short rule;
} callPatch[callPatchSize];
short callPatchNext;

short currentRule;

char   lineBuffer[lineSize];
char   *tptr;
short  lineNumber;
short  errorCount;

enum kindType {kIdent, kIntLit, kStrLit, 
               kEquals, kColon, kSemiColon, kComma, kLParen, kRParen,
               kReturn, kBreak, kLCurly, kRCurly, kLSquare, kRSquare,
               kBar, kCall, kEmit, kStar, kErr, kQuestion,
               kInput, kOutput, kType, kError, kMechanism, kRules, kEnd,
               kEof, kInvalid};

struct tokenType {
  char buffer[bufferSize];     /* used to hold StrLits (and idents) */
  short buflen;
  short val;                   /* int lits are short */
  enum kindType kind;
  short lineNumber;
} token;

enum identKindType {ikUnknown, ikInput, ikOutput, ikError, 
                    ikType, ikVal, ikOp, ikRule};

struct identTableType {
  char buffer[identSize+1];
  short buflen;
  enum kindType kind;
  enum identKindType identKind;
  char declared;
  char choice;
  char param;
  short val;         /* code #, or rule address */
  short identType;
  short paramType;
  
} identTable[identTableSize];
short identTableNext;
short opValNext;

char upper[256],blank[256],identStart[256],identCont[256],digit[256];
enum kindType punct[256];

main(argc,argv)
int argc;
char *argv[];
{
  int hit_break();
  short tyPtr,i;

  if (argc<2) {
    printf("Usage:   scanner <file>\n");
    return(-1);
  }

  strcpy(token.buffer,argv[1]);
  strcpy(token.buffer+strlen(argv[1]),".ssl");
  if((src=fopen(token.buffer,"r"))==NULL) {
    printf("Can't open source file %s\n",token.buffer);
    return(-1);
  }
  if((out=fopen("ram:a.tbl","w"))==NULL) {
    printf("Can't open output file a.tbl\n");
    return(-1);
  }
  if((hdr=fopen("ram:a.h","w"))==NULL) {
    printf("Can't open output file a.h\n");
    return(-1);
  }
  if((doc=fopen("ram:a.doc","w"))==NULL) {
    printf("Can't open output file a.doc\n");
    return(-1);
  }
  if((lst=fopen("ram:a.lst","w"))==NULL) {
    printf("Can't open output file a.lst\n");
    return(-1);
  }

  onbreak(&hit_break);

  initScanner();

  stringTableNext = 0;
  errorCount = 0;

  get_token();
  while (token.kind != kEnd) {
    if (token.kind == kInput) {
      readIdList(ikInput);
    } else if (token.kind == kOutput) {
      readIdList(ikOutput);
    } else if (token.kind == kError) {
      readIdList(ikError);
    } else if (token.kind == kType) {
      get_token();
      if (token.kind != kIdent) {
        error_msg ("Expecting type name");
      } else {
        identTable[token.val].declared = 1;
        identTable[token.val].identKind = ikType;
        tyPtr = token.val;
        readIdList(ikVal);
        for (i=tyPtr+1;i<identTableNext;i++) {
          identTable[i].identType = tyPtr;
        }
      }
    } else if (token.kind == kEof) {
      error_msg ("Missing End statment");
      break;
    } else if (token.kind == kMechanism) {
      get_token();
      if (token.kind != kIdent)
        error_msg ("Expecting mechanism name");
      else {
        identTableNext--;   /* drop mech name from table */
        get_token();
      }
      if (token.kind != kColon)
        error_msg ("Missing colon");
      else
        get_token();
      while (token.kind == kIdent) {
        i = token.val;
        identTable[i].identKind = ikOp;
        identTable[i].val = opValNext++;
        identTable[i].declared = 1;
        get_token();
        if (token.kind == kLParen) {
          get_token();
          if (token.kind != kIdent ||
              !identTable[token.val].declared ||
              identTable[token.val].identKind != ikType)
            error_msg ("Expecting parameter type");
          else {
            identTable[i].paramType = token.val;
            identTable[i].param = 1;
            get_token();
          }
          if (token.kind != kRParen)
            error_msg ("Expecting ')'");
          else
            get_token();
        }
        if (token.kind == kReturn) {
          get_token();
          if (token.kind != kIdent ||
              !identTable[token.val].declared ||
              identTable[token.val].identKind != ikType)
            error_msg ("Expecting choice type");
          else {
            identTable[i].identType = token.val;
            identTable[i].choice = 1;
            get_token();
          }
        }
      }
      if (token.kind != kSemiColon)
        error_msg ("Expecting ';'");
      else
        get_token();
    } else if (token.kind == kRules) {
      parseRules();
    } else {
      error_msg ("Expecting a program block");
      get_token();
    }
  }

  fixCallPatches();

  if (errorCount)
    printf("\n%d error(s)\n",errorCount);
  printf("\nWriting Files\n");
  dumpTables();
  writeHeader();

  fclose(src);
  fclose(out);
  fclose(hdr);
  fclose(doc);
}

parseRules()
{
  codeTableNext = 0;
  callPatchNext = 0;
  choiceStackNext = 0;
  choicePatchNext = 0;
  breakPatchNext = 0;

  get_token();

  while (token.kind != kEnd) {
    currentRule = token.val;
    printf("Rule %s\n",token.buffer);
    if (token.kind != kIdent || identTable[currentRule].declared) {
      error_msg ("Expecting a rule name");
      get_token();
      continue;
    }
    identTable[currentRule].declared = 1;
    identTable[currentRule].identKind = ikRule;
    identTable[currentRule].val = codeTableNext;
    get_token();
    if (token.kind == kReturn) {
      get_token();
      if (token.kind != kIdent ||
          !identTable[token.val].declared ||
          identTable[token.val].identKind != ikType)
        error_msg ("Expecting choice type");
      else {
        identTable[currentRule].identType = token.val;
        identTable[currentRule].choice = 1;
        get_token();
      }
    }
    if (token.kind != kColon)
      error_msg ("Expecting ':'");
    else
      get_token();
    while (token.kind != kSemiColon)
      parseStatement();
    get_token();
    codeTable[codeTableNext++] = oReturn;
  }
}

parseStatement()
{
  short i,lab,choiceType,choiceStackStart,choices,choicePatchStart;
  short breakPatchStart;

  if (token.kind == kStrLit)
    lookupString();
  if (token.kind == kIdent) {
    i = token.val;
    if (identTable[i].identKind == ikInput) {
      codeTable[codeTableNext++] = oInput;
      codeTable[codeTableNext++] = identTable[i].val;
      get_token();
    } else if (identTable[i].identKind == ikOp) {
      if (identTable[i].param) {
        get_token();
        if (token.kind != kLParen)
          error_msg ("Parameter required");
        else {
          get_token();
          if (token.kind != kIdent ||
              identTable[token.val].identKind != ikVal ||
              identTable[token.val].identType !=
                        identTable[i].paramType)
            error_msg ("Parameter is wrong type");
          else {
            codeTable[codeTableNext++] = oSetParameter;
            codeTable[codeTableNext++] = identTable[token.val].val;
          }
          get_token();
          if (token.kind != kRParen)
            error_msg ("Missing ')'");
          else
            get_token();
        }
      } else
        get_token();
      codeTable[codeTableNext++] = identTable[i].val;
      if (identTable[i].choice)
        error_msg ("Not a choice construct");
    } else if (identTable[i].identKind == ikUnknown) {
      error_msg ("Undeclared identifier");
      get_token();
    } else {
      error_msg ("Identifier out of place");
      get_token();
    }
  } else if (token.kind == kErr) {
    get_token();
    if (token.kind != kIdent ||
        identTable[token.val].identKind != ikError)
      error_msg ("Expecting an error signal");
    else {
      codeTable[codeTableNext++] = oError;
      codeTable[codeTableNext++] = identTable[token.val].val;
      get_token();
    }
  } else if (token.kind == kEmit) {
    get_token();
    if (token.kind == kStrLit)
      lookupString();
    if (token.kind != kIdent || 
        identTable[token.val].identKind != ikOutput)
      error_msg ("Expecting an output token");
    else {
      codeTable[codeTableNext++] = oEmit;
      codeTable[codeTableNext++] = identTable[token.val].val;
      get_token();
    }
  } else if (token.kind == kCall) {
    get_token();
    if (token.kind != kIdent)
      token.val = 0;  /* err */
    i = token.val;
    if (identTable[i].declared) {
      if (identTable[i].identKind != ikRule)
        error_msg ("Expecting a rule");
      else if (identTable[i].choice)
        error_msg ("Not a choice construct");
      else {
        codeTable[codeTableNext++] = oCall;
        codeTable[codeTableNext++] = identTable[i].val;
        get_token();
      }
    } else {
      identTable[i].identKind = ikRule;   /* not yet declared */
      if (callPatchNext >= callPatchSize)
        fatal ("Call Patch Table Overflow");
      codeTable[codeTableNext++] = oCall;
      callPatch[callPatchNext].addr = codeTableNext++;
      callPatch[callPatchNext++].rule = i;
      get_token();
    }
  } else if (token.kind == kReturn) {
    get_token();
    if (identTable[currentRule].choice) {
      if (token.kind != kIdent ||
          identTable[token.val].identKind != ikVal ||
          identTable[token.val].identType !=
                            identTable[currentRule].identType)
        error_msg ("Return value is wrong type");
      else {
        codeTable[codeTableNext++] = oSetResult;
        codeTable[codeTableNext++] = identTable[token.val].val;
        get_token();
      }
    }
    codeTable[codeTableNext++] = oReturn;
  } else if (token.kind == kLCurly) {
    lab = codeTableNext;
    breakPatchStart = breakPatchNext;
    get_token();
    while (token.kind != kRCurly &&
           token.kind != kSemiColon)
      parseStatement();
    if (token.kind == kRCurly) {
      codeTable[codeTableNext++] = oJumpBack;
      codeTable[codeTableNext] = codeTableNext - lab;
      codeTableNext++;
      get_token();
    }
    for (i=breakPatchStart; i<breakPatchNext; i++)
      codeTable[breakPatch[i]] = codeTableNext - breakPatch[i];
    breakPatchNext = breakPatchStart;
  } else if (token.kind == kLSquare) {
    choiceType = -1;     /* input choice by default */
    get_token();
    if (token.kind == kIdent) {
      i = token.val;
      if (!identTable[i].declared) {        /* rule choice */
        identTable[i].identKind = ikRule;
        if (callPatchNext >= callPatchSize)
          fatal ("Call Patch Table Overflow");
        codeTable[codeTableNext++] = oCall;
        callPatch[callPatchNext].addr = codeTableNext++;
        callPatch[callPatchNext++].rule = i;
        choiceType = -2;  /* use type of first case */
        get_token();
      } else if (identTable[i].identKind == ikRule) {
        if (!identTable[i].choice)
          error_msg ("Must be a choice rule");
        else {
          codeTable[codeTableNext++] = oCall;
          codeTable[codeTableNext++] = identTable[i].val;
          choiceType = identTable[i].identType;
        }
        get_token();
      } else if (identTable[i].identKind != ikOp) {
        error_msg ("Expecting a choice rule or operation");
        get_token();
      } else {                /* semantic choice operation */
        if (identTable[i].param) {
          get_token();
          if (token.kind != kLParen)
            error_msg ("Parameter required");
          else {
            get_token();
            if (token.kind != kIdent ||
                identTable[token.val].identKind != ikVal ||
                identTable[token.val].identType !=
                            identTable[i].paramType)
              error_msg ("Parameter is wrong type");
            else {
              codeTable[codeTableNext++] = oSetParameter;
              codeTable[codeTableNext++] = identTable[token.val].val;
            }
            get_token();
            if (token.kind != kRParen)
              error_msg ("Missing ')'");
            else
              get_token();
          }
        } else
          get_token();
        codeTable[codeTableNext++] = identTable[i].val;
        if (!identTable[i].choice)
          error_msg ("Expecting a choice operation");
        else
          choiceType = identTable[i].identType;
      }
    }
    if (choiceType == -1)
      codeTable[codeTableNext++] = oInputChoice;
    else
      codeTable[codeTableNext++] = oChoice;
    lab = codeTableNext++;    /* plug relative address of table in here */
    choices = 0;
    choiceStackStart = choiceStackNext;
    choicePatchStart = choicePatchNext;
    while (token.kind == kBar) {
      get_token();
      if (token.kind == kIdent || token.kind == kStrLit) {
        while (1) {
          if (token.kind == kStrLit)
            lookupString();
          if (choiceType == -2) {   /* undeclared rule choice */
            if (identTable[token.val].identKind != ikVal) {
              error_msg ("Expecting a choice value");
              while (token.kind != kBar && token.kind != kRSquare)
                get_token();
              continue;
            }
            choiceType = identTable[token.val].identType;
          } else if (choiceType == -1) {     /* input choice */
            if (identTable[token.val].identKind != ikInput) {
              error_msg ("Expecting an input value");
              get_token();
              while (token.kind != kBar && token.kind != kRSquare)
                get_token();
              continue;
            }
          } else if (identTable[token.val].identKind != ikVal ||
                     identTable[token.val].identType != choiceType) {
            error_msg ("Choice value is wrong type");
            get_token();
            while (token.kind != kBar && token.kind != kRSquare)
              get_token();
            continue;
          }
          choices++;
          choiceStack[choiceStackNext++] = codeTableNext;
          choiceStack[choiceStackNext++] = identTable[token.val].val;
          if (choiceStackNext > choiceStackSize)
            fatal ("Choice stack overflow");
          get_token();
          if (token.kind != kComma)
            break;
          get_token();
          if (token.kind != kIdent && token.kind != kStrLit) {
            error_msg ("Expecting a choice value");
            break;
          }
        } /* while */
        if (token.kind != kColon)
          error_msg ("Expecting ':'");
        else
          get_token();
        while (token.kind != kBar && token.kind != kRSquare)
          parseStatement();
        /* jump to end of choices */
        codeTable[codeTableNext++] = oJumpForward;
        choicePatch[choicePatchNext++] = codeTableNext++;
        if (choicePatchNext > choicePatchSize)
          fatal ("Choice Patch Table Overflow");
        if (token.kind == kRSquare) {
          /* end of choices, with no default code. copy choice table */
          /* note, the addresses in the table are relative */
          codeTable[lab] = codeTableNext - lab;      /* rel loc of table */
          codeTable[codeTableNext++] = choices;
          i = choiceStackNext-1;
          while (i > choiceStackStart) {
            codeTable[codeTableNext++] = choiceStack[i--];
            codeTable[codeTableNext] = codeTableNext - choiceStack[i--];
            codeTableNext++;
          }
          choiceStackNext = choiceStackStart;
          codeTable[codeTableNext++] = oEndChoice;
        }
      } else if (token.kind == kStar) {
        /* default code here. copy choice table first */
        codeTable[lab] = codeTableNext - lab;  /* rel loc of table */
        codeTable[codeTableNext++] = choices;
        i = choiceStackNext-1;
        while (i > choiceStackStart) {
          codeTable[codeTableNext++] = choiceStack[i--];
          codeTable[codeTableNext] = codeTableNext - choiceStack[i--];
          codeTableNext++;
        }
        choiceStackNext = choiceStackStart;
        get_token();
        if (token.kind != kColon)
          error_msg ("Expecting ':'");
        else
          get_token();
        while (token.kind != kBar && token.kind != kRSquare)
          parseStatement();
        if (token.kind == kBar)
          error_msg ("Default choice must be given last");
      } else
        error_msg ("Expecting a choice value");
    } /* while bar */
    if (token.kind != kRSquare)
      error_msg ("Expecting ']'");
    else
      get_token();
    /* patch jumps to end of choice table, from each choice */
    i = choicePatchStart;
    while (i < choicePatchNext) {
      codeTable[choicePatch[i]] = codeTableNext-choicePatch[i];
      i++;
    }
    choicePatchNext = choicePatchStart;
  } else if (token.kind == kBreak) {
    get_token();
    codeTable[codeTableNext++] = oJumpForward;
    breakPatch[breakPatchNext++] = codeTableNext++;
  } else if (token.kind == kQuestion) {
    get_token();
    codeTable[codeTableNext++] = oInputAny;
  } else {
    error_msg ("Expecting a statment");
    get_token();
  }

  if (codeTableNext >= codeTableSize)
    fatal ("Code Table Overflow");
}


readIdList(identKind)
enum identKindType identKind;
{
  short code,i;

  get_token();
  if (token.kind != kColon)
    error_msg ("Missing colon");
  else
    get_token();
  code = 0;
  while (token.kind == kIdent) {
    i = token.val;
    identTable[i].identKind = identKind;
    identTable[i].declared = 1;
    get_token();
    if (token.kind == kStrLit) {
      if (identKind != ikInput && identKind != ikOutput)
        error_msg ("String shortform only valid for input or output");
      else if (token.buflen > identSize)
        error_msg ("String shortform too long");
      else {
        strcpy(stringTable[stringTableNext].buffer,token.buffer);
        stringTable[stringTableNext++].val = i;
        if (stringTableNext > stringTableSize)
          fatal ("String shortform table overflow");
      }
      get_token();
    }
    if (token.kind == kEquals) {
      get_token();
      if (token.kind != kIntLit)
        error_msg ("Expecting integer");
      else {
        code = token.val;
        get_token();
      }
    }
    identTable[i].val = code++;
  }
  if (token.kind != kSemiColon)
    error_msg ("Expecting ';'");
  else
    get_token();
}

lookupString()    /* shortforms for input, output names */
{
  short i;
  for (i=0; i<stringTableNext; i++)
    if (!strcmp(token.buffer,stringTable[i].buffer)) {
      token.kind = kIdent;
      token.val = stringTable[i].val;
      return;
    }
  error_msg ("Using an undefined string shortform");
  token.kind = kInvalid;
  token.val = 0;
}

initScanner ()
{
  register short i;
  short firstOp,op;

  /* C guarantees static vars to start with 0, so here I don't
       bother to initialize all of blank,identStart,identCont,digit */

  for (i=0;i<256;i++)
    upper[i] = i;
  for (i='a';i<='z';i++) {
    upper[i] = i - ('a'-'A');
    identStart[i] = 1;
    identCont[i] = 1;
  }
  for (i='A';i<='Z';i++) {
    identStart[i] = 1;
    identCont[i] = 1;
  }
  identStart['_'] = 1;
  identCont['_'] = 1;
  for (i='0';i<='9';i++) {
    digit[i] = 1;
    identCont[i] = 1;
  }
  blank[' '] = 1;
  blank['\t'] = 1;
  blank['\n'] = 1;

  /* single-character tokens */
  for (i=0;i<256;i++)
    punct[i] = kInvalid;
  punct['{'] = kLCurly;
  punct['}'] = kRCurly;
  punct['['] = kLSquare;
  punct[']'] = kRSquare;
  punct['|'] = kBar;
  punct['@'] = kCall;
  punct['*'] = kStar;
  punct['?'] = kQuestion;
  punct['.'] = kEmit;
  punct['='] = kEquals;
  punct[':'] = kColon;
  punct[';'] = kSemiColon;
  punct[','] = kComma;
  punct['('] = kLParen;
  punct[')'] = kRParen;
  punct['#'] = kErr;

  lineBuffer[0] = '\0';
  tptr = lineBuffer;
  lineNumber = 0;
  identTableNext = 0;
  token.kind = kColon;   /* dummy */
  addIdent("input",kInput);
  addIdent("output",kOutput);
  addIdent("type",kType);
  addIdent("error",kError);
  addIdent("mechanism",kMechanism);
  addIdent("rules",kRules);
  addIdent("end",kEnd);
  /* add basic operations */
  firstOp = identTableNext;
  addIdent("oJumpForward",kIdent);
  addIdent("oJumpBack",kIdent);
  addIdent("oInput",kIdent);
  addIdent("oInputAny",kIdent);
  addIdent("oEmit",kIdent);
  addIdent("oError",kIdent);
  addIdent("oInputChoice",kIdent);
  addIdent("oCall",kIdent);
  addIdent("oReturn",kIdent);
  addIdent("oSetResult",kIdent);
  addIdent("oChoice",kIdent);
  addIdent("oEndChoice",kIdent);
  addIdent("oSetParameter",kIdent);
  opValNext = 0;
  for (op=firstOp;op<identTableNext;op++) {
    identTable[op].val = opValNext++;
    identTable[op].identKind = ikOp;
    identTable[op].declared = 1;        /* other fields false */
  }
}  

get_token()
{
  char more,*p,readln();
  short lit;

  /* error if we try to read past the end of file */
  if (token.kind == kEof)
    fatal ("Unexpected end of file"); 

 /* skip to start of token */

  while (1) {
    while (blank[*tptr])
      tptr++;

    if (*tptr=='\0') {
      more = readln(lineBuffer);
      if (!more) {
        token.kind = kEof;
        return;
      }
      tptr = lineBuffer;
      lineNumber++;
      fprintf(lst,"%4d: %s",codeTableNext,lineBuffer);
    } else if (*tptr=='%') {
      more = readln(lineBuffer);
      if (!more) {
        token.kind = kEof;
        return;
      }
      tptr = lineBuffer;
      lineNumber++;
      fprintf(lst,"%4d: %s",codeTableNext,lineBuffer);
    } else {
      break;
    }
  }

  /* copy token */

  token.lineNumber = lineNumber;
  p = token.buffer;
  if (identStart[*tptr]) {
    while (identCont[*tptr])
      *p++ = *tptr++;   /* upper[*tptr++]; */
    *p = '\0';
    token.buflen = p-token.buffer;
    token.kind = kIdent;
    lookupIdent();
  } else if (digit[*tptr]) {
    lit = 0;
    while (digit[*tptr]) {
      lit = lit*10 - '0' + *tptr;
      *p++ = *tptr++;
    }
    *p = '\0';
    token.val = lit;
    token.kind = kIntLit;
  } else if (*tptr=='\'') {
    tptr++;
    token.kind = kStrLit;
    while (p-token.buffer+1 < bufferSize) {
      if (*tptr=='\'') {
        if (*++tptr=='\'') {
          *p++ = '\'';
          tptr++;
        } else {
          break;
        }
      } else if (*tptr=='\0') {
        break;
      } else {
        *p++ = *tptr++;
      }
    }
    *p = '\0';
    token.buflen = p-token.buffer;
  } else if (*tptr=='>') {
    tptr++;
    if (*tptr=='>') {
      tptr++;
      token.kind = kReturn;
    } else {
      token.kind = kBreak;
    }
  } else {
    token.kind = punct[*tptr++];
    if (token.kind == kInvalid) {
      --tptr;
      printf("Invalid token on line %d: %c\n",token.lineNumber,*tptr++);
    }
  }
}

addIdent(buffer,kind)
char *buffer;
short kind;
{
  identTable[identTableNext].buflen = strlen(buffer);
  strcpy(identTable[identTableNext].buffer,buffer);
  identTable[identTableNext].kind = kind;
  identTable[identTableNext].identKind = ikUnknown; /* for user idents */
  if (++identTableNext >= identTableSize)
    fatal("identifier table overflow");
}

lookupIdent()
{
  short i;
  register char *a,*b;

  for (i=0;i<identTableNext;i++) {
    if (token.buflen==identTable[i].buflen) {
      a = token.buffer;
      b = identTable[i].buffer;
      while (upper[*a++]==upper[*b++]) {
        if (!*a) {   /* match */
          token.kind = identTable[i].kind;   /* for built-in idents */
          token.val = i;    /* for user-defined idents */
          return;
        }
      }
    }
  }
  token.val = identTableNext;      /* add to end */
  addIdent(token.buffer,kIdent);
}

/* returns 0 if EOF, else 1.  lineBuffer gets the '\0'-terminated string
   (usually with \n, except on last line). */

char readln(lineBuffer)
char *lineBuffer;
{
  char *ptr;
  ptr = fgets(lineBuffer,lineSize,src);
  if (ptr==NULL)
    return(0);
  return(1);
}

hit_break()
{
  printf("\nBreaking...\n");
  fclose(src);
  fclose(out);
  fclose(hdr);
  fclose(doc);
  return(1);
}

fatal(msg)
char *msg;
{
  printf("[FATAL] ");
  fprintf(doc,"[FATAL] ");
  error_msg (msg);
  dumpTables();
  writeHeader();
  fclose(src);
  fclose(out);
  fclose(hdr);
  fclose(doc);
  exit(-3);
}

error_msg(msg)
char *msg;
{
  errorCount++;
  printf("%s on line %d (token=",msg,token.lineNumber);
  fprintf(doc,"%s on line %d (token=",msg,token.lineNumber);
  if (token.kind==kIdent) {
    printf("%s)\n",token.buffer);
    fprintf(doc,"%s)\n",token.buffer);
  } else if (token.kind==kStrLit) {
    printf("'%s')\n",token.buffer);
    fprintf(doc,"'%s')\n",token.buffer);
  } else {
    printf("<%d>)\n",(int)token.kind);
    fprintf(doc,"<%d>)\n",(int)token.kind);
  }
}

dumpTables()
{
  short i;
  fprintf(doc,"Ident table:\n\n");
  for (i=0; i<identTableNext; i++) {
    fprintf(doc,"%d: %s\n",i,identTable[i].buffer);
    fprintf(doc,"    iKind=%d val=%d dcl=%d >>=%d typ=%d ()=%d pTyp=%d\n",
            (int) identTable[i].identKind,
            (int) identTable[i].val,
            (int) identTable[i].declared,
            (int) identTable[i].choice,
            (int) identTable[i].identType,
            (int) identTable[i].param,
            (int) identTable[i].paramType);
  }
  fprintf(doc,"\nString shortform table:\n\n");
  for (i=0; i<stringTableNext; i++)
    fprintf(doc,"%s  '%s'\n",identTable[stringTable[i].val].buffer,
                             stringTable[i].buffer);
  fprintf(out,"%d\n\n",codeTableNext);
  for (i=0; i<codeTableNext; i++)
    fprintf(out,"%d\n",codeTable[i]);
}

fixCallPatches()
{
  short p,i;

  /* Apparently, absolute addresses are used in oCall instructions */

  for (p=0; p<callPatchNext; p++) {
    i = callPatch[p].rule;
    if (!identTable[i].declared) {
      strcpy(lineBuffer,"No definition was found for rule ");
      strcat(lineBuffer,identTable[i].buffer);
      fatal (lineBuffer);
    }
    codeTable[callPatch[p].addr] = identTable[i].val;
  }
}

writeHeader()
{
  short i,count;
  fprintf(hdr,"#define w_codeTableSize %d\n\n",codeTableNext+50);

  for (i=0; i<identTableNext; i++)
    if (identTable[i].kind == kIdent &&
        identTable[i].identKind != ikType &&
        identTable[i].identKind != ikRule)
      fprintf(hdr,"#define %s %d\n",
              identTable[i].buffer,
              identTable[i].val);
  count = 0;
  fprintf(hdr,"\nstruct w_errTableType {\n");
  fprintf(hdr,"  char *msg;\n  short val;\n");
  fprintf(hdr,"} w_errTable[] = {\n");
  for (i=0; i<identTableNext; i++)
    if (identTable[i].kind == kIdent &&
        identTable[i].idKind == ikErr) {
      fprintf(hdr,"   \"%s\", %d,\n",identTable[i].buffer,
                                     identTable[i].val);
      count++;
    }
  fprintf(hdr,"   \"\", 0\n};\n#define w_errTableSize %d\n",count);

}
