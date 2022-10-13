{ cut down excerpts from pente.pas }


{  Two-player PENTE       Object: get 5-in-a-row
                          Surrounding 2 stones removes them.           }

PROGRAM pente;


{$I GRAPH.P }

// --------------------------------------------------------------------------------
// For now, copying from t9.pas
// --------------------------------------------------------------------------------

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

// Wait for a key, and return its scan code.  Scan codes defined by SDL2.
// TO DO: should return both ascii char and scan code.
function waitKey : integer; cdecl; external 'runlib' name 'runlibWaitKey';

// Key codes that could be returned by waitKey
const KEY_RIGHT = 1073741903;
const KEY_LEFT = 1073741904;
const KEY_DOWN = 1073741905;
const KEY_UP = 1073741906;
const KEY_ENTER = 13;
const KEY_ESC = 27;
const KEY_SPACE = 32;


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

procedure FillShape( x, y, fillColor, borderColor : integer);
  FloodFill( x, y, fillColor, borderColor );


// --------------------

// Return a color value
function rgb( r, g, b : integer ) : integer;
  begin
    rgb := r*256*256 + g*256 + b;
  end;


// --------------------------------------------------------------------------------

// Using these color values, rather than hardcoded 1 2 3
// Not sure what the original colors were.  Look it up.
// var color0 : integer = rgb( 0, 0, 0 );
// var color1 : integer = rgb( 255, 255, 0 );
// var color2 : integer = rgb( 0, 255, 255 );
// var color3 : integer = rgb( 255, 128, 128 );
//// I don't really have a color map mode, so this needs work
// var colorReverse : integer = rgb( 255, 0, 0 );

// I seem to have a bug when calling a function from initialization.
// Do init in main instead.
var color0 : integer;
var color1 : integer;
var color2 : integer;
var color3 : integer;
var colorReverse : integer;



