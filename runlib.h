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
void runlibWriteShortStr( char* ptr );
void runlibWritePChar( char* ptr );
void runlibWriteP( char* ptr );
void runlibWriteEnum( int val, EnumNameTable* table );
void runlibWriteCR();
void* runlibMalloc( int size );
void* runlibRealloc( void* ptr, int size );
void runlibFree( void* ptr );

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
