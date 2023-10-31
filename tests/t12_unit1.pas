
// trying units


unit t12_unit1;

interface

  var x1 : integer = 11;
      x2 : integer = 22;

  var u1x1 : integer = 9800;

  procedure unit1_proc1( p1, p2 : integer );

implementation

  procedure unit1_proc1( p1, p2 : integer );
    var p1x1 : integer = 10;
        p1x2 : integer = 20;
  begin
    writeln( 'Hello from unit1_proc1.  p1x1=', p1x1, ' x1=', x1 );
  end;

initialization
  begin
    writeln( 'Hello from t12_unit1.  x1=', x1 );
  end;

finalization
  begin
    writeln( 'Goodbye from t12_unit1.  x1=', x1 );
  end;

end.