CONST // other: ARRAY [0..2] OF integer  = (1,2,1);
      // dx: ARRAY [0..7] of integer  = (1,1,0,-1,-1,-1,0,1);  { 8 directions }
      // dy: ARRAY [0..7] of integer  = (0,-1,-1,-1,0,1,1,1);

      width   = 19;        { Board width }
      width1  = 20;        { Board width + 1 (empty margin }

      x_start = -11;        { Edge of (0,0) on screen }
      y_start = -11;
      x_size  = 11;        { Screen size of squares }
      y_size  = 11;

// TO DO: was const
VAR other: ARRAY [0..2] of integer;  // = (1,2,1);
    dx: ARRAY [0..7] of integer;  // = (1,1,0,-1,-1,-1,0,1);  { 8 directions }
    dy: ARRAY [0..7] of integer;  // = (0,-1,-1,-1,0,1,1,1);

TYPE board_type = ARRAY [0..width1,0..width1] OF integer;

VAR  player, winner: integer;
     board: board_type;
     x,y:   integer;
     move:  integer;     { Count of moves made }
     abort: boolean;
     score: ARRAY [1..2] OF integer;


{ --------------------------- Managing the board ----------------------- }

PROCEDURE init_board;
VAR x,y: integer;
BEGIN
     FOR x := 0 TO width+1 DO
        FOR y := 0 TO width+1 DO
           board[x,y] := 0
END;


FUNCTION owner (x,y: integer): integer;       { Returns player at position }
BEGIN                                 { Depends on implementation of board }
     owner := board[x,y]
END;



{ -------------------------- User Interface ---------------------------- }

PROCEDURE Attention (player: integer);
BEGIN
     // GotoXY(37,5);
     // write(player:1)
END;


PROCEDURE beep;
BEGIN
     // Sound(440);
     Delay(250);
     // NoSound
END;


PROCEDURE draw_board;
VAR  x,y,i: integer;
     x_end, y_end: integer;
BEGIN
     // GotoXY(30,1);
     writeln('Pente');
     // GotoXY(30,5);
     writeln('Player 1');


     x_end := x_start + width * x_size;
     y_end := y_start + width * y_size;

     x := x_start + x_size;
     FOR i := 1 TO width DO
        BEGIN
           Draw(x,y_start+y_size,x,y_end, color2);
           x := x + x_size;
        END;
     y := y_start + y_size;
     FOR i := 1 TO width DO
        BEGIN
           Draw(x_start+x_size,y,x_end,y, color2);
           y := y + y_size;
        END;
END;



PROCEDURE cursor (cx,cy: integer);
BEGIN
     Draw(cx-x_size,cy,cx+x_size,cy, colorReverse);    { Use colour table 3,1,2,0 }
     Draw(cx,cy-y_size,cx,cy+y_size, colorReverse);
END;


PROCEDURE get_move (player: integer;  VAR x,y: integer);
VAR  cx,cy: integer;
     done: boolean;
     c: char;
     key: integer;
BEGIN
     // ColorTable(2,1,0,3);      { For transparent cursor movement }

     done := false;

     REPEAT

        cx := x_start + x_size * x;
        cy := y_start + y_size * y;

        cursor (cx,cy);        { Darken cursor }

     
//        REPEAT
//        UNTIL KeyPressed;
//
//        read(kbd,c);
//
//        CASE c OF
//           #27: IF KeyPressed THEN
//                   BEGIN
//                      read(kbd,c);
//                      CASE c OF
//                        #72: IF y > 1 THEN      y := y - 1;
//                        #80: IF y < width THEN  y := y + 1;
//                        #75: IF x > 1 THEN      x := x - 1;
//                        #77: IF x < width THEN  x := x + 1;
//                        #71: BEGIN  x := 10;  y := 10  END;
//                        #79: BEGIN  abort := true;  done := true  END;
//                      END;
//                   END;
//           #32: IF owner(x,y) = 0 THEN
//                   done := true
//                ELSE
//                   beep;    { Occupied }
//        END;

        // New input logic, with my keyboard api and no case statement

        key := WaitKey;
        writeln( 'get_move sees key ', key );

        if key = KEY_SPACE then
            begin
                if owner( x, y ) = 0 then
                    done := true
                else
                    beep;    { Occupied }
            end
        else if key = KEY_ESC then
            begin
                abort := true;
                done := true
            end
        else if key = KEY_UP then
            begin
                IF y > 1 THEN      y := y - 1;
            end
        else if key = KEY_DOWN then
            begin
                IF y < width THEN  y := y + 1;
            end
        else if key = KEY_LEFT then
            begin
                IF x > 1 THEN      x := x - 1;
            end
        else if key = KEY_RIGHT then
            begin
                IF x < width THEN  x := x + 1;
            end;

        cursor (cx,cy);
     UNTIL done;
END;


PROCEDURE draw_stone (player,x,y: integer);
VAR  cx,cy: integer;
BEGIN
     cx := x_start + x_size * x;
     cy := y_start + y_size * y;
     DrawCircle(cx,cy,5,color1);
     IF player = 1 THEN
        FillShape(cx,cy,color1,color1)
     ELSE
        BEGIN
           Draw(cx-5,cy,cx+5,cy, color0);
           Draw(cx,cy-5,cx,cy+5, color0)
        END
END;


PROCEDURE erase_stone (x,y: integer);
VAR  cx,cy: integer;
BEGIN
     cx := x_start + x_size * x;
     cy := y_start + y_size * y;
     DrawCircle(cx,cy,5,0);
     FillShape(cx,cy,0,0);
     IF x > 1 THEN Draw(cx-5,cy,cx,cy,color2);
     IF x < width THEN Draw(cx,cy,cx+5,cy,color2);
     IF y > 1 THEN Draw(cx,cy-5,cx,cy,color2);
     IF y < width THEN Draw(cx,cy,cx,cy+5,color2)
END;


{ ---------------------------- Logical Guts ---------------------------- }

{ Count longest chain of stones starting at x,y, in direction d.
  Return o = owner, l = length, t = termination (0, or other player #) }

PROCEDURE find_chain (x,y,d: integer;  VAR o,l,t: integer);
BEGIN
     l := 0;
     o := board[x,y];
     IF o <> 0 THEN
        REPEAT
           x := x + dx[d];
           y := y + dy[d];
           l := l + 1;
        UNTIL board[x,y] <> o;
     t := board[x,y];
END;



PROCEDURE make_move(player,x,y: integer);
VAR  d,o,l,t: integer;
     a,b: integer;
BEGIN
     board[x,y] := player;
     draw_stone (player,x,y);
     FOR d := 0 TO 7 DO
        BEGIN
           a := x+dx[d];
           b := y+dy[d];
           find_chain (a,b,d,o,l,t);  { Owner Length Termination }
           IF (l = 2) AND (t = player) THEN  { Implies Owner = other[player] }
              BEGIN
                 board[a,b] := 0;
                 erase_stone(a,b);
                 board[a+dx[d],b+dy[d]] := 0;
                 erase_stone(a+dx[d],b+dy[d])
              END;
        END;
END;



FUNCTION winning(x,y: integer): boolean;   { Has x,y move caused win? }
VAR  d,o,l,l2,t: integer;
BEGIN
     winning := false;
     FOR d := 0 TO 3 DO
        BEGIN
           find_chain (x,y,d,o,l,t);
           find_chain (x,y,d+4,o,l2,t);
           IF l + l2 - 1 >= 5 THEN
              BEGIN
                 winning := true;
                 break
              END
        END
END;



{ ------------------------------ Testing ------------------------------- }


PROCEDURE fill_board;
VAR x,y: integer;
BEGIN
     FOR x := 1 TO width DO
        FOR y := 1 TO width DO
           IF owner(x,y) <> 0 THEN
              draw_stone (1,x,y)
END;


{ ------------------------- Controlling Program ------------------------ }


BEGIN   { Main }

    // ----------------------------------------------------
    // Moved these from const to var
    // ----------------------------------------------------

    // other := (1,2,1);
    other[0] := 1;
    other[1] := 2;
    other[2] := 1;

    // dx := (1,1,0,-1,-1,-1,0,1);  { 8 directions }
    dx[0] := 1;
    dx[1] := 1;
    dx[2] := 0;
    dx[3] := -1;
    dx[4] := -1;
    dx[5] := -1;
    dx[6] := 0;
    dx[7] := 1;

    // dy := (0,-1,-1,-1,0,1,1,1);
    dy[0] := 0;
    dy[1] := -1;
    dy[2] := -1;
    dy[3] := -1;
    dy[4] := 0;
    dy[5] := 1;
    dy[6] := 1;
    dy[7] := 1;

    color0 := rgb( 0, 0, 0 );
    color1 := rgb( 255, 255, 0 );
    color2 := rgb( 0, 255, 255 );
    color3 := rgb( 255, 128, 128 );
    colorReverse := rgb( 255, 0, 0 );

    // ----------------------------------------------------

     // GraphColorMode;
     // Palette(3);
     // GraphBackground(Black);
     draw_board;

     score[1] := 0;
     score[2] := 0;

     player := 0;
     winner := 0;
     move   := 0;

     abort  := false;
     init_board;


     x := 10;
     y := 10;


     REPEAT

        player := other[player];
        move := move + 1;
        Attention(player);

        get_move(player,x,y);    { Input player's move ..  guarenteed legal }

        IF not abort THEN
           BEGIN
              make_move(player,x,y);   { Update board }

              IF winning(x,y) THEN   { Check for player winning around x,y }
                 winner := player;
           END;

     UNTIL (winner > 0) OR abort;

     // sound (433);
     delay (300);
     // sound (885);
     delay (170);
     // sound (440);
     delay (175);
     // sound (880);
     delay (250);
     // nosound;

END.
