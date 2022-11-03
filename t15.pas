program t15;
  // enum types, goto
  
  type Direction = (North, East, South, West);
  var d1: Direction = East;
  label label1, label2;
  label 1, 2;

  var count: integer;

begin

  count := 0;
  label1: writeln( 'd1=', d1 );
  // writeln( ' ord=', ord(d1) );
  // d1 := pred(d1);
  // writeln( 'd1=', d1 );
  // writeln( ' ord=', ord(d1) );

  count := count + 1;
  if count < 3 then
    goto label1;

  // goto label2;  -- error: used but not defined

  count := 0;
  1: write( 'Hey' );
  count := count + 1;
  if count < 3 then
    goto 1;
  writeln( '' );

end.
