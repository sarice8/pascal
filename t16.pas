program t16;
  // byte, char types
  
  var b1, b2: byte;
  var c1, c2: char;
  var i1: integer;

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

  b1 := 130;
  i1 := b1;

  writeln( 'b1=', b1, ' i1=', i1 );

{
  c1 := #130;
  i1 := integer(c1);
  writeln( 'c1=', c1, ' i1=', i1 );

  i1 := 256 + 49;
  c1 := chr( i1 );
  writeln( 'c1=', c1 );
  i1 := ord( chr( i1 ) );
  writeln( 'i1=', i1 );
}

  i1 := 10;
  b1 := 10;
  if i1 = b1 then
    writeln( 'int/byte comparison worked' );

end.
