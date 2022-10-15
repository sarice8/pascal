
// trying static scopes


program test (input, output);

  var x1 : integer = 1;
      x2 : integer = 2;
  
  procedure proc1( p1, p2 : integer );
    var p1x1 : integer = 10;
        p1x2 : integer = 20;

    procedure proc11( p11 : integer );
      var p11x1 : integer = 100;
    begin
      writeln( 'Hello from proc11.  p11x1=', p11x1, ' p1x1=', p1x1, ' x1=', x1 );
    end;

  begin
    writeln( 'Hello from proc1.  p1x1=', p1x1, ' x1=', x1 );
    proc11( p1x1 + 1 );
  end;


begin

  writeln( 'Hello from main.  x1=', x1 );
  proc1( 5, 6 );

end.
