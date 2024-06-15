# Cross compiling cfiles.com for 8080 CP/M
# List file sizes
#
all: cfiles.com

# Mainly for testing the ltell() routine
cfiles.com: cfiles.o cglob.o ltell.o
	cx80 -proto cxcpm80rel.pro -L/opt/wsl/lib/80/ -o $@ $^

%.o: %.c
	cx80 -proto cxcpm80.pro -I "|/opt/wsl/include/v22/" -s +o $<

%.o: %.s
	cx80 -proto cxcpm80.pro +o $<

clean:
	rm -f *.o *.com *.commap
