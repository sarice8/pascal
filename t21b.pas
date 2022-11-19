// case statement

program t21;

  var i : integer;
  const c1 = 1;
  const c2 : integer = 2;

  var b: byte;

  type Direction = ( North, East, South, West );

  procedure proc1( i1: integer );
    begin
      case i1 of
        c1:  writeln( 'One' );
        //c2:  writeln( 'Two' );
        //  -- fpc: error: constant expression required
        //     so, typed constant not considered a constant
        // fpc does allow this:
        c1 + 1: writeln( 'Two' );
        3:  writeln( 'Three' );
        6, 7..8:  writeln( 'Six to Eight' );
        otherwise
          write( 'Something else' );
          writeln( ' entirely' );
      end;
    end;

{
  procedure proc2( s1: ShortString );
    begin
      case s1 of
        'Dog', 'Cat', 'Pig' :  writeln( 'Animal' );
        'Man', 'Woman' :  writeln( 'Human' );
        // 'A'..'Z' : writeln( 'Something capitalized' );
        //    -- fpc error: duplicate case
        'E' : writeln( 'Most common letter' );
        'a'..'z' : writeln( 'Something lowercase' );
        else
          write( 'Something else' );
          writeln( ' entirely' );
      end;
    end;
}

  procedure proc3( b1: byte );
    begin
      case b1 of
        1..100: writeln( '1..100' );
        101..130: writeln( '101..130' );
        132: writeln( '132' );
        133..250: writeln( '133..250' );
        otherwise writeln( 'otherwise' );
      end;
    end;

  function proc4( c: char ) : string;
    begin
      case c of
        'a'..'z': proc4 := 'lower';
        'A'..'Z': proc4 := 'upper';
        ' ': proc4 := 'space';
        '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '=',
        '_', '+', '[', ']', '{', '}', '\', '|' :  proc4 := 'punct';
        '0'..'9': proc4 := 'digit';
        else proc4 := 'other';
      end;
    end;

begin

  b := 132;
  case b of
    132: writeln( 'byte comparison ok' );
    else writeln( 'byte comparison bad' );
  end;


  for i := 1 to 10 do
    proc1( i );

{

  proc2( 'Cat' );
  proc2( 'Woman' );
  proc2( 'Rock' );
  proc2( 'snowflake' );
  proc2( '94555' );
  proc2( 'E' );
  proc2( '' );
}

  case South of
    North, East: writeln( 'NorthEast' );
    South..West: writeln( 'SouthWest' );
  end;

  proc3( 0 );
  proc3( 50 );
  proc3( 100 );
  proc3( 132 );
  proc3( 240 );
  proc3( 255 );

  for i := 1 to 17 do
    write( proc4( 'The Time is 3:15!'[i] ), ' ' );
  writeln;

  writeln( 'The End' );
end.
