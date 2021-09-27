{  TEST4.PAS  -- Array & pointer references & for statement

        27Sep89                   }

program test (input, output);

type atype = array [1..9] of array[1..2,1..2] of integer;

var  a1,a2: atype;
     i,j:   integer;

begin

  for i := 1 to 9 do
    begin
      if i = 3 then
        cycle;   { "Cycle is a Mac Pascal extension. Borland Pascal has Continue instead." }
      for j := 1 to 2 do
        a1[i][j,j] := i;
    end;

  for i := 9 downto 1 do
    begin
    writeln(a1[i][1,1],a1[i][1,2],a1[i,1][1],a1[i,1][2],a1[i,1,1],a1[i,1,2]);
    writeln(a1[i][2,1],a1[i][2,2],a1[i,2][1],a1[i,2][2],a1[i,2,1],a1[i,2,2]);
    end;

end.
