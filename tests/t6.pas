
{  t6.pas     Nov 8, 1990    }

program test (input, output);
uses testing;

var   i,j:   integer;

begin

  writeln;
  writeln;

  for i := 1 to 10 do
    begin
      for j := 1 to i do
          write ('    ');
      writeln ('Testing Testing!');
      for j := 10 downto i do
          write ('    ');
      writeln (' Testing Testing!');

    end;

  writeln;
  writeln;

end.
