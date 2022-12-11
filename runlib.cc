/*

  runlib.cc

  Runtime library functions.

*/


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

#include <string>
#include <stack>
#include <vector>
#include <unordered_map>

#include <SDL.h>
//   -- for my graphics api

#include "runlib.h"


// For now, I have a hardcoded list of available external methods.
//
std::unordered_map<std::string, void*> runlibMethods = {
  { "runlibShortStrCmp", (void*) runlibShortStrCmp },
  { "runlibMalloc", (void*) runlibMalloc },
  { "runlibRealloc", (void*) runlibRealloc },
  { "runlibFree", (void*) runlibFree },
  { "runlibClearScreen", (void*) runlibClearScreen },
  { "runlibUpdateScreen", (void*) runlibUpdateScreen },
  { "runlibSetPixel", (void*) runlibSetPixel },
  { "runlibGetPixel", (void*) runlibGetPixel },
  { "runlibDelay", (void*) runlibDelay },
  { "runlibWaitKey", (void*) runlibWaitKey },
};


void*
runlibLookupMethod( const char* name )
{
  auto it = runlibMethods.find( std::string(name) );
  if ( it != runlibMethods.end() ) {
    return it->second;
  } else {
    return nullptr;
  }
}


void
runlibWriteI( int val )
{
  printf( "%d", val );
}

void
runlibWriteBool( bool val )
{
  printf( val ? "TRUE" : "FALSE" );
}

void
runlibWriteChar( char val )
{
  printf( "%c", val );
}

void
runlibWriteShortStr( const char* ptr )
{
  int len = uint8_t( ptr[0] );
  for ( int i = 1; i <= len; ++i ) {
    printf( "%c", ptr[i] );
  }
}

void
runlibWritePChar( const char* ptr )
{
  printf( "%s", ptr );
}

void
runlibWriteP( const void* ptr )
{
  printf( " <%p>", ptr );
}

void
runlibWriteEnum( int val, const EnumNameTable* table )
{
  for ( int i = 0; table[i].name != nullptr; ++i ) {
    if ( table[i].value == val ) {
      printf( "%s", table[i].name );
      return;
    }
  }
  printf( "<?badEnum>" );
}

void
runlibWriteD( double val )
{
  printf( "%g", val );
}

void
runlibWriteCR()
{
  printf( "\n" );
}


void
runlibReadI( int* ptr )
{
  // Skip leading whitespace
  // per fpc experiments: That whitespace may include newlines.
  FILE* f = stdin;
  int c = fgetc( f );
  while ( isspace( c ) ) {
    c = fgetc( f );
  }
  if ( c == EOF ) {
    *ptr = 0;
    return;
  }
  // per experiments with fpc: consume all characters up to but not including whitespace
  std::string token;
  token.push_back( char(c) );
  c = fgetc( f );
  while ( !isspace( c ) ) {
    token.push_back( char(c) );
    c = fgetc( f );
  }
  ungetc( c, f );
  // Attempt to interpret that entire sequence as a signed integer.  Error if not entirely valid.
  // TO DO: detect error
  *ptr = atoi( token.c_str() );
}

void
runlibReadChar( char* ptr )
{
  FILE* f = stdin;
  int c;
  c = fgetc( f );
  if ( c == '\r' || c == '\n' ) {
    // Doesn't read eoln
    ungetc( c, f );
    c = '\0';
  } else if ( c == EOF ) {
    c = 0;
  }
  *ptr = c;
}


void
runlibReadShortStr( char* ptr, int capacity )
{
  // per fpc: Read all characters up to but not including newline, or until capacity filled.
  // Does not consume the newline.
  FILE* f = stdin;
  int len = 0;
  int c;
  while ( len < capacity ) {
    c = fgetc( f );
    if ( c == '\r' || c == '\n' ) {
      ungetc( c, f );
      break;
    } else if ( c == EOF ) {
      break;
    }
    ++len;
    ptr[len] = c;
  }
  ptr[0] = char( len );
}

void
runlibReadCR()
{
  // per fpc experiments: consume -all- characters up to and including end-of-line sequence.
  // Allow the end-of-line sequence for any platform, not just the native platform.
  FILE* f = stdin;
  int c;
  c = fgetc( f );
  while ( c != '\r' && c != '\n' && c != EOF ) {
    c = fgetc( f );
  }
  if ( c == '\r' ) {
    // Allow for windows sequence \r\n
    c = fgetc( f );
    if ( c != '\n' ) {
      ungetc( c, f );
    }
  }
}



// Compare two Pascal ShortStrings.
// Return negative if A < B,  0 if A = B,  positive if A > B
//
int
runlibShortStrCmp( const char* shortStrA, const char* shortStrB )
{
  int lengthA = shortStrA[0];
  int lengthB = shortStrB[0];
  int result = strncmp( &shortStrA[1], &shortStrB[1], std::min( lengthA, lengthB ) );
  if ( result == 0 ) {
    if ( lengthA < lengthB ) {
       result = -1;
    } else if ( lengthA > lengthB ) {
       result = 1;
    }
  }
  return result;
}



