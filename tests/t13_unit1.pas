// testing priority of scopes in unit implementation.
// See what fpc says.
// Answer: as I hoped, the interface portion still has priority
// over the things pulled in from the uses clause in the implementation.

unit t13_unit1;

interface

  var x : integer = 20;

  function getX : integer;

implementation
  uses t13_unit2
    ;

  function getX : integer;
    begin
      writeln( 'Hello from unit1.getX.  x=', x );
      getX := x;
    end;

initialization
  begin
    writeln( 'Hello from unit1.  x=', x );
  end;

end.
