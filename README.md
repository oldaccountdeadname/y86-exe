y86 CPU implementation
======================

This is an implementation of the Y86 ISA. It works with output from
[this assembler][asm], which is, namely 10-byte padded
instructions. This only current backend implementation is SEQ
(sequential, non-pipelined execution).

[asm]: https://github.com/lincolnauster/y86-asm
