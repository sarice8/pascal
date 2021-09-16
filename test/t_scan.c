/**/ 
static char sccsid[] = "@(#)schema_scan.c	1.2 6/11/93 16:15:16 /files/home/sim/sarice/compilers/schema/SCCS/s.schema_scan.c";
/**/

/*  Testing the SSL runtime module and scanner module
 *
 *  This test is derived from the Schema compiler
 *
 *  t_scan.c
 *
 */


#include <stdio.h>
#include <string.h>

#ifdef AMIGA
#include <dos.h>
#endif AMIGA

#include "ssl_rt.h"
#include "t.h"


struct ssl_token_table_struct my_keyword_table[] =
{

    "Boolean1",      pBoolean1,
    "Character1",    pCharacter1,
    "Integer4",      pInteger4,
    "Integer8",      pInteger8,
    "IntegerN",      pIntegerN,
    "Real4",         pReal4,
    "Real8",         pReal8,
    "RealN",         pRealN,
    "StringN",       pStringN,
    "ObjectType",    pObjectType,
    "Node",          pNode,
    "List",          pList,

    "Pri",           pPri,
    "Alt",           pAlt,
    "Opt",           pOpt,
    "Int",           pInt,
    "Ext",           pExt,
    "Sys",           pSys,
    "Tmp",           pTmp,

    "Schema",        pSchema,
    "Is",            pIs,
    "Root",          pRoot,
    "End",           pEnd,

    NULL,            0
};

struct ssl_token_table_struct my_operator_table[] =
{
    ":",             pCOLON,
    ",",             pCOMMA,
    "(",             pLPAREN,
    ")",             pRPAREN,
    "[",             pLSQUARE,
    "]",             pRSQUARE,
    "::=",           pDERIVES,
    "=>",            pCONTAINS,

    NULL,            0
};

struct ssl_special_codes_struct my_special_codes;


init_my_scanner ()
{
    my_special_codes.invalid = pINVALID;
    my_special_codes.eof     = pEOF;
    my_special_codes.ident   = pIDENTIFIER;

    my_special_codes.intlit  = pINVALID;   /* Don't want integers for now */
    my_special_codes.strlit  = pINVALID;   /* Don't want strings for now */

    ssl_init_scanner (my_keyword_table, my_operator_table, &my_special_codes);
}


