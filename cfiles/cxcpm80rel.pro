#	WHITESMITHS ENVIRONMENT PROTOTYPE FILE
#
c:(e)pp	-o (o) {-I?-i (h)} -x -dUTEXT (i)
1:(e)p1	-o (o) -b0 -n8 -u (i)
2:(e)p2.80	-o (o) (i)
s:(e)as.80	-o (o) (i)
o::(e)linkw -a -eb__memory -ed__edata -tb0x100 -o (o).tmp (l)crts.80 (i) (l)libc.80 \
  && (e)rel80 -v (o).tmp > (o)map && (e)linkw -htr -o (o) (o).tmp && rm (o).tmp
com:
