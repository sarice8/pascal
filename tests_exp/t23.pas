// type casts

program t22;

  const c1 = char( 33 );
  type Direction = ( North, East, South, West );
  const c2 = Direction( 2 );

  var i1: integer;
  var i2: longint;
  var c3: char;

begin

  writeln( c1 );
  writeln( c2 );
  i1 := integer( Succ( Succ( East ) ) );
  writeln( i1 );

  //writeln( 'sizeof(integer)=', sizeof( integer ) );
  //writeln( 'sizeof(longint)=', sizeof( longint ) );

  // lvalue typecast is only allowed if the size matches.
  // No conversion code occurs.  Being an lvalue, it Can be used on lhs of assignment,
  // after @, or passing to a var parameter.

  // fpc error: typecast has different size (2 -> 4) in assignment
  // (because its "integer" is int16).  I would allow it since my integer is int32.
  // Direction( i1 ) := North;
  // writeln( i1 );

  Direction( i2 ) := North;
  writeln( i2 );

  // fpc: error: typecast has different size (1 -> 4) in assignment
  // Direction( c3 ) := East;
  // writeln( c3 );

end.
