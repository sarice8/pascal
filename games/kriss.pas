
{ Kriss Kross Solving Aid }

{ Steve Rice,  April 1987 }

{$I-}

const maxMenu = 100;
      maxWords = 72;
      strLen  = 15;
      edge     = 45;    { Left edge of menu half of screen }
      rMin     = 6;     { First row of board }
      rMax     = 25;    { Board size }
      cMax     = 40;
      blankMessage = '               ';
      blankWord = '     ';
      cUnused   = #176;
      cEmpty    = #250;

type str = string[strLen];
     errmsg = string[50];

     constraintType = array [1..maxMenu] of boolean;

     menu = record
               txt: array [1..maxMenu] of str;
               dim: constraintType;
               entries: integer;
               row:  integer;
               col:  integer;
               rows: integer;
               colWidth: integer;
            end;

     board = array [1..cMax,1..rMax] of char;

     indexType = array [1..cMax,1..rMax,boolean] of integer;



     dimlistType = array [1..maxWords] of constraintType;
     changedType = array [1..maxWords] of boolean;


var main:         menu;
    game:         board;
    original:     board;
    robmap:       board;
    words:        menu;
    origWords:    menu;

    index:        indexType;    { A number for each board line... index into }
    dimlist:      dimlistType;  {   the dimlist array                        }
    changed:      changedType;
    lastIndex:    integer;      { Last assigned (marked) }

    i:            integer;
    ch:           char;
    code:         integer;

    inverted:     boolean;   { Current drawing colour inverted? }

    command:      integer;
    escape:       boolean;
    numWords:     integer;
    currentWord:  integer;
    quit:         boolean;
    filename:     string[15];

    playx,playy:  integer;
    dirh,dirv:    array [boolean] of integer;
    marked:       boolean;
    markedx,markedy,markedl: integer;
    markedh:      boolean;
    markedi:      integer;

    robot:        boolean;
    showWords:    boolean;
    soundOn:      boolean;
    wordsFound:   integer;


procedure Inverse (dim: boolean);
begin
     TextColor (black);
     if dim then
        TextBackground (Magenta)
     else
        TextBackground (white);
     inverted := true;
end;

procedure Normal (dim: boolean);
begin
     if dim then
        TextColor (DarkGray)
     else
        TextColor (white);
     TextBackground (black);
     inverted := false;
end;


function upper (ch: char): char;
begin
     if ch in ['a'..'z'] then
        upper := chr(ord(ch)-ord('a')+ord('A'))
     else
        upper := ch
end;

function max (a,b: integer): integer;
begin
     if a>b then
        max := a
     else
        max := b
end;

procedure clearWords;
var  w: integer;
begin
     numWords := 0;
     for w := 1 to maxWords do
        begin
           words.txt[w] := blankWord;
           words.dim[w] := true;
        end;
end;

procedure countwords;
var  i: integer;
begin
     numWords := 0;
     for i := 1 to maxWords do
        if words.txt[i] <> blankWord then
           numWords := numWords + 1
end;



procedure clearBoard;
var  r,c: integer;
begin
     lastIndex := 0;
     for r := rMin to rMax do
        for c := 1 to cMax do
           begin
              game[c,r] := cUnused;
              index[c,r,true] := 0;
              index[c,r,false] := 0;
           end
end;

procedure drawBoard (b: board);
var  r,c: integer;
begin
     Normal(false);
     for r := rMin to rMax do
        begin
           GotoXY(1,r);
           for c := 1 to cMax do
              write (b[c,r]);
        end
end;


{ Waits for a char, ch.  If ch is <Esc>, reads a function integer and
  returns it in code. }

