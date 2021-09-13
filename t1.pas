{  TEST.PAS
      Simple test of Steve's Pascal compiler.
      HISTORY
        30Jul89  test scanner.c
        21Sep89  test framework of parser in ssl         }

PROGRAM test (Input,Output);

CONST magic  = -27;
      magic2 = -magic;

VAR   x, y, i: integer;

BEGIN
  x := magic;
  i := 1;
  while (i <= 10) do
    begin
      y := i * 5;
      writeln (x, y, x+y);
      i := i + 1;
    end;
END.
