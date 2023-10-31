// sizeof(x)

program t24;

  function f : integer;
    begin
      writeln( 'Hello from f' );
      f := 1;
    end;

  function g( i: integer ) : integer;
    begin
      writeln( 'Hello from g' );
      g := 10;
    end;
    
  var i1: longint;

  const c1 = sizeof( char );
  const c2 = sizeof( c1 + c1 );
  const c3 = sizeof( longint( 'a' ) );

begin

  // fpc does not execute the expression given to sizeof.  No side effect.
  writeln( sizeof( f ) );
  writeln( sizeof( g( f ) ) );

  writeln( sizeof( 10 + 40000 ) );
  // -- fpc: 2 -  must have considered 40000 as uint16_t
  //              and addition stayed there

  writeln( sizeof( 40000 - 10 ) );
  // -- fpc: 2 -  must have evaluated const subtraction before deciding result type

  writeln( sizeof( 10 - 40000 ) );
  // -- fpc: 4 -  must have evaluated const subtraction before deciding result type

  writeln( sizeof( 70000 ) );
  // -- fpc: 4

  i1 := 1;
  writeln( sizeof( i1 ), ' ', sizeof( i1 * 2 ) );
  // -- fpc: 4 8   - put * in real, and that's double??

  //writeln( sizeof( real ) );
  // -- fpc: 8

  //writeln( sizeof( i1 div 2 ), ' ', sizeof( i1 / 2 ), ' ', sizeof( i1 / 1 ) );
  // -- fpc: 8 8 8  - hmm even div went to real?? or int64?
  //     and didn't optimize away /1

  // writeln( sizeof( array[1..3] of 1..10 ) );
  //   -- fpc: error: expected ) but found [

  // writeln( sizeof( record a: integer end ) );
  //   -- fpc: error: expected ) but found identifier A

  // writeln( sizeof( 1..10 ) );
  //  -- fpc: error: expected ) but found ..


  writeln( sizeof( 'Strlit' ) );
  //  -- fpc:  6


  // An expression that starts with a type name (a type cast):
  writeln( sizeof( ShortString( 'Strlit' ) ) );
  //  -- fpc: 256

  writeln( 'Test n: ', sizeof( longint( 'A' ) ) );

  // So I conclude that the argument of sizeof() may be
  //   a simple type name,  or an expression (which is not evaluated)
  //   And, tricky, the expression may start with a typecast
  //   which also consists of a type name.

  // This is another case I have difficulty with at the moment.
  // The type name starts with a unit prefix.
  // I can't handle it because I currently need to be able to unaccept,
  // to switch from typename to expression.  And I can only unaccept one token.
  // writeln( sizeof( system.LongInt ) );

  // writeln( sizeof( mysystem.LongInt ) );

  // Ok here's a clue.  fpc allows this.
  // Suggests that within sizeof they allow type names within expressions.
  // That might be workable for me.  Yeah that's what I've done.
  writeln( sizeof( LongInt + 1 ) );

  writeln( c1, ' ', c2, ' ', c3 );
end.
