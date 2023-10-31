// const expressions

program t22;

  const c1 = 1;
  const c2 : integer = 2;
  const c3 = c1 < 10;

  // const c4 = c2 < 10;
  //   fpc: illegal expression

  // const c5 : boolean = c2 < 10;
  //   fpc: illegal expression
  //   I catch this too.

  const c6 = c1 + 2;

  type Direction = ( North, East, South, West );
  const c7 = South;
  const c8 = c7 < East;
  const c9 = 'Tango' > 'Cash';
  const c10 = 'Blueberry' + ' ' + 'Muffin';
  const c11 = 'I think ' + c10 + ' is tasty';
  const c12 = Succ( East );

begin

  writeln( c1 );
  writeln( c3 );  // fpc: TRUE
  writeln( c6 );
  writeln( c7 );
  writeln( c8 );
  writeln( c9 );
  writeln( c10 );
  writeln( c11 );
  writeln( c12 );

end.
