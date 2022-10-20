//  TEST9.PAS
//
// Forward declaration, recursion


program test (input, output);
uses mygraph
 // TO DO: bug: have to put the ; on the next line after uses <unit>
 // due to scanner include behavior:
;

// this procedure is never defined.  Should give an error, at least if we call it.
procedure neverDefined( x: integer );
  forward;


// cdecl method with complex parameter type is not supported yet.
// On linux this record should be packed in a register.
type rTwoInts = record
    i1, i2: integer;   // ASIDE: am I right to require a ; before the end here ??
  end;
procedure toughCdecl1( x: rTwoInts ); cdecl; external;

// cdecl method with more complex parameter type is not supported yet.
// On linux, I think(?) this goes on the stack.
type rThreeInts = record
    i1, i2, i3: integer;
  end;
procedure toughCdecl2( x: rTwoInts ); cdecl; external;


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

var r1 : rTwoInts;
    x : integer;
    color : integer;
    color2 : integer;
    r : integer;
    c : integer;
    sc : integer;

var fstep, fx, fy, dx, dy, loopCount : integer;

// TO DO: support initialization
// var delayMs : integer = 3;
var delayMs : integer;

procedure DrawFace( fx, fy, color, color2 : integer );
  begin
    DrawCircle( fx, fy, 30, color );
    FloodFill( fx, fy, color2, color );

    DrawCircle( fx-15, fy-10, 7, color );
    FloodFill( fx-15, fy-10, 0, color );
    DrawCircle( fx+15, fy-10, 7, color );
    FloodFill( fx+15, fy-10, 0, color );

    Draw( fx-15, fy+10, fx+15, fy+10, color );
    Draw( fx-15, fy+10, fx-10, fy+15, color );
    Draw( fx-10, fy+15, fx+10, fy+15, color );
    Draw( fx+10, fy+15, fx+15, fy+10, color );
    FloodFill( fx, fy+11, 0, color );
  end;

BEGIN

  writeln( 'Proc test:' );
  proc1( 100, 200, 300, 3 );
  proc1( 400, 500, 600, 2 );

  // TO DO: accept hex numbers   $ffffff
  // color := rgb( 255, 0, 255 );
  color := rgb( 50, 128, 255 );
  color2 := rgb( 255, 255, 128 );

  // extern method test
  {
  for x := 10 to 20 do
    begin
      setPixel( x * 2, 15, color );
    end;
  }

  // Draw( 20, 20, 80, 20, color );
  // Draw( 20, 20, 40, 30, color );

  writeln( 'Hit a key to begin ...' );
  sc := waitKey;
  writeln( '  sc = ', sc );

  fx := 50;
  fy := 50;
  dx := 2;
  dy := 2;
  delayMs := 3;

  for loopCount := 1 to 2000 do
    begin
      fx := fx + dx;
      if ( fx < 50 ) or ( fx > 250 ) then
        dx := dx * -1;
      fy := fy + dy;
      if ( fy < 50 ) or ( fy > 150 ) then
        dy := dy * -1;

      clearScreen;
      DrawFace( fx, fy, color, color2 );
      Delay( delayMs );
    end;


  c := GetPixel( 19, 20 );
  writeln( 'GetPixel(19,20) = ', c );
  c := GetPixel( 20, 20 );
  writeln( 'GetPixel(20,20) = ', c );
  
  // unsupported cdecl test
  r1.i1 := 1;
  r1.i2 := 2;
  toughCdecl1( r1 );


  writeln( 'Good bye!' );

END.
