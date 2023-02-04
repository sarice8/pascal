// mysystem - built-in declarations and definitions.
//
// This unit is used automatically by every module.
//

unit mysystem;

interface

type PChar = ^Char;   // by convention, PChar is expected to point to null-terminated text, aka C string.
type PByte = ^Byte;
type PInteger = ^Integer;

// For now, type String always means ShortString.
type String = ShortString;

// In fpc, longint is int32 and integer is int16.
// For me, both are int32.
type LongInt = Integer;

// Real may be Single or Double.  Matching fpc.
type Real = Double;

// My version of AnsiString is similar to C++ std::string
// As with the latest std::string, it doesn't use reference counting.
// I don't yet have small-string optimization.
// value is always an allocated buffer, containing null-terminated text.
//
// Record internals are for use by compiler.
//
type AnsiString = record
  length: integer;
  capacity: integer;
  value: PChar;
end;


// Public methods
//
function Length( var str: AnsiString ): integer;

function strlen( pc: PChar ): integer;
procedure memcpy( dest: Pointer; src: Pointer; size: integer );

// TO DO: a built-in SizeOf( type )  would be nice, to use with memcpy


const PI = 3.141592653589793238462643383;

// Standard Pascal math functions.
// (fpc has additional functions in unit math)

// TO DO: support overloading, for both pascal methods and cdecl methods.
// function abs( x: integer ): integer;

function abs( x: double ): double;

function arctan( radians: double ): double; cdecl;
  external 'runlib' name 'runlibArctan';

function cos( radians: double ): double; cdecl;
  external 'runlib' name 'runlibCos';

function exp( x: double ): double; cdecl;
  external 'runlib' name 'runlibExp';

function ln( x: double ): double; cdecl;
  external 'runlib' name 'runlibLn';

// round to nearest integer; .5 rounds to closest even integer
function round( x: double ): integer; cdecl;
  external 'runlib' name 'runlibRound';

function sin( radians: double ): double; cdecl;
  external 'runlib' name 'runlibSin';

// square (power 2)
function sqr( x: double ): double;

function sqrt( x: double ): double; cdecl;
  external 'runlib' name 'runlibSqrt';

// round towards zero
function trunc( x: double ): integer; cdecl;
  external 'runlib' name 'runlibTrunc';


// For use by compiler
// -------------------

// Returns universal pointer type.
//
function malloc( size: integer ): Pointer; cdecl;
  external 'runlib' name 'runlibMalloc';

function realloc( ptr: Pointer; size: integer ): Pointer; cdecl;
  external 'runlib' name 'runlibRealloc';

procedure free( ptr: Pointer ); cdecl;
  external 'runlib' name 'runlibFree';

procedure ShortStringAppendShortString( strA: ^ShortString; strB: ^ShortString );
procedure ShortStringAppendChar( strA: ^ShortString; c: Char );

// Compare two ShortStrings.
// Returns -1 if A < B,  0 if A = B,  1 if A > B
//
function ShortStringCmp( strA: ^ShortString; strB: ^ShortString ) : integer; cdecl;
  external 'runlib' name 'runlibShortStrCmp';

procedure ConstructAnsiString( var str: AnsiString );
procedure DestroyAnsiString( var str: AnsiString );
function  AnsiStringToPChar( var str: AnsiString ): PChar;
procedure AssignAnsiStringFromPChar( var str: AnsiString; from: PChar );


implementation

function abs( x: double ): double;
  begin
    if x >= 0.0 then
      abs := x
    else
      abs := -x;
  end;

function sqr( x: double ): double;
  begin
    sqr := x * x;
  end;


procedure ShortStringAppendShortString( strA: ^ShortString; strB: ^ShortString );
    var lengthA, lengthB : integer;
        freeA : integer;
        toCopy : integer;
  begin
    lengthA := ord( strA^[0] );
    lengthB := ord( strB^[0] );
    freeA := 255 - lengthA;  // assuming default size of ShortString
    toCopy := lengthB;
    if toCopy > freeA then
      toCopy := freeA;
    memcpy( @strA^[lengthA+1], @strB^[1], toCopy );
    strA^[0] := chr( lengthA + toCopy );
  end;

