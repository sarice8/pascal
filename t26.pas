// floating point types

program t26; 
  var f1: real;
  var f2: single;
  var f3: double;

  const fc1 = 1.234;
  const fc2 = 1.234 + 1.0;
  const fc3 = double(1);

  // const ic1 = shortint( 1.1 );
  //  fpc:  Illegal type conversion: "Extended" to "ShortInt"
  //    (docs say should use explicit call to trunc() or round())

  var s1: shortstring;

begin

{
  writeln( sizeof( f1 ), ' ', sizeof( f2 ), ' ', sizeof( f3 ) );

  // I'm not able to figure out how fpc decides which float type to use.
  // I think in most cases it's going to the 10-byte extended float (80-bit)
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

  // Some literal tests
  writeln( 1.235e-2 );
  writeln( 1.234e+10 );
  writeln( 1.234e10 );
  writeln( 1.2345 );

  f3 := 1.234e-10;
  writeln( f3 );

  f3 := f3 + 1.1e-10;
  writeln( f3 );

  writeln( f3 - 1.1e-10 );
  writeln( f3 * 1.1e-10 );
  writeln( f3 / 1.1e-10 );
  writeln( -1.1e-10 );
  writeln( -f3 );

  // mixing integer/byte and double, on either side of operator.
  // Double on left, integer on right:
  writeln( f3 + 1 );
  writeln( f3 - 1 );
  writeln( f3 * 2 );
  writeln( f3 / 2 );

  // integer on left, double on right:
  writeln( 1 + f3 );
  writeln( 1 - f3 );
  writeln( 2 * f3 );
  writeln( 2 / f3 );

  // relational operators, with double on both sides
  f3 := 1.5;
  writeln( f3 = 1.5 );
  writeln( f3 <> 1.5 );
  writeln( f3 > 1.4 );
  writeln( f3 > 2.0 );
  writeln( f3 >= 1.5 );
  writeln( f3 <= 1.5 );
  writeln( f3 < 2.0 );


  // relational operators, with mix of integer/byte and double, on either side.
  writeln( f3 = 1 );
  writeln( f3 <> 1 );
  writeln( f3 > 1 );
  writeln( f3 > 2 );
  writeln( f3 >= 1 );
  writeln( f3 <= 1 );
  writeln( f3 < 2 );
  
  writeln( 1 = f3 );
  writeln( 1 <> f3 );
  writeln( 1 < f3 );
  writeln( 2 < f3 );
  writeln( 1 <= f3 );
  writeln( 1 >= f3 );
  writeln( 2 > f3 );

  // Constant
  writeln( 'constant: ', fc1 );
  writeln( 'constant: ', fc2 );
  writeln( 'constant: ', fc3 );

  // While I'm at it, a couple string vs strlit comparisons
  s1 := 'Alphabet';
  writeln( s1 > 'Alberta' );
  writeln( s1 > 'Alphabetize' );
  writeln( 'Zebra' > s1 );

end.
