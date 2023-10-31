unit testing;

interface

implementation

initialization
  begin
    writeln( 'TEST: Starting test' );
  end;

finalization
  begin
    // Note: fpc doesn't show this if log is redirected.
    writeln( 'TEST: Finished test' );
  end;

end.
