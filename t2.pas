{  TEST2.PAS  -- Fibonacci numbers

        30Jul89  test scanner.c
        21Sep89  test framework of parser in ssl         }

PROGRAM test2 (Input,Output);

TYPE arrayType = array [1..10] of integer;

VAR f1, f2, temp : INTEGER;
    a1, a2 :       arrayType;
    ptr :          ^integer;

BEGIN
  f1 := 1;
  f2 := 1;
  while (f2 <= 100) do
    begin
      write (f2);
      temp := f1 + f2;
      f1 := f2;
      f2 := temp;
    end;
  writeln;

{
  f1 := 0;
  repeat
    write(f1);
    if 4 < f1 then
      if f1 < 7 then
        writeln(-1)
      else
    else
      writeln(-2);
    f1 := f1 + 1;
  until f1 > 10;
}

  f1 := 1;
  repeat
    writeln(f1, f1>5);
    a1[f1] := 11 - f1;
    f1 := f1 + 1;
  until f1>10;

  a2 := a1;
  f1 := 1;
  repeat
    writeln(a1[f1],a2[f1]);
    f1 := f1 + 1;
  until f1 > 10;

  writeln(ptr);

END.
