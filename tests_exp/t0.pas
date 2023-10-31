
PROGRAM test syntax error (Input,Output);

CONST magic  = -27;
      magic2 = -magic;

VAR   x, y, i: integer;

BEGIN
  x := magic;
  i := 1;
  while (i <= 10) do
    begin
      y := i * 5;
      writeln (x, ', ', y, ', ', x+y);
      i := i + 1;
    end;
END.
