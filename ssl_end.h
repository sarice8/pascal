/*
static char sccsid[] = "%W% %G% %U% %P%";
*/

/*
*****************************************************************************
*
*   Syntax/Semantic Language Runtime Support
*
*   by Steve Rice
*
*   Aug 18, 1993
*
*****************************************************************************
*
*   ssl_end.h           SSL interpreter function end   
*
*   HISTORY
*----------------------------------------------------------------------------
*   08/18/93 | Steve  | Based on SSL interpreter in my Schema compiler
*            |        |  
*
*****************************************************************************
*/

 
/*
 *  This file should be included in the application program
 *  immediately following a set of "case" statements
 *  which perform the application's semantic operations.
 *
 */

/*
 * --------------------------------------------------------------------------
 */

       /* [ Application code goes here ] */


       default:
              ssl_pc--;
              sprintf(ssl_error_buffer,"SSL: Semantic operation %d (at %d) has not been implemented",
                      ssl_code_table[ssl_pc],ssl_pc);
              ssl_fatal (ssl_error_buffer);

     } /* switch */

   }  /* while */
}



/*
 *  For now, error recovery function is also included in this header.
 *
 *  ssl_recovery_token is the token code to synchronize with.
 *  For example, typically it will be something like pSemicolon.
 *  In the Schema compiler it is pDERIVES.
 *
 *  ssl_eof_token is the token code for end of file (pEof).
 *  It's checked here in order to avoid ugly "unexpected eof"
 *  during recovery.  Alternatively, the scanner could be told
 *  not to care during recovery, or we could have an eof flag
 *  from the scanner directly.
 *
 *  NOTE: In schema compiler, recovery was cancelled and we abort
 *  instead, because I claimed that I can only synchronize to a
 *  "required" token, not a choice token (which pDERIVES is).
 *  Can that be improved?
 *
 */

/*  On entry, ssl_pc will sit at start of oInput, oError, oInputChoice or
    oChoice instruction, which caused the mismatch.
    ssl_token will contain the mismatched token (not yet accepted).
    On exit, ssl_pc will sit at an instruction just after accepting a ';', and
    ssl_token will contain a just-accepted ';' (or pEOF).
    No semantic mechanisms will be called; no output will be generated.
    The ssl_last_id index will not be maintained.
    Note, currently Semantic/Rule choices will follow the same algorithm as
    input choices: take default, or first choice if no default.    */

