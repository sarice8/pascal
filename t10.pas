
// looking for a bug in jit


{
  23    tPushGlobalI       8
  25    tPushConstI       50
  27    tLessI
  28    tJumpFalse         2

  30    tJumpTrue          3

  32    tLabel     2
  34    tPushGlobalI       8
  36    tPushConstI      250
  38    tGreaterI
  39    tJumpFalse         4

  41    tLabel     3
  43    tPushAddrGlobal   16  "A"
  45    tWriteStr
  46    tWriteCR
  47    tLabel     4
}


program test (input, output);

  var fx, dx : integer;
begin
  
  fx := 50;
  dx := 2;

  if ( fx < 50 ) or ( fx > 250 ) then
    writeln( 'A' );

  writeln( 'B' );
end.
