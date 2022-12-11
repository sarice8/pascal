// floating point types

program t26; 
  var f1: real;
  var f2: single;
  var f3: double;

begin

{
  writeln( sizeof( f1 ), ' ', sizeof( f2 ), ' ', sizeof( f3 ) );

  // I'm not able to figure out how fpc decides which float type to use.
  //
  writeln( sizeof( 1.0 ), ' ', sizeof( 1.1 ), ' ', sizeof( 0.123 ) );
  writeln( sizeof( 1.0 ), ' ', sizeof( 1.1 ), ' ', sizeof( 0.123 ) );
  writeln( sizeof( 1.2E6 ), ' ', sizeof( 0.2345E6 ) );
  writeln( sizeof( 1.2E6 ), ' ', sizeof( 0.2345E6 ), ' ', sizeof( 1.123E0 ) );

  writeln( sizeof( 1.0 + 1.0 ) );
  writeln( sizeof( 1.1 + 1.1 ) );
  writeln( sizeof( 3.1415926535897932 ) );
  writeln( sizeof( 7.5 ) );    // 4
  writeln( sizeof( 0.75 ) );    // 4   (note:  .75 is a syntax error)
  writeln( sizeof( 7.5E10 ), ' ', sizeof( 7.5E38 ) );   // 10 10
  writeln( sizeof( 7.5E8 ), ' ', sizeof( 7.5E9 ) );   // 4 10
  writeln( sizeof( 7.6E8 ), ' ', sizeof( 7.6E9 ) );   // 4 4
}

  writeln( 1.235e-2 );

  f3 := 1.234e-10;
  writeln( f3 );

{
  f3 := 1.234e+10;
  writeln( f3 );

  f3 := 1.234e10;
  writeln( f3 );
}


end.
