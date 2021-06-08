# Port the Whitesmiths C compiler to Linux
A slowly ongoing project is to port the Whitesmiths C compiler version 2.2
to be compiled with gcc and run on a 64 bit Linux host.
The primary target is 8080/Z80 in order to make programs
for: [hansake/Z80_Computer_board: A simple Z80 based computer board](https://github.com/hansake/Z80_Computer_board).

When the cross compiler is working the code may be placed here.

# Notes on the port to 64 bit gcc
Functions with variable arguments must be modified to use the va_ macros in <stdarg.h>.
The original Whitesmiths code assumes a straight linear placement of arguments on the stack.

A major problem seems to be that functions that returns pointers and are not specified before they are used
will return a 32 bit int instead of a 64 bit pointer, this will cause a core dump when the truncated
pointer is used.
