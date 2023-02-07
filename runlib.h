#ifndef RUNLIB_H
#define RUNLIB_H


struct EnumNameTable {
  int value;
  int padding;
  const char* name;
};


void runlibWriteI( int val );
void runlibWriteBool( bool val );
void runlibWriteChar( char val );
void runlibWriteShortStr( const char* ptr );
void runlibWritePChar( const char* ptr );
void runlibWriteP( const void* ptr );
void runlibWriteEnum( int val, const EnumNameTable* table );
void runlibWriteD( double val );
void runlibWriteCR();

void runlibReadI( int* ptr );
void runlibReadChar( char* ptr );
void runlibReadShortStr( char* ptr, int capacity );
void runlibReadCR();

int runlibShortStrCmp( const char* shortStrA, const char* shortStrB );
void* runlibMalloc( int size );
void* runlibRealloc( void* ptr, int size );
void runlibFree( void* ptr );

double runlibArctan( double x );
double runlibCos( double radians );
double runlibExp( double x );
double runlibLn( double x );
int runlibRound( double x );
double runlibSin( double radians );
double runlibSqrt( double x );
int runlibTrunc( double x );

void grInit();
void grTerm();
void runlibClearScreen();
void runlibUpdateScreen();
void runlibSetPixel( int x, int y, int color );
int  runlibGetPixel( int x, int y );
void runlibDelay( int milliseconds );
int  runlibWaitKey();

// ---

void* runlibLookupMethod( const char* name );


#endif
