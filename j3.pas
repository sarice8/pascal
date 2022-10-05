// jit testing

program test (input, output);

var I1: integer;
var I2: integer;

type RecType = record
  f1: integer;
  f2: integer;
  f3: integer;
  end;

type ArrayType = array [1..4] of integer;

var R1: RecType;
var A1: ArrayType;
var B1, B2, B3, B4: boolean;

begin

 // grammar test
 B2 := false;
 B3 := true;
 B4 := false;
 B1 := B2 = B3 <> B4;   // gives false, for either (B2=B3)<>B4 or B2=(B3<>B4) but expect former
 writeln( 'B1 = ', B1 );
 I1 := - 3;

 // These are accepted by FPC, but not sure what grammar allows them.
 // I1 := - -3;
 // I1 := -+-+-3;
 // I1 := 3 --+-+-3;
 // writeln( 'I1 = ', I1 );

 {
  I1 := 8;
  I2 := I1 + 10 + 22;

  A1[1] := 5;
  A1[2] := 7;
  A1[3] := 9;
  A1[2 + 2] := 11;
 }

  I1 := 3;
  A1[ I1 ] := I1 * 5;

  I2 := A1[ I1 ];
  A1[ I1 ] := I1 * I2;

  writeln( I2 );
  writeln( A1[ I1 ] );

  if ( I1 < 3 ) then
    writeln( 'Stmt 1' )
  else
    writeln( 'Stmt 2' );

  if ( ( I1 < 3 ) and ( I2 > 10 ) ) then
    writeln( 'Stmt 3' )
  else
    writeln( 'Stmt 4' );

 {
  A1[ I1 + 1 ] := 14;


  R1.f1 := 4;
  R1.f2 := 5;
  R1.f3 := 6;
  R1.f3 := I1 + I2;

  writeln( R1.f1, ' ', R1.f2, ' ', R1.f3 );
 }
end.
