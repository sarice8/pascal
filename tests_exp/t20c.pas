// strings

program t20;

  var i1: integer;
  var p1: ^integer;

begin

  i1 := 10;
  p1 := @i1;

  writeln( p1[0] );

  p1[0] := 20;
  writeln( p1[0] );

  writeln( i1 );

end.
