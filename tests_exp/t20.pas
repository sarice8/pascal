// strings

program t20;

  var a1: AnsiString;
  var p1: PChar;
  var p2: PChar;
  var s1: ShortString;
  var c: char;
  var i1: integer;

  procedure proc1( var s1: ShortString );
    begin
      writeln( 'proc1 sees ', s1 );
    end;

  procedure proc2( s1: ShortString );
    begin
      writeln( 'proc2 sees ', s1 );
    end;


begin

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

  // Does fpc see a null termination after assigning to PChar ??
  for i1 := 0 to 260 do
    write( ord( p1[i1] ), ' ' );
  writeln;
  s1 := 'Hello';

  // p1 := s1;
  //   -- fpc: Error: incompatible types, got ShortStr, expected PChar
  //      Note that fpc did allow  p1 := <strlit> above
  //      so it must differentiate between strlit and ShortString
  p1 := @s1[1];
  for i1 := 0 to 260 do
    write( ord( p1[i1] ), ' ' );
  writeln;


  a1 := 'Hello There';
  writeln( a1 );

  p1 := 'Good Day';
  writeln( p1 );

  a1 := 'Welcome ';
  a1 := a1 + 'Back';
  writeln( a1 );

  // fpc: This worked, probably because compiler handled static strlit addition
  // p1 := 'Be Seeing ' + 'You';
  // writeln( p1 );

  // p1 := 'Be Seeing ';
  // p1 := p1 + 'You';
  //   -- fpc: Error, got shortstring, expected pchar
  // writeln( p1 );

  // fpc: this worked
  p1 := 'Be Seeing ';
  s1 := p1 + 'You';
  writeln( s1 );

  // p1 := 'Catch You ';
  // p2 := 'Later';
  // s1 := p1 + p2;
  //  -- fpc: Error, operation + not supported for types PChar and PChar
  //     This makes me think that fpc treats strlit as ShortString
  // writeln( s1 );

  // Passing a strlit into a VAR shortstring parameter.  Not allowed.
  // proc1( 'A Strlit' );
  //   fpc: Error: variable identifier expected

  // Ok to pass a strlit to a non-VAR shortstring parameter
  proc2( 'A Strlit' );

end.
