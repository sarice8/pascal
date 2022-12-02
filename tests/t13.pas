program t13;
  // uses t13_unit1;
  // uses t13_unit1, t13_unit3;
  uses testing, t13_unit3, t13_unit1;

  // answer: within one uses clause, later unit has priority
  

begin
  writeln( 'Hello from main.  x=', x );
  writeln( 'and getX=', getX );
end.
