
// trying units


program test (input, output);
  //uses t12_unit1, t12_unit2;
  // scanner include mechanism needs work, resumes at next line I think.  see s_hit_eof().
 uses t12_unit1
 {  , t12_unit2 }
 ;


  var x1 : integer = 1;
      x2 : integer = 2;

  procedure proc1( p1, p2 : integer );
    var p1x1 : integer = 10;
        p1x2 : integer = 20;
  begin
    writeln( 'Hello from proc1.  p1x1=', p1x1, ' x1=', x1 );
  end;


begin

  writeln( 'Hello from main.  x1=', x1 );
  proc1( 5, 6 );
  writeln( 'u1x1=', u1x1 );
  {
  unit1_proc1( 50, 60 );
  x1 := 15;
  unit1_proc1( 51, 61 );
  unit1.x1 := 16;
  unit1_proc1( 52, 62 );

  writeln( x1 );
  writeln( unit1.x1 );
  }

end.