procedure ShortStringAppendChar( strA: ^ShortString; c: Char );
    var lengthA : integer;
  begin
    lengthA := ord( strA^[0] );
    // assuming default size of ShortString
    if lengthA < 255 then
      begin
        strA^[lengthA+1] := c;
        strA^[0] := chr( lengthA + 1 );
      end;
  end;


{
  // This was moved into runlib, since needed by string case statement

// Compare two ShortStrings.
// Returns -1 if A < B,  0 if A = B,  1 if A > B
//
function ShortStringCmp( strA: ^ShortString; strB: ^ShortString ) : integer;
    var remainA, remainB : integer;
        pA, pB : PChar;
    label done;
  begin
    remainA := ord( strA^[0] );
    remainB := ord( strB^[0] );
    pA := @strA^[1];
    pB := @strB^[1];
    while remainA > 0 do
      begin
        if ( remainB = 0 ) or ( pA^ > pB^ ) then
          begin
            ShortStringCmp := 1;
            goto done;
          end;
        if pA^ < pB^ then
          begin
            ShortStringCmp := -1;
            goto done;
          end;
        remainA := remainA - 1;
        remainB := remainB - 1;
      end;

    if remainB > 0 then
      ShortStringCmp := -1
    else
      ShortStringCmp := 0;

    done: ;
  end;
}

procedure ConstructAnsiString( var str: AnsiString );
  begin
    str.length := 0;
    str.capacity := 32;
    str.value := malloc( 32 );
  end;

procedure DestroyAnsiString( var str: AnsiString );
  begin
    free( str.value );
  end;


// Private helper.  Increase string capacity, so it could hold
// a string of the given length.
//
procedure BumpAnsiStringCapacity( var str: AnsiString; requiredLength: integer );
    var newCapacity: integer;
  begin
    // Find the new capacity.  In machine code we could use the "bsr" instruction
    // to find the highest set bit in requiredLength.  Here I do it the slow way.
    newCapacity := 32;
    while newCapacity <= requiredLength do
      newCapacity := newCapacity * 2;
    str.capacity := newCapacity;
    str.value := realloc( str.value, newCapacity );
  end;


function AnsiStringToPChar( var str: AnsiString ): PChar;
  begin
    AnsiStringToPChar := str.value;
  end;

procedure AssignAnsiStringFromPChar( var str: AnsiString; from: PChar );
    var fromLen: integer;
        newLen: integer;
  begin
    fromLen := strlen( from );
    newLen := str.length + fromLen;
    if newLen >= str.capacity then
      BumpAnsiStringCapacity( str, newLen );
    memcpy( str.value + str.length, from, fromLen );
    str.length := newLen;
  end;


function Length( var str: AnsiString ): integer;
  begin
    Length := str.length;
  end;


// TO DO: this should be a builtin instead, so I can use faster machine code
function strlen( pc: PChar ): integer;
    var len: integer; 
  begin
    // TO DO: a pointer can be treated as an array of pointed-to values
    len := 0;
    while pc[ len ] <> #0 do
      len := len + 1; 
    strlen := len;

    // OR:
    // TO DO: support pointer math, same as C.  ptr +- integer, ptr - ptr
    //        (those are scaled by element size.)
    //        There are also two methods  increment(pointer)  decrement(pointer)
    //        which are the same as ptr := ptr + 1;  or - 1.
    // var p: pchar;
    // p := pc;
    // while p^ <> #0 do
    //   p := p + 1;
    // strlen := p - pc;
  end;



// TO DO: this should be a builtin instead, so I can use faster machine code
procedure memcpy( dest: Pointer; src: Pointer; size: integer );
    var i: integer;
  begin
    for i := 0 to size - 1 do
      PByte(dest)[i] := PByte(src)[i];
  end;


initialization
  begin
    // writeln( 'Hello from system.initialization' );
  end;

finalization
  begin
    // writeln( 'Goodbye from system.finalization' );
  end

end.

