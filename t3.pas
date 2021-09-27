{  TEST3.PAS  -- Array & pointer references

        24Sep89                   }

program test (input, output);

type atype = array [1..10] of integer;

type ptr =  ^integer;

var  i:     integer;
     a1,a2: atype;
     p:     ptr;
     a:     ^atype;

begin

  a := ^a1;
  for i := 1 to 10 do
    begin
      p := ^a^[i];
      p^ := i;
    end;

  a2 := a1;

  for i := 1 to 10 do
    writeln(i, a1[i], a2[i], ^a1[i], ^a2[i]);

end.
