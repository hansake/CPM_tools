# Cyput sends multiple files using the YMODEM protocol

The cyput (and hopefully later cyget) program can
transfer files from and to CP/M computers using
the YMODEM protocol.

Currently the program is working on a Zendex Model 835 CP/M-80 system
and makes direct access to the serial channal port.
The program also works a Slicer CP/M-86 system, but accesses
the console port with BIOS calls via BDOS.

Compiled for CP/M-80 and CP/M-86.

The program tmrtst is used to determine the number
of program loops needed to get the correct timeout
for the serial recieve channal.
