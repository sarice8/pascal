// readln

program t25;
  var s1: shortstring;
  var i1, i2: longint;
  var c1: char;
{
  var ss1: string[3];
}

  var byte1: byte;
  var bool1: boolean;

begin
  write( 'What is your name? ');
  readln( s1 );
  writeln( 'Hello, ', s1, '!' );

  write( 'Enter: number> ' );
  readln( i1 );
  writeln( 'Saw ', i1 );

  write( 'Enter: number string number> ' );
  readln( i1, s1, i2 );
  writeln( 'I saw ', i1, ', "', s1, '", ', i2 );

{
  write( 'Enter: number string[3] number> ' );
  readln( i1, ss1, i2 );
  writeln( 'I saw ', i1, ', "', ss1, '", ', i2 );

  write( 'Enter a byte: ' );
  readln( byte1 );
  //  -- fpc does allow this
  writeln( 'I saw ', byte1 );
}

  // write( 'Enter a boolean: ' );
  // readln( bool1 );
  //  -- fpc error: can't read or write values of this type
  //        (but actually it does allow write, so message is wrong here)
  // writeln( 'I saw ', bool1 );


end.
