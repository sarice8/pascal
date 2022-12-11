/*

  tcode.cc

  Utilities for tcode, the intermediate output of the Pascal compiler.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


#include <unordered_map>
#include <string>
#include <vector>


#include "pascal.h"


struct instrInfo_s {
  const char* name;
  int args;
};

struct instrInfo_s tCodeInstrs[] = {
  { "tPushGlobalI", 1 },
  { "tPushGlobalB", 1 },
  { "tPushGlobalP", 1 },
  { "tPushLocalI", 1 },
  { "tPushLocalB", 1 }, 
  { "tPushLocalP", 1 },
  { "tPushParamI", 1 },
  { "tPushParamB", 1 }, 
  { "tPushParamP", 1 },
  { "tPushUpLocalI", 2 },
  { "tPushUpLocalB", 2 }, 
  { "tPushUpLocalP", 2 },
  { "tPushUpParamI", 2 },
  { "tPushUpParamB", 2 }, 
  { "tPushUpParamP", 2 },
  { "tPushConstI", 1 },
  { "tPushConstD", 2 },  // value: low 32 bits, high 32 bits
  { "tPushAddrGlobal", 1 },
  { "tPushAddrLocal", 1 },
  { "tPushAddrParam", 1 },
  { "tPushAddrActual", 1 },
  { "tPushAddrUpLocal", 2 },
  { "tPushAddrUpParam", 2 },
  { "tSwap", 0 },
  { "tFetchI", 0 },
  { "tFetchB", 0 },
  { "tFetchP", 0 },
  { "tAssignI", 0 },
  { "tAssignB", 0 },
  { "tAssignP", 0 },
  { "tCopy", 1 },
  { "tCastBtoI", 0 },
  { "tCastItoB", 0 },
  { "tIncI", 0 },
  { "tDecI", 0 },
  { "tMultI", 0 },
  { "tDivI", 0 },
  { "tAddPI", 0 },
  { "tAddI", 0 },
  { "tSubP", 0 },
  { "tSubPI", 0 },
  { "tSubI", 0 },
  { "tNegI", 0 },
  { "tNot", 0 },
  { "tEqualB", 0 },
  { "tNotEqualB", 0 },
  { "tGreaterB", 0 },
  { "tLessB", 0 },
  { "tGreaterEqualB", 0 },
  { "tLessEqualB", 0 },
  { "tEqualI", 0 },
  { "tNotEqualI", 0 },
  { "tGreaterI", 0 },
  { "tLessI", 0 },
  { "tGreaterEqualI", 0 },
  { "tLessEqualI", 0 },
  { "tEqualP", 0 },
  { "tNotEqualP", 0 },
  { "tGreaterP", 0 },
  { "tLessP", 0 },
  { "tGreaterEqualP", 0 },
  { "tLessEqualP", 0 },
  { "tAllocActuals", 1 },
  { "tAllocActualsCdecl", 1 },
  { "tFreeActuals", 1 },
  { "tCall", 1 },
  { "tCallCdecl", 1 },
  { "tReturn", 0 },
  { "tEnter", 1 },
  { "tJump", 1 },
  { "tJumpTrue", 1 },
  { "tJumpFalse", 1 },
  { "tJumpCaseB", 1 },
  { "tJumpCaseI", 1 },
  { "tJumpCaseS", 1 },
  { "tCase", 2 },
  { "tCaseRange", 3 },
  { "tCaseEnd", 1 },
  { "tLabel", 1 },
  { "tLabelAlias", 2 },
  { "tLabelExtern", 2 },
  { "tWriteI", 0 },
  { "tWriteBool", 0 },
  { "tWriteChar", 0 },
  { "tWriteShortStr", 0 },
  { "tWritePChar", 0 },
  { "tWriteP", 0 },
  { "tWriteEnum", 0 },
  { "tWriteD", 0 },
  { "tWriteCR", 0 },
  { "tReadI", 0 },
  { "tReadChar", 0 },
  { "tReadShortStr", 1 },
  { "tReadCR", 0 },
  { "tFile", 1 },
  { "tLine", 1 }
};
int numtCodeInstrs = sizeof(tCodeInstrs) / sizeof(tCodeInstrs[0]);


// Returns the size of a given tcode instruction.
//
int
tcodeInstrSize( int instr )
{
  return tCodeInstrs[instr].args + 1;
}


// Dump tcode to a file.
//
// Given the code block, and the starting address to dump from.
// codeSize is the number of entries in the code table.
// If the last instruction in the code table is incomplete, it will not be dumped.
// Returns the address of the next instruction to be dumped,
// which may be codeSize or less if the last instruction was incomplete.
//
int
tcodeDump( FILE* fp, int* code, int fromPc, int codeSize )
{
  int pc;
  for ( pc = fromPc; pc < codeSize; ) {
    int instr = code[pc];
    if ( instr >= 0 && instr < numtCodeInstrs ) {
       if ( pc + tCodeInstrs[instr].args >= codeSize ) {
         // this instruction is incomplete
         return pc;
       }
    }

    fprintf( fp, "====\t%4d\t", pc );

    if ( instr >= 0 && instr < numtCodeInstrs ) {
      fprintf( fp, "%s", tCodeInstrs[instr].name );
      pc++;
      for ( int a = 0; a < tCodeInstrs[instr].args; ++a ) {
        fprintf( fp, "\t%4d", code[pc++] );
      }
      fprintf( fp, "\n" );
    } else {
      fprintf( fp, "?\n" );
      pc++;
    }
  }
  return pc;
}

