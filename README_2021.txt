
Notes from Steve 9/13/2021, while assembling my collection of old projects.

SSL is the Syntax/Semantic Language, invented by my Queen's University professor James Cordy.
An ssl file describes the legal structure of an input stream, and a high level indication of
how to process it.  It was typically used to implement individual passes in a compiler.

This is my own implementation of the language, for my personal compiler projects.

The first version "hssl.c" is a standalone implementation in C.
This was used to bootstrap later versions, written in SSL itself.

It looks like I didn't have a makefile for hssl.  Apparently I ran the script "do hssl" to build it.

The first version would read a *.ssl source file, and produce a *.tbl table file.
A client program that wanted to implement the language (or pass) described by that ssl file
would have to provide two things: a scanner (file tokenizer), and a table walker
(reading the tbl file and interpreting the operation codes listed there.)

Later versions of the ssl compiler simplify this a bit by generating C code for the table,
and offering a generic runtime ssl_rt, plus an interactive ssl debugger.

------

Version 1.2.3 (03/26/1991) is implemented in ssl itself.
The program is implemented by reading and walking through the table file ssl.tbl,
which is produced either by itself or by the earlier bootstrap implementation hssl.

(I don't have a makefile for this version, so I'm not totally certain that hssl
can produce a compatbile tbl file, but I expect it can.  The dates are about right.)

