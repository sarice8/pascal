// case statement

program t21;

  var i : integer;
  const c1 = 1;
  const c2 : integer = 2;

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

begin

  for i := 1 to 10 do
    proc1( i );

  proc2( 'Cat' );
  proc2( 'Woman' );
  proc2( 'Rock' );
  proc2( 'snowflake' );
  proc2( '94555' );
  proc2( 'E' );
  proc2( '' );

  case South of
    North, East: writeln( 'NorthEast' );
    South..West: writeln( 'SouthWest' );
  end;

  writeln( 'The End' );
end.
