//  TEST9.PAS
//
// Forward declaration, recursion


program test (input, output);


// this procedure is declared as an external cdecl procedure.
// Not fully implemented, can't call yet.
procedure setPixel( x, y : integer; color : integer ); cdecl; external;

// this procedure is never defined.  Should give an error.
procedure neverDefined( x: integer );
  forward;


// forward declaration
//
procedure proc2( i1, i2 : integer; i3 : integer );
  forward;


procedure proc1( p1, p2 : integer; p3 : integer; rep : integer );
    var total : integer;
  begin
    if rep > 0 then
      // recursion
      proc1( p1, p2, p3, rep-1 );

    writeln( 'Hello from proc1, rep ', rep );
    proc2( p1, p2, p3 )
  end;


// should insist this matches the forward declaration,
// except that the parameter names may differ.

procedure proc2( p1, p2 : integer; p3 : integer );
    var total : integer;
  begin
    writeln( 'Hello from proc2' );

    total := p1 + p2 + p3;
    writeln( '  p1 = ', p1 );
    writeln( '  p2 = ', p2 );
    writeln( '  p3 = ', p3 );
    writeln( '  total = ', total );
  end;


BEGIN

  writeln( 'Proc test:' );
  proc1( 100, 200, 300, 3 );
  proc1( 400, 500, 600, 2 );

  writeln( 'Good bye!' );

END.
