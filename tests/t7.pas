//  TEST7.PAS  -- checking case sensitivity



program test (input, output);
uses testing;

type atype = array [1..9] of array[1..2,1..2] of INTEGER;

var  a1,a2: atype;
     i,j:   integer;

BEGIN

  writeln ('Here we go!');

  for i := 1 to 9 do
    begin
      if I = 3 then
        continue;
      for j := 1 to 2 do
        a1[i][J,J] := i;
    end;

  FOR I := 9 DOWNTO 1 DO
    begin
    writeln('-> ',a1[i][1,1],a1[i][1,2],a1[I,1][1],a1[i,1][2],a1[i,1,1],a1[i,1,2]);
    writeln('-> ',a1[i][2,1],a1[i][2,2],a1[I,2][1],a1[i,2][2],a1[i,2,1],a1[i,2,2]);
    end;

  write ('That''s ','all ');
  WRITELN ('folks!');

END.
