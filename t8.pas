//  TEST8.PAS  -- checking progress on migrating symbol table to schema


program test (input, output);

const one = 1;
const two = 2;
const nine = 9;

type atype = array [one..nine] of array[one..two,one..two] of INTEGER;

type ptype = ^atype;
type rtype = record
    f1, f2: integer;
    f3, f4: integer;
  end;

type rrtype = record
    f1: integer;
    f2: rtype;
    f3: rtype;
  end;

var  a1,a2: atype;
     i,j:   integer;
     k: ^integer;
     l: integer;

     r1: rtype;
     r2: rtype;

     rr1: rrtype;

const c1 = 10;

BEGIN

  writeln( 'Record test:' );

  r1.f1 := 1;
  r1.f2 := 2;
  r1.f3 := 3;
  r1.f4 := 4;

  // did I implement compound var assignment yet?  Yep
  r2 := r1;

  writeln( 'r1=', r1.f1, r1.f2, r1.f3, r1.f4 );
  writeln( 'r2=', r2.f1, r2.f2, r2.f3, r2.f4 );

  rr1.f2 := r1;
  rr1.f3.f1 := 10;
  rr1.f3.f2 := 20;
  rr1.f3.f3 := 30;
  rr1.f3.f4 := 40;

  writeln( 'rr1.f2=', rr1.f2.f1, rr1.f2.f2, rr1.f2.f3, rr1.f2.f4 );
  writeln( 'rr1.f3=', rr1.f3.f1, rr1.f3.f2, rr1.f3.f3, rr1.f3.f4 );


(*
  for i := 5 to 10 do
  begin
    j := i;
    writeln( 'j is ', j );
    //exit;
    //cycle;
    //exit;
    //exit;
  end;

  i := c1 + 2;
  j := i * 2 + 5;
  k := ^i;
  l := k^ + 100;
  l := a1[ 1 ][ 2, 2 ];

  writeln ( 'pointer=', k, ' value=', k^ );
*)

  writeln ( 'SAR: ', a1[1][2,1] );
  writeln ( 'SAR: ', a1[1][2][1] );
  //i = a1[1][2,1];


  writeln ( 'SAR 1: ', a1[1][2,1] );
  writeln ('Here we go!');

  for i := 1 to 9 do
    begin
     (*
      if I = 3 then
        cycle;
      *)
      for j := 1 to 2 do
        a1[i][J,J] := i;
    end;

  writeln ( 'SAR 2: ', a1[1][2,1] );

  FOR I := 9 DOWNTO 1 DO
    begin
    writeln( a1[i][2,1] );
//(*
    writeln('-> ',a1[i][1,1],a1[i][1,2],a1[I,1][1],a1[i,1][2],a1[i,1,1],a1[i,1,2]);
    writeln('-> ',a1[i][2,1],a1[i][2,2],a1[I,2][1],a1[i,2][2],a1[i,2,1],a1[i,2,2]);
//*)
    writeln( a1[i][2,1] );
    end;


  write ('That''s ','all ');
  WRITELN ('folks!');


END.
