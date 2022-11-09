// pointer arithmetic

program t18;

  type T1 = array[1..10] of integer;
  const c1: T1 = ( 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 );

  var i1: integer;
  var p1: ^integer;

begin

  p1 := @c1[1];
  for i1 := 1 to 10 do
    begin
      write( p1^, ' ' );

      // I know that we can't assign into const
      // c1[i1] := i1;
      // But I don't catch assigning into const via pointer.  fpc is same.
      p1^ := i1;

      p1 := p1 + 1;
    end;
  writeln;

  for i1 := 1 to 10 do
    begin
      write( c1[i1], ' ' );
    end;
  writeln;

end.