// Runlib methods declared in unit mysystem

void*
runlibMalloc( int size )
{
  return malloc( size );
}


void*
runlibRealloc( void* ptr, int size )
{
  return realloc( ptr, size );
}


void
runlibFree( void* ptr )
{
  free( ptr );
}


// ----------------------------------------------
// My mini graphics api, with SDL2 under the hood
// ----------------------------------------------

bool grInitialized = false;
SDL_Window* grWindow = nullptr;
SDL_Renderer* grRenderer = nullptr;
SDL_Texture* grBuffer = nullptr;  // this is what program will draw into
uint32_t* grPixels = nullptr;   // or actually this is
bool grPixelsPending = false;

int grBufferX = 320;
int grBufferY = 240;
int grScale = 4;  // scaling up the pixels, from buffer to screen
int grWindowX = grBufferX * grScale;
int grWindowY = grBufferY * grScale;


void
grInit()
{
  if ( grInitialized ) {
    // maybe should clear screen
    return;
  }

  SDL_Init( SDL_INIT_VIDEO );
  grWindow = SDL_CreateWindow( "Pascal",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      grWindowX, grWindowY, 0 );
  grRenderer = SDL_CreateRenderer( grWindow, -1, 0 );
  grBuffer = SDL_CreateTexture( grRenderer,
      SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
      grBufferX, grBufferY );
  grPixels = new uint32_t[ grBufferX * grBufferY ];
  memset( grPixels, 0, grBufferX * grBufferY * sizeof( uint32_t ) );
  grPixelsPending = true;
  grInitialized = true;
}

void
grLazyInit()
{
  if ( !grInitialized ) {
    grInit();
  }
}

void
grTerm()
{
  if ( grInitialized ) {
    // wait until user closes window

    bool quit = false;
    SDL_Event event;

    while ( !quit ) {
      runlibUpdateScreen();

      SDL_WaitEvent( &event );
      switch( event.type ) {
        case SDL_QUIT:
          quit = true;
          break;
        default:
          break;
      }
    }
  }
  delete[] grPixels;
  SDL_DestroyTexture( grBuffer );
  SDL_DestroyRenderer( grRenderer );
  SDL_DestroyWindow( grWindow );
  SDL_Quit();
}

void
runlibClearScreen()
{
  grLazyInit();
  memset( grPixels, 0, grBufferX * grBufferY * sizeof( uint32_t ) );
  grPixelsPending = true;
}

void
runlibUpdateScreen()
{
  if ( grPixelsPending ) {
    SDL_UpdateTexture( grBuffer, nullptr, grPixels, grBufferX * sizeof( uint32_t ) );
    // fill the renderer with the current drawing color.  probably I don't need.
    // SDL_RenderClear( grRenderer );

    // copy entire grBuffer to grRenderer, scaling to fit.
    // srcRect == null indicates entire texture.
    // destRect == null indicates entire rendering target. 
    SDL_Rect* srcRect = nullptr;
    SDL_Rect* destRect = nullptr;
    SDL_RenderCopy( grRenderer, grBuffer, srcRect, destRect );
    // show the renderer's updates on the screen
    SDL_RenderPresent( grRenderer );

    grPixelsPending = false;
  }
}

void
runlibSetPixel( int x, int y, int color )
{
  grLazyInit();
  if ( x >= 0 && x < grBufferX &&
       y >= 0 && y < grBufferY ) {
    grPixels[ x + y * grBufferX ] = color;
    grPixelsPending = true;
  }
}

int
runlibGetPixel( int x, int y )
{
  grLazyInit();
  if ( x >= 0 && x < grBufferX &&
       y >= 0 && y < grBufferY ) {
    return grPixels[ x + y * grBufferX ];
  } else {
    return 0;
  }
}

void
runlibDelay( int milliseconds )
{
  // Do this under the hood, in case there's anything new to show
  runlibUpdateScreen();
  SDL_Delay( milliseconds );
}

// Waits for a key to be pressed.
// Returns the SDL_Keycode of the key that was hit.
// (That's a slightly more generic form of scancode.)
//
// Note, the SDL graphical window needs to be in focus,
// not the terminal window.  I'm not sure if SDL keyboard events
// can be used without a graphical window.
//
int
runlibWaitKey()
{
  // uses SDL, so need at least SDL_Init
  grLazyInit();
  // Also before we wait, bring the screen up-to-date, similar to runlibDelay().
  runlibUpdateScreen();

  SDL_Event event;
  while ( true ) {
    SDL_WaitEvent( &event );

    if ( event.type == SDL_KEYDOWN ) {
      // Maybe: ignore if event.key.repeat != 0 because that's a repeat
      // "Note: if you are looking for translated character input,
      //   see the SDL_TEXTINPUT event."
      // SDL_Scancode = event.key.keysym.scancode;
      SDL_Keycode sym = event.key.keysym.sym; // SDL virtual key code
      return (int) sym;
    }
  }
}


