/*	TMRTST.C: Test timeout function for getting a character
 *      from the serial channel
 */
#include <std.h>

/*  
 *    flags:
 *      -d      debug flag
 *      -p#     port number
 */

GLOBAL METACH cgetchr();
GLOBAL VOID cputchr();

GLOBAL ULONG timloops;

GLOBAL BOOL dflag = NO;
GLOBAL COUNT portno = 2; /* Console */

TEXT *_pname {"tmrtst"};

/*    Test program for serial channal timout setting
 */
BOOL main(ac, av)
    BYTES ac;
    TEXT **av;
    {
    COUNT loops;
    METACH inchar;
    ULONG newtiml;

    getflags(&ac, &av, "d,p#", &dflag, &portno);

    if (dflag)
        putfmt("dflag = %p, portno = %i\n", dflag?"YES":"NO", portno);

    if (portno == 1)
        putfmt("Using port 1: TTY\n");
    else if (portno == 2)
        putfmt("Using port 2: CONSOLE\n");
    else
        error("Unknown port", NULL);
    putfmt("Press <Enter> to start timout test\n");
    getch();
    putfmt("Press any key after 30 seconds\n");
    for (loops = 0; loops < 70; loops++)
        {
        cputchr(portno, '.');
        inchar = cgetchr(portno, 1);
        if (inchar < 0)
            continue;
        else if ((inchar & 0xff) == 3) /* Ctrl-C */
            error("^C", "");
        else
            break;
        }
    if (70 <= loops)
        {
        putfmt("\n");
        error("no input character to stop timeout test", NULL);
        }
    putfmt("\nIn 30 seconds %i tests with 1 second timeout were made\n", loops);
    putfmt("Current value of timloops is %l\n", timloops);
    newtiml = (timloops * loops)/30;
    putfmt("Proposed value of timloops is %l\n", newtiml);
    return (YES);
    }
dflag?"YES":"NO", portno);

    if (portno == 1)
        putfmt("Using port 1: TTY\n");
    else if (