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
*   ssl_begin.h         SSL interpreter function start
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
 *  immediately preceding a set of "case" statements
 *  which perform the application's semantic operations.
 *
 */

/*
 * --------------------------------------------------------------------------
 */

/*  Return status:
 *    0 = finished execution of program
 *    1 = completed single stepping
 *    2 = hit breakpoint
 */

int  ssl_walkTable()
{
   ssl_walking = 1;

   if (ssl_debug)
   {
       if (setjmp (ssl_fatal_jmp_buf))   /* Return here from calls to ssl_fatal() during run */
       { 
           ssl_walking = 0;
           ssl_error ("Stopped at fatal error");
           return (0);   /* I guess we say that the program is complete: hit fatal error */
       } 
   }


   while (1) {

     if (ssl_debug)
     {
        if (dbg_check_step_count ())
        {
            ssl_walking = 0;
            return (2);
        }
     }


     switch (ssl_code_table[ssl_pc++]) {

       /* SSL Instructions */

       case oJumpForward :
              ssl_pc += ssl_code_table[ssl_pc];
              continue;
       case oJumpBack :
              ssl_pc -= ssl_code_table[ssl_pc];
              continue;
       case oInput :
              ssl_get_token();
              if (ssl_token.code != ssl_code_table[ssl_pc])
              {
                  sprintf (ssl_error_buffer, "Syntax error: expecting \"%s\"",
                           ssl_get_code_name(ssl_code_table[ssl_pc]));
                  ssl_error (ssl_error_buffer);
                  ssl_pc--;
                  ssl_error_recovery ();
                  continue;
              }
              ssl_pc++;
              ssl_accept_token();
              continue;
       case oInputAny :
              ssl_get_token();
              ssl_accept_token();
              continue;

#if 0
       /* Application should define oEmit operation itself */
       case oEmit :
              dTemp = ssl_code_table[ssl_pc++];
              w_outTable[w_here++] = dTemp;
              continue;
#endif /* 0 */

       case oError :
              ssl_error_signal (ssl_code_table[ssl_pc]);
              ssl_pc--;
              ssl_error_recovery ();
              continue;
       case oInputChoice :
              ssl_get_token();
              ssl_tmp = ssl_pc - 1;              /* start of choice instruction */
              ssl_pc += ssl_code_table[ssl_pc];
              ssl_choice_options = ssl_code_table[ssl_pc++];
              while (ssl_choice_options--) {
                if (ssl_code_table[ssl_pc++] == ssl_token.code) {
                  ssl_pc -= ssl_code_table[ssl_pc];
                  ssl_accept_token();
                  break;
                }
                ssl_pc++;
              }
              continue;
       case oCall :
              if (++ssl_sp==ssl_stackSize) ssl_fatal ("SSL: call stack overflow");
              ssl_stack[ssl_sp] = ssl_pc+1;
              ssl_pc = ssl_code_table[ssl_pc];
              continue;
       case oReturn :
              if (ssl_sp) {
                ssl_pc = ssl_stack[ssl_sp--];
                continue;
              } else {
                ssl_pc--;
                ssl_walking = 0;
                return(0);            /* done walking table */
              }
       case oSetResult :
              ssl_result = ssl_code_table[ssl_pc++];
              continue;
       case oChoice :
              ssl_tmp = ssl_pc - 1;              /* start of choice instruction */
              ssl_pc += ssl_code_table[ssl_pc];
              ssl_choice_options = ssl_code_table[ssl_pc++];
              while (ssl_choice_options--) {
                if (ssl_code_table[ssl_pc++] == ssl_result) {
                  ssl_pc -= ssl_code_table[ssl_pc];
                  break;
                }
                ssl_pc++;
              }
              continue;
       case oEndChoice :           /* choice or inputChoice didn't match */
              ssl_error ("Syntax error");
              /* ssl_token.accepted = 1; */ /* actually, only if input choice err*/
              ssl_pc = ssl_tmp;        /* back to start of choice instruction */
              ssl_error_recovery ();
              continue;
       case oSetParameter :
              ssl_param = ssl_code_table[ssl_pc++];
              continue;
        case oBreak :
              ssl_walking = 0;
              return(1);   /* hit breakpoint */


       /* Semantic Mechanisms */


       /* [ Application code goes here ] */

/*
 * --------------------------------------------------------------------------
 */

