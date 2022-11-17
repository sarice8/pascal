// const expressions

program t22;

  const c1 = 1;
  const c2 : integer = 2;
  const c3 = c1 < 10;

  // const c4 = c2 < 10;
  //   fpc: illegal expression

  // const c5 : boolean = c2 < 10;
  //   fpc: illegal expression

  const c6 = c1 + 2;

begin

  writeln( c1 );
  writeln( c3 );  // fpc: TRUE
  writeln( c6 );

end.
