// floating point types

program t26b;
  var f1: double;
  var f2: double;

  var i1: integer;
begin

  f1 := 1.5;
  f2 := 1.6;

  writeln( f1 + f2 );
  writeln( 1.9 + 0.2 );
  writeln( f1 + 0.1 );

  writeln( f1 - 0.1 );
  writeln( f1 - f2 );
  writeln( 1.9 - 0.2 );
  writeln( -2.0 );
  writeln( -f1 );

  writeln( f1 * f2 );
  writeln( f1 * 2.0 );

  writeln( f1 / f2 );
  writeln( f1 / 2.0 );
  writeln( 4.0 / 2.0 );

  i1 := 3;
  writeln( 2.2 * i1 );

  // comparisons
  writeln( 'static: ', 2.2 >= 2.2 );
  writeln( f1 > f2 );
  writeln( f1 < f2 );
  writeln( f1 > 1.4 );
  writeln( 1.4 < f1 );

  // = and <> need work.
  // Currently uses int64 comparison.
  // I'm obligated to use floating point comparison for these,
  // first to (eventually) allow for NAN,
  // but more importantly so I can use instructions that have access to xmm registers.
  // This implies that I cannot use memcmp for structured type comparision,
  // at least if there are any floating point values in or under the structured type.
  // (Ugh, that's a hassle for arrays, would have to generate a looping comparison.
  // Take a peek at what c compilers do for something like that.)
  // This might be a first step towards support for managed types, so worth tackling.
  //
  // writeln( f1 = 1.5 );
  // writeln( f1 + 1.0 = 2.5 );

end.
