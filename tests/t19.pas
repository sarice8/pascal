// pointer arithmetic
// treating pointer as array

program t19;
uses testing;

  function strlen( pc: PChar ): integer;
      var len: integer = 0;
      var upper: integer = 0;
    begin
      while pc[ len ] <> #0 do
        begin
          if ( pc[ len ] >= 'A' ) and
             ( pc[ len ] <= 'Z' ) then
            upper := upper + 1;

          len := len + 1;
        end;
      strlen := len;
      writeln( 'strlen sees ', upper, ' uppercase characters' );
    end;

  var text : array [0 .. 31] of char;

  var i1: integer;
  var b1: boolean;

begin

  writeln( 'Hello There' );

  text[0] := 'H';
  text[1] := 'e';
  text[2] := 'l';
  text[3] := 'l';
  text[4] := 'o';
  text[5] := ' ';
  text[6] := 'T';
  text[7] := 'h';
  text[8] := 'e';
  text[9] := 'r';
  text[10] := 'e';
  text[11] := chr( 0 );

  writeln( strlen( @text[0] ) );

{
  i1 := 256;
  // fpc truncates this, rather than converting non-0 to true.  I'll do the same.
  b1 := boolean( i1 );
  if b1 then
    writeln( 'See true' );
}

end.
