// system - built-in declarations and definitions.
//
// This unit is used automatically by every module.
//

unit system;

interface

type PChar = ^Char;

// My version of AnsiString is similar to C++ std::string
// As with the latest std::string, it doesn't use reference counting.
// I don't yet have small-string optimization.
// value is always an allocated buffer, containing null-terminated text.
type AnsiString = record
  length: integer;
  capacity: integer;
  value: PChar;
end;


implementation


initialization
  begin
    writeln( 'Hello from system.initialization' );
  end;

finalization
  begin
    writeln( 'Hello from system.finalization' );
  end

end.

