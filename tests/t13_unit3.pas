// testing priority of unit scopes, in uses clause

unit t13_unit3;

interface

  var x : integer = 40;

  function getX : integer;

implementation

  function getX : integer;
    begin
      writeln( 'Hello from unit3.getX.  x=', x );
      getX := x;
    end;

initialization
  begin
    writeln( 'Hello from unit3.  x=', x );
  end;

end.