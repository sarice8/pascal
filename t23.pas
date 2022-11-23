// type casts

program t22;

  const c1 = char( 33 );
  type Direction = ( North, East, South, West );
  const c2 = Direction( 2 );

begin

  writeln( c1 );
  writeln( c2 );

end.
