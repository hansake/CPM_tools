/*  TMRTST.C: Program to test timeout function for
 *  getting a character from the serial channel.
 *  Written for Whitesmiths C compiler version 2.2.
 *  You are free to use, modify, and redistribute
 *  this source code. The software is provided "as is",
 *  without warranty of any kind.
 *  Hastily Cobbled Together 2021 by Hans-Ake Lund.
 */
#include <std.h>

/*  
 *  flags:
 *   -d      debug flag
 *   -p#     port number
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

    portno = cginit();
    getflags(&ac, &av, "d,p#", &dflag, &portno);

    if (dflag)
        putfmt("dflag = %p, portno = %i\n", dflag?"YES":"NO", portno);

    if (cgportok(portno))
        putfmt("Timer test using port %i (%p)\n", portno, cgname(portno));
    else
        error("invalid port number", NULL);

    putfmt("Press <Enter> to start timout test\n");
    getch();
    putfmt("Press any key after 30 seconds\n");
    cgstart(portno);
    for (loops = 0; loops < 70; loops++)
        {
        cputchr(portno, '.');
        inchar = cgetchr(portno, 1);
        if (inchar < 0)
            continue;
        else if ((inchar & 0xff) == 3) /* Ctrl-C */
            {
            cgstop(portno);
            error("^C", "");
            }
        else
            break;
        }
    if (70 <= loops)
        {
        cgstop(portno);
        putfmt("\n");
        error("no input character to stop timeout test", NULL);
        }
    cgstop(portno);
    putfmt("\nIn 30 seconds %i tests with 1 second timeout were made\n", loops);
    putfmt("Current value of timloops is %l\n", timloops);
    newtiml = (timloops * loops)/30;
    putfmt("Proposed value of timloops is %l\n", newtiml);
    return (YES);
    }
 portno, cgname(portno));
    else
        error("invalid port number", NULL);

    putfmt("Press <Enter>