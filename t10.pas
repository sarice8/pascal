
// looking for a bug in jit

{
   jumps on wrong boolean condition

  18	tPushGlobalB	   8
  20	tNot
  21	tJumpFalse	   2

   0x7ffff6b1b027:	cmpb   $0x0,0x3fda(%rip)        # 0x7ffff6b1f008
   0x7ffff6b1b02e:	sete   %al
   0x7ffff6b1b031:	cmp    $0x0,%al
   0x7ffff6b1b034:	jne    0x7ffff6b1b059

}


program test (input, output);

  var x : boolean;
begin
  
  x := false;

  if not x then
    writeln( 'Ok' );

  writeln( 'Done' );
end.
