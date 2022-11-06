program t17;
  // array init

  type T1 = array[1 .. 3] of integer;
  const c1: T1 = ( 10, 20, 30 );

  // const c2: T1 = c1;
  //   error: expected ( but found identifier c1

  type T2 = array[1..3, 1..3] of integer;

  // const c2: T2 = ( 1, 2, 3, 4, 5, 6, 7, 8, 9 );
  //   error: expected ( but found ordinal const

  const c2: T2 = ( ( 1, 2, 3 ), ( 4, 5, 6 ), ( 7, 8, 9 ) );

  // var v1: T2 = c2;
  //   error: expected ( but found identifier c2

  const ci1: integer = 10;
  // var v1: T2 = ( ( ci1, 2, 3 ), ( 4, ci1, 6 ), ( 7, 8, ci1 ) );
  //    error: illegal expression, at each of the ci1's
  //    So, cannot use a typed const in this const initializer! Typed const more like a variable.

  // const cx: integer = ci1;
  //    error: illegal expression
  //    So can't even use it here.  

  // var vx: integer = ci1;
  //    error: illegal expression
  //    So can't even use it here.  

  // var avx: array [1..ci1] of integer;
  //    error: can't evaluate constant expression
  //    So, can't even use it here.  Really not considered const, at least globally

  procedure proc1( param1: integer );
      // var pvx: integer = ci1;
      //   error: illegal expression
      //   So doesn't relax rule in later proc initialization

      // var pvx: integer = param1;
      //   error: illegal expression
      //   surprising, can't reference non-const even here

      // type pat = array [1..param1] of integer;
      //   error: can't evaluate constant expression
    begin
    end;

 { TO DO: My compiler doesn't work here yet:
  const ci2 = 10;
  var v1: T2 = ( ( ci2, 2, 3 ), ( 4, ci2, 6 ), ( 7, 8, ci2 ) );
  //    -- that is allowed.  i.e. untyped const, which is more like preprocessor
 }

begin

  writeln( c1[1], ',', c1[2], ',', c1[3] );

  // writeln( c1 );
  //   error: can't read or write variables of this type
  
  writeln( c2[1,1], ',', c2[1,2], ',', c2[1,3] );
  writeln( c2[2,1], ',', c2[2,2], ',', c2[2,3] );
  writeln( c2[3,1], ',', c2[3,2], ',', c2[3,3] );
  writeln;

{
  writeln( v1[1,1], ',', v1[1,2], ',', v1[1,3] );
  writeln( v1[2,1], ',', v1[2,2], ',', v1[2,3] );
  writeln( v1[3,1], ',', v1[3,2], ',', v1[3,3] );
  writeln;
}
  // v1 := ( ( ci2, 2, 3 ), ( 4, ci2, 6 ), ( 7, 8, ci2 ) );
  //  error: found , but expected )  i.e. normal expression parenthesis; can't use array notation here

end.
