{  Two-player PENTE       Object: get 5-in-a-row
                          Surrounding 2 stones removes them.           }

PROGRAM pente;


{$I GRAPH.P }


CONST other: ARRAY [0..2] OF integer  = (1,2,1);
      dx: ARRAY [0..7] of integer  = (1,1,0,-1,-1,-1,0,1);  { 8 directions }
      dy: ARRAY [0..7] of integer  = (0,-1,-1,-1,0,1,1,1);

      width   = 19;        { Board width }
      width1  = 20;        { Board width + 1 (empty margin }

      x_start = -11;        { Edge of (0,0) on screen }
      y_start = -11;
      x_size  = 11;        { Screen size of squares }
      y_size  = 11;


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
     GotoXY(37,5);
     write(player:1)
END;


PROCEDURE beep;
BEGIN
     Sound(440);
     Delay(250);
     NoSound
END;


PROCEDURE draw_board;
VAR  x,y,i: integer;
     x_end, y_end: integer;
BEGIN
     GotoXY(30,1);
     writeln('Pente');
     GotoXY(30,5);
     writeln('Player 1');


     x_end := x_start + width * x_size;
     y_end := y_start + width * y_size;

     x := x_start + x_size;
     FOR i := 1 TO width DO
        BEGIN
           Draw(x,y_start+y_size,x,y_end,2);
           x := x + x_size;
        END;
     y := y_start + y_size;
     FOR i := 1 TO width DO
        BEGIN
           Draw(x_start+x_size,y,x_end,y,2);
           y := y + y_size;
        END;
END;



PROCEDURE cursor (cx,cy: integer);
BEGIN
     Draw (cx-x_size,cy,cx+x_size,cy,-1);    { Use colour table 3,1,2,0 }
     Draw (cx,cy-y_size,cx,cy+y_size,-1)
END;


PROCEDURE get_move (player: integer;  VAR x,y: integer);
VAR  cx,cy: integer;
     done: boolean;
     c: char;
BEGIN
     ColorTable(2,1,0,3);      { For transparent cursor movement }

     done := false;

     REPEAT

        cx := x_start + x_size * x;
        cy := y_start + y_size * y;

        cursor (cx,cy);        { Darken cursor }

        REPEAT
        UNTIL KeyPressed;

        read(kbd,c);

        CASE c OF
           #27: IF KeyPressed THEN
                   BEGIN
                      read(kbd,c);
                      CASE c OF
                        #72: IF y > 1 THEN      y := y - 1;
                        #80: IF y < width THEN  y := y + 1;
                        #75: IF x > 1 THEN      x := x - 1;
                        #77: IF x < width THEN  x := x + 1;
                        #71: BEGIN  x := 10;  y := 10  END;
                        #79: BEGIN  abort := true;  done := true  END;
                      END;
                   END;
           #32: IF owner(x,y) = 0 THEN
                   done := true
                ELSE
                   beep;    { Occupied }
        END;
        cursor (cx,cy);
     UNTIL done;
END;


PROCEDURE draw_stone (player,x,y: integer);
VAR  cx,cy: integer;
BEGIN
     cx := x_start + x_size * x;
     cy := y_start + y_size * y;
     Circle(cx,cy,5,1);
     IF player = 1 THEN
        FillShape(cx,cy,1,1)
     ELSE
        BEGIN
           Draw(cx-5,cy,cx+5,cy,0);
           Draw(cx,cy-5,cx,cy+5,0)
        END
END;


PROCEDURE erase_stone (x,y: integer);
VAR  cx,cy: integer;
BEGIN
     cx := x_start + x_size * x;
     cy := y_start + y_size * y;
     Circle(cx,cy,5,0);
     FillShape(cx,cy,0,0);
     IF x > 1 THEN Draw(cx-5,cy,cx,cy,2);
     IF x < width THEN Draw(cx,cy,cx+5,cy,2);
     IF y > 1 THEN Draw(cx,cy-5,cx,cy,2);
     IF y < width THEN Draw(cx,cy,cx,cy+5,2)
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
                 exit
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

     GraphColorMode;
     Palette(3);
     GraphBackground(Black);
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

     sound (433);
     delay (300);
     sound (885);
     delay (170);
     sound (440);
     delay (175);
     sound (880);
     delay (250);
     nosound;

END.
