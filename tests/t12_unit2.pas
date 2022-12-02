
// trying units


unit t12_unit2;

interface

  var u2x1 : integer = 7500;


implementation

initialization
  begin
    writeln( 'Hello from t12_unit2' );
  end;

finalization
  begin
    writeln( 'Goodbye from t12_unit2' );
  end;

end.
