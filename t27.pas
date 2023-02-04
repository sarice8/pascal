// float functions

program t27;
// uses testing;

  procedure proc1( f1: double );
    begin
      // writeln( f1, ' round=', round(f1), ' trunc=', trunc(f1) );
      writeln( f1, ' trunc=', trunc(f1) );
    end;

begin

  proc1( 1.1 );
  proc1( 1.6 );
  proc1( -1.1 );
  proc1( -1.6 );

  proc1( 1.500 );
  proc1( -1.500 );

  proc1( 2.500 );
  proc1( -2.500 );
end.
