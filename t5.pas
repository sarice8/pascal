{  TEST5.PAS  -- Array & pointer references & for statement

        01Jan90                   }

  // Another comment
  (* And yet
     another comment, old-style *)

program test (input, output);

type atype = array [1..9] of array[1..2,1..2] of integer;

var  a1,a2: atype;
     i,j:   integer;

begin

  writeln ('Here we go!');

  for i := 1 to 9 do
    begin
      if i = 3 then
        continue;
      for j := 1 to 2 do
        a1[i][j,j] := i;
    end;

  for i := 9 downto 1 do
    begin
    writeln('-> ',a1[i][1,1],a1[i][1,2],a1[i,1][1],a1[i,1][2],a1[i,1,1],a1[i,1,2]);
    writeln('-> ',a1[i][2,1],a1[i][2,2],a1[i,2][1],a1[i,2][2],a1[i,2,1],a1[i,2,2]);
    end;

  write ('That''s ','all ');
  writeln ('folks!');

end.
