# Cross compiling Zmodem implementations czput and czget for 8080 CP/M
# First target is the Zendex computer
# czget is not ready yet
#
all: czput.com

# Send file(s) with Zmodem
czput.com: czput.o czm.o cglob.o cziozen.o crctab.o ltell.o
	cx80 -proto cxcpm80rel.pro -L/opt/wsl/lib/80/ -o $@ $^

# Recieve file(s) with Zmodem - TODO
czget.com: czget.o czm.o cziozen.o crctab.o ltell.o
	cx80 -proto cxcpm80rel.pro -L/opt/wsl/lib/80/ -o $@ $^

%.o: %.c
	cx80 -proto cxcpm80.pro -I "|/opt/wsl/include/v22/" -s +o $<

%.o: %.s
	cx80 -proto cxcpm80.pro +o $<

clean:
	rm -f *.o *.com *.commap
