// float functions

program t27;
// uses testing;

  procedure proc1( f1: double );
    begin
      writeln( f1, ' round=', round(f1), ' trunc=', trunc(f1) );
    end;

  procedure arctanProc( x: double );
    begin
      writeln( x, ' arctan=', arctan(x) );
    end;

  // TO DO: compiler bug:
  //        "radians" is an undefined symbol.  We go into error recovery,
  //        and ProcHeaderDecl fails to create a paramScope.
  //        Later, caller tries to query an attribute from paramScope
  //        and crashes.
  //        I need to refresh my memory of how error recovery works.
  //        I sort of think it skips all semantic mechanism calls until
  //        parsing syncs up at the next ';' token.  Maybe I want something
  //        different.
  //        
  // procedure sinCosProc( r: radians );
  //
  procedure sinCosProc( r: double );
    begin
      writeln( r, ' sin=', sin(r), ' cos=', cos(r) );
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

  arctanProc( 1.0 );
  arctanProc( 2.0 );
  arctanProc( 3.0 );

  sinCosProc( 0.0 );
  sinCosProc( PI * 0.5 );
  sinCosProc( PI );
  sinCosProc( PI * 1.5 );

end.
