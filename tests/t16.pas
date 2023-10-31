program t16;
uses testing;

  // byte, char types
  // ord, chr methods

  type Direction = (North, East, South, West);
  type Dir2 = (N2, E2, S2 = 10, W2);
  // type Dir2 = (N2, E2, S2 = 2, W2);
  
  var b1, b2: byte;
  var c1, c2: char;
  var i1: integer;

  var d1: Direction;
  var d2: Dir2;

  // These are the built-in integer types, according to fpc docs (and their representation in cstdint terms).
  // At this moment I only support byte and integer.
  //
  // byte       uint8_t
  // shortint   int8_t
  // smallint   int16_t
  // word       uint16_t
  // integer    = longint  int32_t
  // cardinal   = longword uint21_t
  // longint    int32_t
  // longword   uint32_t
  // int64      int64_t
  // qword      uint64_t
  // "An integer constant gets the smallest signed type."

  // function ord( x ) : integer;  - x may be any scalar type, including enum, boolean, char, integer, and their subranges.
  // function chr( x ) : char;     - x is integer between 0 and 255.  I see that fpc simply truncates low byte.

begin

  writeln( 'Hello '#65#66#67#68 );
  // fpc doesn't allow spaces between char codes:
  //   writeln( 'Hello '#65 #66 #67 #68 );
  writeln( #65#66#67'There' );

  c1 := 'a';
  writeln( c1, ' ', ord( c1 ), ' ', 'z' );

  b1 := 130;
  i1 := b1;

  writeln( 'b1=', b1, ' i1=', i1 );

{
  c1 := #130;
  i1 := integer(c1);
  writeln( 'c1=', c1, ' i1=', i1 );
}

  i1 := 256 + 50;
  c1 := chr( i1 );
  writeln( 'c1=', c1 );
  i1 := ord( chr( i1 ) );
  writeln( 'i1=', i1 );

  i1 := 10;
  b1 := 10;
  if i1 = b1 then
    writeln( 'int/byte comparison worked' );

  d1 := West;
  i1 := ord( d1 );
  writeln( 'i1=', i1 );
  writeln( 'ord=', ord( East ) );
  
  c1 := chr(49);
  writeln( 'c1=', c1 );
  i1 := ord( c1 );
  writeln( 'i1=', i1 );

  d1 := West;
  writeln( d1 );
  writeln( pred( d1 ) );
  // writeln( succ( d1 ) );
  //   runtime error 107
  // writeln( pred( North ) );
  //   error: range check error while evaluating constants
  //   (-1 must be between 0 and 3)
  
  d2 := E2;
  writeln( d2 );
  // d2 := succ( d2 );
  //   error: succ or pred on enums with assignments not possible
  //   (This only errors out if the assignment left a gap)
  writeln( d2 );

  // fpc doesn't detect an out-of-bounds error here.
  // And indeed it shows ord == 4 which is out of bounds.
  d1 := succ( d1 );
  i1 := ord( d1 );
  writeln( i1 );
  
end.
