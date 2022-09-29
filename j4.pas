// jit testing

program test (input, output);

var I1: integer;
var I2: integer;
var I3: integer;

begin

  I1 := 0;
  I2 := 0;

  // if ( ( I1 < 3 ) and ( I2 > 10 ) and ( I1 <> I2 ) ) then
  if ( I1 < 3 ) and ( I2 > 10 ) and ( I1 <> I2 ) then
    I3 := 100
  else
    I3 := 200;

end.
