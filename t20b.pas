// strings
// ^

program t20;

  var s1: ShortString;
  var c: char;
  var i1: integer;
  var p1: PChar;


  procedure proc2( s1: ShortString );
    begin
      writeln( 'proc2 sees ', s1 );
    end;


begin

  writeln( 'Welcome to this string test' );

  s1 := 'Message Here';
  writeln( s1 );


 
  // can a strlit be dereferenced? Yes.
  // fpc says 'e' which suggests it represents the strlit as ShortString
  // rather than PChar.  Or array [1..n] of char, with null termination.
  c := 'Hello'[2];
  writeln( c );

  // fpc says 5 which agrees with strlit represented as ShortString.
  i1 := ord( 'Hello'[0] );
  writeln( i1 );

  // Here, fpc says 'l' which makes agrees with PChar pointer arithmetic.
  p1 := 'Hello';
  c := p1[2];
  writeln( c );

  p1 := 'Good Day';
  writeln( p1 );

  proc2( 'A Strlit' );


  s1 := 'Did it work';
  s1 := '-' + '>' + ' ' + s1 + '??';
  //s1 := s1 + '?';
  writeln( s1 );

end.