procedure getKey (var ch: char;  var code: integer);
var  a: char;
begin
     repeat until keyPressed;
     read (Kbd, ch);
     if (ch = #27) and keyPressed then
        begin
           read (Kbd, a);
           code := ord(a)
        end
     else
        code := 0
end;

{ Menu routines }

procedure menuDisplay (var m: menu);
    {   191 ¿  192 À  179 ³  196 Ä  217 Ù  218 Ú    }
    {   176 °  177 ±  178 ²  219 Û  249 ù  250 ú    }
const ur = 191;
      ll = 192;
      lr = 217;
      ul = 218;
      hh = 196;
      vv = 179;
var  r,c,e,i: integer;
     dim: boolean;
begin
     Normal(false);
     dim := false;
     r := m.row;
     c := m.col;
     e := r + m.rows - 1;
     for i := 1 to m.entries do
        begin
           gotoXY(c,r);
           if m.dim[i] <> dim then
              begin
                 dim := m.dim[i];
                 Normal(dim);
              end;
           write (' ',m.txt[i],' ');
           if r = e then
              begin
                 r := m.row;
                 c := c + m.colWidth
              end
           else
              r := r + 1
        end;
     Normal(false)
end;

procedure menuSelect (var m: menu; var entry: integer; edit: boolean;
                      var escape: boolean);
label home, down;
var  e, i, code: integer;
     ch: char;
     done: boolean;
     var row,col: integer;
     word: str;
     len,p: integer;
     newEntry: integer;
begin
     done := false;
     escape := false;
     newEntry := 0;
     if entry = 0 then entry := 1;
     row := m.row + ((entry - 1) mod m.rows);
     col := m.col + ((entry - 1) div m.rows) * m.colWidth;
     e := m.row + m.rows - 1;
     repeat
        GotoXY(col,row);
        inverse(m.dim[entry]);
        write (' ',m.txt[entry],' ');
        getKey(ch,code);
        GotoXY(col,row);
        if (ch in ['!'..'z']) then
           if edit then
              begin
                 len := length(m.txt[entry]);
                 m.txt[entry] := ch;
                 write (' ', ch, Copy('            ',1,len));
                 GotoXY(col+2,row);
                 readln(word);
                 len := max(len,length(word)+1);
                 p := pos(' ',word);    { Trailing blanks? }
                 if p > 0 then
                    word := copy(word,1,p-1);
                 m.txt[entry] := ch + word;
                 GotoXY(col+length(word)+3,row);
                 normal(m.dim[entry]);
                 write (Copy('                ',1,len - length(word)-1));
                 GotoXY(col,row);
                 ch := #13
              end
           else
              begin
                 i := 1;
                 ch := upper(ch);
                 while (i < m.entries) and
                       (upper(copy(m.txt[i],1,1)) <> ch) do
                    i := i + 1;
                 if upper(copy(m.txt[i],1,1)) = ch then
                    begin
                       newEntry := i;
                       ch := #13
                    end
              end;
        normal(m.dim[entry]);
        write (' ',m.txt[entry],' ');
        if newEntry > 0 then
           entry := newEntry;
        case ch of
        #13:
           if edit then
              goto down
           else
              done := true;
        #27, ' ':
           if code = 0 then     { Esc alone }
              begin
                 escape := true;
                 done := true
              end
           else
              begin
                 case code of
                    71: begin
home:                      row := m.row;         { home }
                           col := m.col;
                           entry := 1
                        end;
                    72: if entry > 1 then          { up }
                           begin
                              entry := entry - 1;
                              row := row - 1;
                              if row < m.row then
                                 begin
                                    row := e;
                                    col := col - m.colWidth
                                 end
                           end
                        else
                           begin
                              entry := m.entries;
                              row := m.row + ((entry - 1) mod m.rows);
                              col := m.col +
                                      ((entry - 1) div m.rows) * m.colWidth;
                           end;
                    75: if col > m.col then       { left }
                           begin
                              entry := entry - m.rows;
                              col := col - m.colWidth
                           end;
                    77: if entry + m.rows <= m.entries then   { right }
                           begin
                              entry := entry + m.rows;
                              col := col + m.colWidth
                           end;
                    80: begin                                  { down }
down:                      entry := (entry mod m.entries) + 1;
                           if entry = 1 then
                              goto home
                           else
                              begin
                                 row := row + 1;
                                 if row > e then
                                    begin
                                       row := m.row;
                                       col := col + m.colWidth
                                    end
                              end
                        end;
                    83: if edit then
                           begin                  { Delete }
                              len := length(m.txt[entry]);
                              m.txt[entry] := blankWord;
                              GotoXY(col+length(blankWord)+2,row);
                              write (Copy('                ',
                                     1,len - length(blankWord)-1));
                        end;
                    else
                       begin end;
                 end;
              end;
        else
           begin end;
        end;
     until done;
end;

procedure get_filename;
begin
     GotoXY(17,1);
     write ('File:                ');
     GotoXY(23,1);
     readln(filename);
     if pos('.',filename) = 0 then
        filename := filename + '.k'
end;

procedure read_game;
var  x,y: integer;
     fil: text;
     ch:  char;
     line: string[cMax];
     unused,empty: char;
begin
     get_filename;
     if filename = '.k' then
        exit;
     assign(fil,filename);
     reset(fil);
     GotoXY(1,5);
     if IOresult <> 0 then
        begin
           write ('File not found');
           close(fil);
           exit
        end;
     readln(fil,unused,empty);
     for y := rMin to rMax do          { Currently, board size fixed }
        begin
           readln(fil,line);
           GotoXY(1,y);
           write(line);
           for x := 1 to cMax do
              begin
                 game[x,y] := Copy(line,x,1);
                 if game[x,y] = unused then
                    game[x,y] := cUnused
                 else
                    if game[x,y] = empty then
                       game[x,y] := cEmpty
              end
        end;
     command := 2;
     clearWords;
     readln(fil,ch);
     x := 1;
     if ch <> '*' then
        write ('Improper format')
     else
        while not eof(fil) do
           begin
              readln(fil, words.txt[x]);
              x := x + 1
           end;
     close(fil);
     menuDisplay (words);
     countWords;
     if (unused <> cUnused) or (empty <> cEmpty) then
        drawboard (game);
     original := game;
     origWords := words;
end;

procedure save_game;
var x,y: integer;
    fil: text;
begin
     get_filename;
     if filename = '.k' then exit;
     assign(fil,filename);
     rewrite(fil);
     writeln(fil,cUnused,cEmpty);
     for y:= rMin to rMax do
        begin
           for x := 1 to cMax do
              write(fil,game[x,y]);
           writeln(fil);
        end;
     writeln(fil,'*');
     for x := 1 to maxWords do
        writeln(fil,words.txt[x]);
     close(fil);
     GotoXY(1,5);
     if IOresult <> 0 then
        write ('Disk Error')
     else
        write (blankMessage);
end;

procedure edit_game;
var  x,y,code: integer;
     ch: char;
     done: boolean;
begin
     done := false;
     x := 1;  y := rMin;
     repeat
        GotoXY(x,y);
        getKey(ch,code);
        case ch of
           #27:
              case code of
                 0:  done := true;
                 72: begin
                        y := y - 1;
                        if y < rMin then y := rMax
                     end;
                 75: begin
                        x := x - 1;
                        if x = 0 then x := cMax
                     end;
                 77: x := (x mod cMax) + 1;
                 80: begin
                        y := y + 1;
                        if y > rMax then y := rMin
                     end;
                 83: begin
                        game[x,y] := cUnused;
                        write(cUnused)
                     end;
                 else begin end
              end;
           #13:
              begin
                 x := 1;
                 y := (y mod rMax) + 1;
              end;
           ' ':
              begin
                 game[x,y] := cEmpty;
                 write(cEmpty);
              end;
           else begin end;
        end;
     until done;
end;

procedure edit_words;
var  escape: boolean;
begin
     GotoXY(edge,25);
     inverse(false);
     write(' Hit <Space> to stop editing words ');
     menuSelect (words, currentWord, true, escape);
     GotoXY(edge,25);
     write('                                   ');
     countWords;
end;

{ ------------------------------------------------------------------------ }


{ Given x,y in a word, and horizontal/vertical flag.  Returns x,y of start
  of word, length (=0 if no word), and full=true if filled with letters. }

procedure contains (var x,y: integer; horizontal: boolean;
                      var length: integer; var full: boolean);
var  x1,y1: integer;
begin
     length := 0;
     full := false;
     if game[x,y] = cUnused then
        exit;
     x1 := x;  y1 := y;
     full := true;
     while (x1 <= cMax) and (y1 <= rMax) and (game[x1,y1] <> cUnused) do
        begin
           length := length + 1;
           if game[x1,y1] = cEmpty then
              full := false;
           x1 := x1 + dirh[horizontal];
           y1 := y1 + dirv[horizontal];
        end;
     while (x >= 1) and (y >= rMin) and (game[x,y] <> cUnused) do
        begin
           length := length + 1;
           if game[x,y] = cEmpty then
              full := false;
           x := x - dirh[horizontal];
           y := y - dirv[horizontal];
        end;
     length := length - 1;
     x := x + dirh[horizontal];
     y := y + dirv[horizontal];
end;

procedure pause (s: errmsg);
begin
     if robot then
        exit;
     GotoXY(1,5);
     write (s,'  (enter)');
     readln;
     GotoXY(1,5);
     write (' ':length(s)+9);
end;

procedure deduced;    { Only one word remains for marked line }
var  i,j,x,y: integer;
begin
     normal(false);
     for i := 1 to maxWords do                { Find it again }
        if not dimlist[markedi][i] then
           begin
              x := markedx;
              y := markedy;
              for j := 1 to markedl do
                 begin
                    game[x,y] := copy(words.txt[i],j,1);
                    GotoXY(x,y);
                    write(game[x,y]);
                    x := x + dirh[markedh];
                    y := y + dirv[markedh]
                 end;
              if soundOn then
                 begin
                    sound(440);  delay(75);  sound(880);  delay(75);
                    sound(400);  delay(130);  nosound;
                 end;
              words.txt[i] := blankWord;
              if showWords then
                 menuDisplay(words);
              for j := 1 to lastIndex do
                 dimlist[j][i] := true;  { May lead to another deduction... }
                                         { Discover it when do mark, later  }
              changed[i] := true;
              wordsFound := wordsFound + 1;
           end
end;


procedure unmark;
var  len,i: integer;
begin
     for i := 1 to markedl do
        begin
           if game[markedx,markedy] = '+' then
              begin
                 game[markedx,markedy] := cEmpty;
                 GotoXY(markedx,markedy);
                 write(cEmpty);
               end;
           markedx := markedx + dirh[markedh];
           markedy := markedy + dirv[markedh];
        end;
     marked := false
end;

procedure mark (var xp,yp: integer; horizontal: boolean; var full: boolean);
var  len,i,oldindex: integer;
     ch: char;
     count: integer;
     x,y: integer;
     w: str;
begin
     x := xp; y := yp;
     if marked then
        unmark;
     contains(x,y,horizontal,len,full);
     if len <= 1 then
        exit;
     marked := true;
     markedx := x;
     markedy := y;
     markedh := horizontal;
     markedl := len;
     if robot then
        begin
           xp := x;
           yp := y
        end;

     oldindex := index[x,y,horizontal];
     markedi := oldindex;
     if oldindex = 0 then                           { first time marked }
        begin
           lastIndex := lastIndex + 1;
           index[x,y,horizontal] := lastIndex;
           changed[lastIndex] := true;
           markedi := lastIndex;
        end;
     w := '';
     for i := 1 to len do
        begin
           GotoXY(x,y);
           w := w + game[x,y];
           if game[x,y] = cEmpty then
              begin
                 game[x,y] := '+';
                 write('+');
              end;
           x := x + dirh[horizontal];
           y := y + dirv[horizontal];
        end;
     count := 0;
     if oldindex = 0 then
        begin
           for i := 1 to maxWords do
              begin
                 words.dim[i] := (length(words.txt[i]) <> len);
                 if not words.dim[i] then
                    count := count + 1
              end;
           dimlist[markedi] := words.dim;
        end
     else
        words.dim := dimlist[markedi];
     if showWords then
        menuDisplay(words);
     GotoXY(24,4);
     write(w,'     ');
     if count = 1 then
        deduced
end;

procedure constrain(x,y: integer);
var  offsetm,offsetc: integer;
     s: set of char;
     ch: char;
     x1,y1,len,i,idx,count: integer;
     reduced,full: boolean;
begin
     if markedh then                   { Offset in marked word }
        offsetm := x - markedx + 1
     else
        offsetm := y - markedy + 1;
     s := [];

     { Build a set of letters available from the potential crossing word.
       Potential crossing words are those left over by constraint on the
       crossing line. }

     if (game[x,y] <> cEmpty) and     { Special case: particular constraint }
        (game[x,y] <> '+') then
        s := [game[x,y]]
     else
        begin
           x1 := x;  y1 := y;
           contains(x1,y1,not markedh,len,full);
           if len <= 1 then
              begin
                 pause ('No crossing word');
                 exit
              end;
           if not markedh then               { Offset in crossing word }
              offsetc := x - x1 + 1
           else
              offsetc := y - y1 + 1;

           idx := index[x1,y1,not markedh];
           if idx = 0 then      { Crossing line hasn't been constrained yet }
              for i := 1 to maxWords do          { require the right length }
                 if length(words.txt[i]) = len then
                    s := s + [copy(words.txt[i],offsetc,1)]
                 else begin end
           else
              for i := 1 to maxWords do
                 if not dimlist[idx][i] then
                    s := s + [copy(words.txt[i],offsetc,1)];
        end;

     count := 0;
     reduced := false;     { Reduced the number of eligible words? }
     for i := 1 to maxWords do
        if not words.dim[i] then
           begin
              ch := copy(words.txt[i],offsetm,1);
              words.dim[i] := not (ch in s);
              if not words.dim[i] then
                 count := count + 1
              else
                 reduced := true
           end;
     dimlist[markedi] := words.dim;
     changed[markedi] := reduced;
     if reduced and soundOn then
        begin
           sound(880); delay(50); sound(2000); delay(50); nosound
        end;
     if showWords then
        menuDisplay(words);
     if count = 1 then
        deduced
end;

procedure play_game;
var  x,y,i,code,robcode,len: integer;
     robx,roby: integer;
     roboffset: integer;
     ch: char;
     done,full: boolean;
begin
     normal(false);
     GotoXY(1,3);
     write ('F1 - mark vertical     Enter - constrain':(edge - 5));
     GotoXY(1,4);
     write ('F2 - mark horizontal                    ':(edge - 5));
     done := false;
     wordsFound := 0;    { May want to resume old game...? }
     if robot then
        robmap := game;
     x := playx;  y := playy;
     robx := x;   roby := y;
     robcode := 59;

     repeat
        GotoXY(x,y);
        if robot then
           begin
              if (roboffset > markedl) or full then
                 begin
                    repeat
                       robx := (robx mod cMax) + 1;
                       if robx = 1 then
                          begin
                             roby := roby + 1;
                             if roby > rMax then
                                begin
                                   roby := rMin;
                                   robmap := game;
                                   if robcode = 59 then
                                      robcode := 60
                                   else
                                      robcode := 59
                                end
                          end;
                    until robmap[robx,roby] <> cUnused;
                    roboffset := 0;
                    x := robx;  y := roby;  ch := #27;  code := robcode
                 end
              else
                 begin
                    if roboffset >= 1 then
                       begin
                          x := x + dirh[markedh];
                          y := y + dirv[markedh];
                       end;
                    roboffset := roboffset + 1;
                    robmap[x,y] := cUnused;
                    ch := #13;
                 end
           end
        else
           getKey(ch,code);
        case ch of
           #13:
              if not marked then
                 pause ('No mark')
              else
                 if ((x <> markedx) and (y <> markedy)) or
                    (x >= markedx + markedl) or (y >= markedy + markedl) or
                    (x < markedx) or (y < markedy) then
                    pause ('Constrain on line only')
                 else
                    constrain(x,y);
           #27:
              case code of
                 0:  done := true;
                 59: mark (x,y,false,full);
                 60: mark (x,y,true,full);
                 72: begin
                        y := y - 1;
                        if y < rMin then y := rMax
                     end;
                 75: begin
                        x := x - 1;
                        if x = 0 then x := cMax
                     end;
                 77: x := (x mod cMax) + 1;
                 80: begin
                        y := y + 1;
                        if y > rMax then y := rMin
                     end;
              end
        end;
     until (wordsFound >= numWords) or done;
     if wordsFound >= numWords then
        pause ('Found all words');
     playx := x;  playy := y;
     for y := 3 to 4 do
        begin
           GotoXY(1,y);
           write(' ':(edge - 5))
        end;
     menuDisplay(main);
end;


{ Main Program }

begin
     NoSound;
     dirh[true]  := 1;    { horizontal and vertical directions }
     dirh[false] := 0;
     dirv[true]  := 0;
     dirv[false] := 1;
     playx := (cMax) div 2;
     playy := (rMin + rMax) div 2;
     marked := false;


     TextMode;
     ClrScr;
     inverted := false;
     GotoXY(1,1);
     Inverse(false);
     write (' Kriss Kross ');



     main.txt[1] := 'Read';
     main.txt[2] := 'Play';
     main.txt[3] := 'Edit';
     main.txt[4] := 'Words';
     main.txt[5] := 'Save';
     main.txt[6] := 'Clear';
     main.txt[7] := 'Quit';
     main.txt[8] := 'Auto';
     main.entries := 8;
     for i := 1 to main.entries do
        main.dim[i] := false;
     main.row  := 3;
     main.col  := 1;
     main.rows := 2;
     main.colWidth := 8;
     menuDisplay (main);

     clearBoard;
     drawBoard(game);
     clearWords;
     words.entries := maxWords;
     words.row := 1;
     words.col := edge;
     words.rows := 24;
     words.colWidth := 11;
     currentWord := 1;
     menuDisplay (words);

     quit := false;
     command := 1;
     repeat
        menuSelect (main, command, false, escape);
        GotoXY(1,5);
        write (blankMessage);
        GotoXY(1,5);
        if escape then
           write ('Escape')
        else
           case command of
             0: write ('Escape');
             1: read_game;
             2: begin
                   robot := false;
                   showWords := true;
                   soundOn := true;
                   if wordsFound >= numWords then
                      begin          { Start over }
                         words := origWords;
                         wordsFound := 0;
                         lastIndex := 0;
                         clearBoard;
                         game := original;
                         drawboard(game)
                      end;
                   play_game;
                end;
             3: begin
                   edit_game;
                   wordsFound := 0
                end;
             4: edit_words;
             5: save_game;
             6: begin
                   write ('Clear (y/n)? ');
                   readln (ch);
                   if (ch = 'y') or (ch = 'Y') then
                      begin
                         clearBoard;
                         drawBoard(game);
                         clearWords;
                         menuDisplay (words);
                      end;
                   GotoXY(1,5);
                   write (blankMessage);
                end;
             7: begin
                   write ('Quit (y/n)? ');
                   readln (ch);
                   if (ch = 'y') or (ch = 'Y') then
                      quit := true;
                   GotoXY(1,5);
                   write (blankMessage);
                end;
             8: begin
                   robot := true;
                   showWords := false;
                   soundOn := false;
                   if wordsFound >= numWords then
                      begin          { Start over }
                         words := origWords;
                         wordsFound := 0;
                         lastIndex := 0;
                         clearBoard;
                         game := original;
                         drawboard(game)
                      end;
                   play_game;
                end;
             else begin end
           end;
     until quit;

     gotoxy(1,4);
end.

