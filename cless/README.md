# Text files viewer cless

CLESS.COM runs on CP/M-80

CLESS.CMD runs on CP/M-86

cless [flags] files
  
  flags:<br>
    -d     debug flag<br>
    -f     print filename before printing file content<br>
    -n#    print n lines before asking to continue (22 default)<br>


After each screen shown, printing is controlled with:<br>
CR: print one more line<br>
Space: print one more screen<br>
n or N: print next file<br>
q or Q or Ctrl-C: end program<br>

The programs are compiled with Whitesmiths C compiler version 2.2.