void
ssl_error_recovery ()
{
    int done_loop = 0;
    int steps;


steps = 0;  /* for debugging */

    /* First, walk table until we accept a ';'.  Do not call semantic mechanisms.
       Do not actually accept input tokens. */


    /* If original failure in a choice table, start recovery by taking the first
       choice.  (By definition, a failure in a choice will only happen if there
       is no default) */
    /* NOTE: here I assume that the first choice given by the user will be the
       code immediately following the oChoice instruction.  Alternatively, could
       follow the table, where the user's first choice is currently the LAST entry. */
/* NOTE: should check if the first choice in the input choice table is for ; */


    if (ssl_debug)
        printf ("Recovery: error with %s at location %d\n", ssl_get_code_name(ssl_token.code), ssl_pc);

    if (ssl_code_table[ssl_pc] == oInputChoice || ssl_code_table[ssl_pc] == oChoice)
    {
        ssl_pc += 2;
    }

#ifdef TRACE_RECOVERY
printf ("Tracing recovery: ");
#endif /* TRACE_RECOVERY */

   while (!done_loop)
   {

#ifdef TRACE_RECOVERY
printf ("%d ", ssl_pc);
#endif /* TRACE_RECOVERY */

     if (steps++ > 10000)
     {
         ssl_fatal ("Error recovery is apparently stuck in a loop");
     }

     switch (ssl_code_table[ssl_pc++]) {

       case oJumpForward :
              ssl_pc += ssl_code_table[ssl_pc];
              continue;
       case oJumpBack :
              ssl_pc -= ssl_code_table[ssl_pc];
              continue;
       case oInput :
              if (ssl_code_table[ssl_pc++] == ssl_recovery_token)
                  done_loop = 1;
              continue;
       case oInputAny :
              continue;
       case oEmit :
              ssl_pc++;
              continue;
       case oError :
              ssl_pc++;
              continue;
       case oInputChoice :
              ssl_tmp = ssl_pc + ssl_code_table[ssl_pc];                /* position of #options */
              ssl_tmp = (ssl_code_table[ssl_tmp] * 2) + ssl_tmp + 1;    /* position of default code */
              if (ssl_code_table[ssl_tmp] == oEndChoice)            /* no default */
              {
                  ssl_pc = ssl_pc + 1;                             /* go to first choice */
                  if (ssl_code_table[ssl_tmp-1] == ssl_recovery_token)  /* ASSUMING first choice is */
                      done_loop = 1;                           /*     last in table        */
              }
              else
                  ssl_pc = ssl_tmp;                                /* go to default code */
              continue;
       case oCall :
              if (++ssl_sp==ssl_stackSize) ssl_fatal ("SSL: call stack overflow");
              ssl_stack[ssl_sp] = ssl_pc+1;
              ssl_pc = ssl_code_table[ssl_pc];
              if (++ssl_var_sp==ssl_var_stackSize) ssl_fatal ("SSL: var stack overflow");
              ssl_var_stack[ssl_var_sp] = ssl_var_fp;
              ssl_var_fp = ssl_var_sp;
              continue;
       case oReturn :
              if (ssl_sp) {
                ssl_pc = ssl_stack[ssl_sp--];
                ssl_var_sp = ssl_var_fp-1;
                ssl_var_fp = ssl_var_stack[ssl_var_fp];
                continue;
              } else {
                ssl_pc--;   /* Leave recovery mode before final Return of program */
                ssl_var_sp = 0;
                ssl_var_fp = 0;
                ssl_walking = 0;
                done_loop = 1;
              }
              continue;
       case oSetResult :
              ssl_result = ssl_code_table[ssl_pc++];
              continue;
       case oChoice :
              /* NOTE: maybe we should obey actual choice value here, but
                       can't now because not calling semantic mechanisms. */
              ssl_tmp = ssl_pc + ssl_code_table[ssl_pc];                /* position of #options */
              ssl_tmp = (ssl_code_table[ssl_tmp] * 2) + ssl_tmp + 1;    /* position of default code */
              if (ssl_code_table[ssl_tmp] == oEndChoice)            /* no default */
                  ssl_pc = ssl_pc + 1;                             /* go to first choice */
              else
                  ssl_pc = ssl_tmp;                                /* go to default code */
              continue;
       case oEndChoice :           /* choice or inputChoice didn't match */
              continue;            /* shouldn't occur during recovery */
       case oPushResult :
              if (++ssl_var_sp==ssl_var_stackSize) ssl_fatal ("SSL: var stack overflow");
              ssl_var_stack[ssl_var_sp] = ssl_result;
              continue;
       case oPop :
              ssl_var_sp -= ssl_code_table[ssl_pc++];
              continue;
       case oGlobalSpace :
#ifdef SSL_DEBUG
              /* Clear vars on every entry, to aid in debugging.
               * (schema nodes will thus be Null or valid nodes).
               */
              bzero (&(ssl_var_stack[ssl_var_sp+1]), ssl_code_table[ssl_pc]*sizeof(ssl_var_stack[0]));
#endif /* SSL_DEBUG */
              ssl_var_sp += ssl_code_table[ssl_pc++];
              ssl_var_fp = ssl_var_sp-1;
              continue; 
       case oLocalSpace :
#ifdef SSL_DEBUG
              /* Clear vars on every entry, to aid in debugging.
               * (schema nodes will thus be Null or valid nodes).
               */
              bzero (&(ssl_var_stack[ssl_var_sp+1]), ssl_code_table[ssl_pc]*sizeof(ssl_var_stack[0]));
#endif /* SSL_DEBUG */
              ssl_var_sp += ssl_code_table[ssl_pc++];
              continue; 
       case oGetParam :
              ssl_result = ssl_var_stack[ssl_var_fp - ssl_code_table[ssl_pc++]];
              continue;
       case oGetFromParam :
              ssl_result = ssl_var_stack[ssl_var_stack[ssl_var_fp - ssl_code_table[ssl_pc++]]];
              continue;
       case oGetLocal :
              ssl_result = ssl_var_stack[ssl_var_fp + ssl_code_table[ssl_pc++]];
              continue;
       case oGetGlobal :
              ssl_result = ssl_var_stack[ssl_code_table[ssl_pc++]];
              continue;
       case oGetAddrParam :
              ssl_result = ssl_var_fp - ssl_code_table[ssl_pc++];
              continue;   
       case oGetAddrLocal :
              ssl_result = ssl_var_fp + ssl_code_table[ssl_pc++];
              continue;   
       case oGetAddrGlobal :
              ssl_result = ssl_code_table[ssl_pc++];
              continue;
       case oAssign :
              ssl_var_stack[ssl_var_stack[ssl_var_sp--]] = ssl_result;
              continue;

       default :                   /* all semantic mechanisms ignored */
              continue;
       }

    }

#ifdef TRACE_RECOVERY
printf ("\n");
#endif /* TRACE_RECOVERY */

    if (ssl_debug)
        printf ("Recovery: ssl_pc advanced to %d\n", ssl_pc);

    /* Now advance token stream to accept a ';' */

    /* first check if mismatch token is ';' */

    if (ssl_token.code == ssl_recovery_token /* || ssl_token.code == ssl_eof_token */)
    {
        ssl_accept_token ();
        return;
    }

    do
    {
        ssl_get_token();
/* printf ("recovery: accepting %s\n", ssl_get_code_name(ssl_token.code)); */
        ssl_accept_token ();
#ifdef AMIGA
printf ("+");  /* FOR DEBUGGING */
#endif /* AMIGA */
    }
    while (ssl_token.code != ssl_recovery_token /* && ssl_token.code != ssl_eof_token */);

#ifdef AMIGA
printf ("\n");  /* For debugging, after '+' */
#endif /* AMIGA */
}


