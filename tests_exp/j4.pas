// jit testing

program test (input, output);

var I1: integer;
var I2: integer;
var I3: integer;
var B1: boolean;
var B2: boolean;

begin

  I1 := 10;
  I2 := 2;
  I3 := I1 + I2;
  I3 := -I3;

  I3 := ( I2 * 2 ) + (I1 / I2) + ( I2 * 2 );
  writeln( 'I3 = ', I3 );

  if I1 = 0 then
    writeln( 'Hello' );

  B1 := I1 < 3;
  writeln( 'B1 = ', B1 );
  B1 := not ( I1 < 3 );
  writeln( 'B1 = ', B1 );

  if ( ( I1 < 3 ) and ( I2 > 10 ) and ( I1 <> I2 ) ) then
    I3 := 100
  else
    I3 := 200;

  while ( I1 < 5 ) do
    begin
      writeln ( 'I1 = ', I1 );
      I1 := I1 + 1;
    end;

  repeat
    begin
      I1 := I1 - 1;
      writeln ( 'and now I1 = ', I1 );
    end
    until I1 < 0;

end.
