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

var glob: integer;


// procedure proc1( p1, p2 : integer; p3 : integer );
procedure proc1( p1, p2, p3 : integer );
    var total : integer;
  begin
    glob := p1;
    glob := p2;
    glob := p3;

    writeln( 'Hello from proc1' );
    writeln( '  glob = ', glob );

    total := p1 + p2 + p3;
    writeln( '  p1 = ', p1 );
    writeln( '  p2 = ', p2 );
    writeln( '  p3 = ', p3 );
    writeln( '  total = ', total );
  end;


// VAR parameters
procedure proc2( p1 : integer;
                 VAR p2, p3 : integer;
                 p4 : integer;
                 VAR p5 : rtype );
  var r1 : rtype;
  begin

    writeln( 'Hello from proc2' );
    p1 := p1 + 1000;
    p2 := p2 + 2000;
    p3 := p3 + 3000;
    p4 := p4 + 4000;
    writeln( '  p1 = ', p1 );
    writeln( '  p2 = ', p2 );
    writeln( '  p3 = ', p3 );
    writeln( '  p4 = ', p4 );

    r1.f1 := p1;
    r1.f2 := p2;
    r1.f3 := p3;
    p5 := r1;
  end;

function func1( p1 : integer;
                p2 : integer ) : integer;
  var r1 : rtype;
  begin
    r1.f1 := p1;
    r1.f2 := p2;
    r1.f3 := p1;

    func1 := r1.f1 + r1.f2 + r1.f3;
  end;


// main
BEGIN

  writeln( 'Func test:' );
  i := func1( 10, 20 );
  writeln( '  result i = ', i );

  writeln( 'Proc test:' );
  proc1( 100, 200, 300 );
  proc1( 400, 500, 600 );

  i := 5;
  j := 10;
  proc2( 1, i, j, 4, r1 );
  proc2( 1, i, j, 4, r1 );
  writeln( 'After proc2, i = ', i, ' j = ', j );
  writeln( '   r1.f1 = ', r1.f1, ' r1.f2 = ', r1.f2, ' r1.f3 = ', r1.f3 );

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
    //break;
    //continue;
    //break;
    //break;
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
      if I = 3 then
        continue;
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
