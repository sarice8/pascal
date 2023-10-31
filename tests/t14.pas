program t14;
uses testing;

  // working with byte and char data types
  // and enum types
  
  var b1, b2: byte;
      i1, i2: integer;
      c1, c2: char;

  type Direction = (North, East, South, West);
  var d1: Direction = East;

begin
  b1 := 10;
  i1 := 2;
  b2 := b1 * i1;
  writeln( 'b1=', b1, ' b2=', b2 );  // 10, 20

  b1 := 10;
  b2 := 30;
  i1 := b1 * b2;
  writeln( 'i1=', i1 );  // 300

  b1 := 10;
  b2 := 30;
  b1 := b1 * b2;
  writeln( 'b1=', b1 );  // 44  (ok, the low byte)

  b1 := 127;
  b2 := 3;
  b1 := b1 + b2;
  writeln( 'b1=', b1 );  // 130.  ok, byte is unsigned.

  b1 := 127;
  b2 := -1;
  b1 := b1 * b2;
  writeln( 'b1=', b1, ' b2=', b2 );  // 129, 255.  Appears to have done mult by 255 then truncation.



  b1 := 255;
  b2 := 10;
  i1 := b1 * b2;
  writeln( 'i1=', i1 );  // 2550

  b1 := 255;
  i1 := -1;
  i1 := b1 * i1;
  writeln( 'i1=', i1 );  // -255

  // b1 := 255;
  // c1 := chr( 2 );
  // i1 := b1 * c1;  // Error: operator is not overloaded: "Byte" * "Char"

  // c1 := chr( 2 );
  // c2 := chr( 40 );
  // i1 := c1 * c2;    // Error: operator is not overloaded: "Char" * "Char"

  writeln( 'd1=', d1, ' ord=', ord(d1) );
  d1 := pred(d1);
  writeln( 'd1=', d1, ' ord=', ord(d1) );

end.
