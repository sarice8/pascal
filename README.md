# pascal
Hobby pascal compiler I started in 1989, and recently returned to.

The first pass parses the Pascal and generates intermediate code for an abstract stack machine.
This pass is written in S/SL, the "Syntax/Semantic Language".
(This repository includes my own implementation of this language.  The compiler is self-hosted,
but is represented as a C program so you just need a C/C++ compiler to use it.)

The second pass is a JIT that translates the intermediate code into machine code for the target hardware.
Currently only X86-64 running on Linux is supported.

Alternatively, the intermediate code can be interpreted.
