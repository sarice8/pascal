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


var  a1,a2: atype;
     i,j:   integer;
     k: ^integer;
     l: integer;

const c1 = 10;

BEGIN

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
