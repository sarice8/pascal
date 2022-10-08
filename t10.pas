
// looking for a bug in jit, doing subtraction of params

program test (input, output);

procedure proc( x0, y0, x1, y1, color : integer );
  var dx : integer;
  begin
    dx := x1 - x0;
    writeln( 'Inside proc: x0 = ', x0, ' x1 = ', x1, ' dx = ', dx );
  end;

begin
  
  proc( 20, 20, 80, 20, 1 );
end.
