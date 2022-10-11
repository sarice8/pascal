
// trying var init



program test (input, output);

  // can't have an initial value if more than one var (according to fpc)
  // var x1, x2 : integer = 1;
  var x1 : integer = 1;
      x2 : integer = 2;
begin
  
  writeln( 'x1=', x1, ' x2=', x2 );
end.
