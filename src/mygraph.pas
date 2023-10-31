//  mygraph - my simple graphics library
//

// BUG: (probably need to fix in runlib)
//      Need to protect against drawing pixels outside the window bounds.
//      It's corrupting memory.

unit mygraph;

interface

// ----------------------------------------------------------------
// Graphics methods in native code (runtime library)
// These are declared as cdecl external.

// Erase the graphics buffer.
procedure clearScreen; cdecl; external 'runlib' name 'runlibClearScreen';

// Show any updates in the graphics buffer to the screen.
// (This is also done under the hood by Delay and read.)
procedure updateScreen; cdecl; external 'runlib' name 'runlibUpdateScreen';

// Set a pixel
procedure setPixel( x, y : integer; color : integer ); cdecl; external 'runlib' name 'runlibSetPixel';
// Get the color of a pixel
function getPixel( x, y : integer ) : integer; cdecl; external 'runlib' name 'runlibGetPixel';
procedure delay( milliseconds : integer ); cdecl; external 'runlib' name 'runlibDelay';

// Wait for a key, and return its "keysym".  keysyms defined by SDL2.
function waitKey : integer; cdecl; external 'runlib' name 'runlibWaitKey';

// Key codes that could be returned by waitKey
const KEY_RIGHT = 1073741903;
const KEY_LEFT = 1073741904;
const KEY_DOWN = 1073741905;
const KEY_UP = 1073741906;
const KEY_ENTER = 13;
const KEY_ESC = 27;
const KEY_SPACE = 32;

function abs( x : integer ) : integer;
procedure Draw( x0, y0, x1, y1 : integer; color : integer );
procedure DrawCircle( xc, yc, r, color : integer );
procedure FloodFill( x, y, fillColor, borderColor : integer );

// Return a color value
function rgb( r, g, b : integer ) : integer;


// ----------------------------------------------------------------

implementation


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

// Visited buffer.
// This could be an array of bit, that I clear on every call.
// I've used an array of boolean, that I clear on every call.
// To speed that up, I'm using an array of integer, and bump the "visited" value on
// every call.  I only need to clear the array at the start, and whenever the visited value
// wraps around.
//
// TO DO: I'm not accepting const expr in the typedef
// var floodFillVisited: array[ 0..FloodFillScreenY-1, 0..FloodFillScreenX-1 ] of boolean;
var floodFillVisited: array[ 0..239, 0..319 ] of integer;
var floodFillVisitedCode : integer = -1;

procedure FloodFillLow( x, y, fillColor, borderColor : integer );
  begin
    if ( floodFillVisited[y, x] <> floodFillVisitedCode ) and
       ( getPixel( x, y ) <> borderColor ) then
      begin
        setPixel( x, y, fillColor );
        floodFillVisited[y, x] := floodFillVisitedCode;
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
    // periodically clear visisted array.  should have a memset.
    floodFillVisitedCode := floodFillVisitedCode + 1;
    if floodFillVisitedCode = 0 then
      begin
        for j := 0 to FloodFillScreenY-1 do
          for i := 0 to FloodFillScreenX-1 do
            floodFillVisited[j, i] := 0;
        floodFillVisitedCode := 1;
      end;

    FloodFillLow( x, y, fillColor, borderColor );
  end;


// Return a color value
function rgb( r, g, b : integer ) : integer;
  begin
    rgb := r*256*256 + g*256 + b;
  end;

END.
