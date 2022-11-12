// strings

program t20;

  var s1: ShortString;
  var c: char;
  var i1: integer;

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

end.
