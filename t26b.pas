// floating point types

program t26b;
  var f1: double;
  var f2: double;

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

end.
