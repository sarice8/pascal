// testing priority of scopes in unit implementation.

unit t13_unit2;

interface

  var x : integer = 30;

  function getX : integer;

implementation
  function getX : integer;
    begin
      writeln( 'Hello from unit2.getX.  x=', x );
      getX := x;
    end;

initialization
  begin
    writeln( 'Hello from unit2.  x=', x );
  end;

end.
