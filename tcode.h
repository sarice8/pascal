#ifndef TCODE_H
#define TCODE_H

int tcodeInstrSize( int instr );
int tcodeDump( FILE* fp, int* code, int fromPc, int codeSize );

#endif
