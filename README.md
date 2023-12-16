# pascal
Hobby pascal compiler I started in 1989, and recently returned to.

The first pass parses the Pascal and generates intermediate code for an abstract stack machine.
This pass is written in S/SL, the "Syntax/Semantic Language".
(This repository includes my own implementation of this language.  The SSL compiler is self-hosted,
but is represented as a C program so you just need a C/C++ compiler to use it.)

The second pass is a JIT that translates the intermediate code into machine code for the target hardware.
Currently only X86-64 running on Linux is supported.

Alternatively, the intermediate code can be interpreted.

Please note - the base code here is very old, written in K&R C, and with things like fixed array sizes.
I haven't attempted yet to clean it up.  My goal for this hobby so far has been to finally reach the goal
that I set out way back then.
