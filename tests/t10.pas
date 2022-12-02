
// trying var init



program test (input, output);
uses testing;

  // can't have an initial value if more than one var (according to fpc)
  // var x1, x2 : integer = 1;
  var x1 : integer = 1;
      x2 : integer = 2;
  var a : array[1..3] of integer;
begin
  
  a[2] := 5;

  writeln( 'x1=', x1, ' x2=', x2 );
end.
