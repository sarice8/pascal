//  TEST9.PAS
//
// Forward declaration, recursion


program test (input, output);


// ----------------------------------------------------------------
// Graphics methods in native code (runtime library)
// These are declared as cdecl external.

// Erase the graphics buffer.
procedure clearScreen; cdecl; external;

// Show any updates in the graphics buffer to the screen.
// (This is also done under the hood by Delay and read.)
procedure updateScreen; cdecl; external;

// Set a pixel
procedure setPixel( x, y : integer; color : integer ); cdecl; external 'runlib' name 'runlibSetPixel';
// Get the color of a pixel
function getPixel( x, y : integer ) : integer; cdecl; external;
procedure delay( milliseconds : integer ); cdecl; external;


// ----------------------------------------------------------------


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


// Draw a line. Bresenham's algorithm, only needs integer math.
// https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
//
// The basic description of the algorithm is for a line going down to the right,
// with slope < 1 (x difference is greater than y difference).
// For such a line there's only one pixel per x coordinate.
// The wikipedia page it shows the full solution for all lines,
// using two helper methods DrawLineLow and DrawLineHigh.
//
//
procedure DrawLineLow( x0, y0, x1, y1 : integer; color : integer );
    var dx, dy, yi, D, x, y : integer;
  begin
    dx := x1 - x0;
    dy := y1 - y0;
    yi := 1;
    if dy < 0 then
      begin
        yi := -1;
        dy := -dy;
      end;
    D := 2*dy - dx;
    y := y0;

    for x := x0 to x1 do
      begin
        setPixel( x, y, color );
        if D > 0 then
          begin
            y := y + yi;
            D := D + 2*( dy - dx );
          end
        else
          begin
            D := D + 2*dy;
          end
      end
  end;


procedure DrawLineHigh( x0, y0, x1, y1 : integer; color : integer );
    var dx, dy, xi, D, x, y : integer;
  begin
    dx := x1 - x0;
    dy := y1 - y0;
    xi := 1;
    if dx < 0 then
      begin
        xi := -1;
        dx := -dx;
      end;
    D := 2*dx - dy;
    x := x0;

    for y := y0 to y1 do
      begin
        setPixel( x, y, color );
        if D > 0 then
          begin
            x := x + xi;
            D := D + 2*( dx - dy );
          end
        else
          begin
            D := D + 2*dx;
          end
      end
  end;

function abs( x : integer ) : integer;
  begin
    if x >= 0 then
      abs := x
    else
      abs := -x;
  end;

// public entry point
//
procedure Draw( x0, y0, x1, y1 : integer; color : integer );
  begin
    if abs( y1 - y0 ) < abs( x1 - x0 ) then
      if x0 > x1 then
        DrawLineLow( x1, y1, x0, y0, color )
      else
        DrawLineLow( x0, y0, x1, y1, color )
    else
      if y0 > y1 then
        DrawLineHigh( x1, y1, x0, y0, color )
      else
        DrawLineHigh( x0, y0, x1, y1, color );
  end;



// Draw Circle - Bresenham's algorithm
// https://www.geeksforgeeks.org/bresenhams-circle-drawing-algorithm

procedure DrawCircleLow( xc, yc, x, y, color : integer );
  begin
    setPixel( xc + x, yc + y, color );
    setPixel( xc - x, yc + y, color );
    setPixel( xc + x, yc - y, color );
    setPixel( xc - x, yc - y, color );
    setPixel( xc + y, yc + x, color );
    setPixel( xc - y, yc + x, color );
    setPixel( xc + y, yc - x, color );
    setPixel( xc - y, yc - x, color );
  end;

procedure DrawCircle( xc, yc, r, color : integer );
    var x, y, d : integer;
  begin
    x := 0;
    y := r;
    d := 3 - 2*r;
    DrawCircleLow( xc, yc, x, y, color );
    while y >= x do
      begin
        // for each pixel we draw all eight pixels
        x := x + 1;
        if d > 0 then
          begin
            y := y - 1;
            d := d + 4*(x - y) + 10;
          end
        else 
          begin
            d := d + 4*x + 6;
          end;
        DrawCircleLow( xc, yc, x, y, color );
      end;
  end;


// Flood fill
// https://en.wikipedia.org/wiki/Flood_fill
//
// Using 4-way (rather than 8-way) so it doesn't leak through diagonal lines.
// Starting with simple recursive solution.
// Using a visited array, so we can one day support pattern fill too.
// The visited array needs a bit per screen pixel, so declaring as a global array
// in case it's too big for the stack.
const FloodFillScreenX = 320;
const FloodFillScreenY = 240;

// TO DO: I'm not accepting const expr in the typedef
// var floodFillVisited: array[ 0..FloodFillScreenY-1, 0..FloodFillScreenX-1 ] of boolean;
var floodFillVisited: array[ 0..239, 0..319 ] of boolean;

procedure FloodFillLow( x, y, fillColor, borderColor : integer );
  begin
    if ( not floodFillVisited[y, x] ) and ( getPixel( x, y ) <> borderColor ) then
      begin
        setPixel( x, y, fillColor );
        floodFillVisited[y, x] := true;
        if y + 1 < FloodFillScreenY then
          FloodFillLow( x, y + 1, fillColor, borderColor );
        if y - 1 >= 0 then
          FloodFillLow( x, y - 1, fillColor, borderColor );
        if x - 1 >= 0 then
          FloodFillLow( x - 1, y, fillColor, borderColor );
        if x + 1 < FloodFillScreenX then
          FloodFillLow( x + 1, y, fillColor, borderColor );
      end;    
  end;

procedure FloodFill( x, y, fillColor, borderColor : integer );
    var i, j : integer;
  begin
    // clear visisted array.  should have a memset.
    for j := 0 to FloodFillScreenY-1 do
      for i := 0 to FloodFillScreenX-1 do
        floodFillVisited[j, i] := false;

    FloodFillLow( x, y, fillColor, borderColor );
  end;


// --------------------

// Return a color value
function rgb( r, g, b : integer ) : integer;
  begin
    rgb := r*256*256 + g*256 + b;
  end;


var r1 : rTwoInts;
var x : integer;
var color : integer;
var color2 : integer;
var r : integer;
var c : integer;

var fstep, fx, fy, loopCount : integer;

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

  for loopCount := 1 to 3 do
    begin
      for fstep := 50 to 250 do
        begin
          fx := fstep * 1;
          fy := 50;
          clearScreen;
          DrawFace( fx, fy, color, color2 );    
          updateScreen;
          Delay( 4 );
        end;
      for fstep := 250 downto 50 do
        begin
          fx := fstep * 1;
          fy := 50;
          clearScreen;
          DrawFace( fx, fy, color, color2 );    
          updateScreen;
          Delay( 4 );
        end;
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
