# CPM_tools
CP/M tools written in C

This reporitory contains some software tools for CP/M-80 and CP/M-86.
These tools are written in C and compiled with Whitesmiths C compiler version 2.2.
The compiler I used is here: [Whitesmiths-C-compiler/ccpm80_v22_bin at main · hansake/Whitesmiths-C-compiler](https://github.com/hansake/Whitesmiths-C-compiler/tree/main/ccpm80_v22_bin)

CP/M functions that are used by several tools are in the cpm_functions directory.

The program "cless" is a filter for paging through text one screenful at a time.

The program "cyput" transfer files from a CP/M system using the YMODEM protocol.

Hopefully there will also be a "cyget" program also that transfer files in the other direction.

The "czput" program in "czputget" is sending files using the ZMODEM protocol.
This is a very early version that needs optimsation.
Hopefully a "czget" program will also appear here.

The "cfiles" program lists file sizes.

Maybe more tools to come ...
