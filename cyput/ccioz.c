/*  CCIOZ.C - Input/output characters over serial
 *  channal on Zendex ZX-85 Processor Board
 *  Written for Whitesmiths C compiler version 2.2.
 *  You are free to use, modify, and redistribute
 *  this source code. The software is provided "as is",
 *  without warranty of any kind.
 *  Hastily Cobbled Together 2021 by Hans-Ake Lund.
 */

#include <std.h>

/* Serial channels on Zendex ZX-85 Processor Board
 * with two Intel 8251 USARTs
 */
#define SIO1DR 0xf4 /* Port 1 Data Register */
#define SIO1CR 0xf5 /* Port 1 Control Register */
#define SIO2DR 0xf6 /* Port 2 Data Register */
#define SIO2CR 0xf7 /* Port 2 Control Register */

#define XMTMASK 1   /* Mask to isolate Xmit Ready bit */
#define XMTRDY  1   /* Value when ready */
#define RCVMASK	2   /* Mask to isolate Receive Ready bit */
#define RCVRDY  2   /* Value when ready */

/* The number of recieve loops needed for one second
 * recieve timeout on the Zendex ZX-85 Processor Board
 */
GLOBAL ULONG timloops = 2200;

GLOBAL METACH cgetchr(portno, timeout)
    COUNT portno;
    COUNT timeout;
    {
    UTINY siodr;
    UTINY siocr;
    ULONG wtimloops;

    if (portno == 1)
        {
        siodr = SIO1DR;
        siocr = SIO1CR;
        }
    else
        {
        siodr = SIO2DR;
        siocr = SIO2CR;
        }

    while (0 < timeout--)
        {
        wtimloops = timloops;
        while (0 < wtimloops--)
            {
            if ((in(siocr) & RCVMASK) == RCVRDY)
                return (in(siodr));
            }
        }
    return (-1); /* timeout */
    }

GLOBAL VOID cputchr(portno, chrout)
    COUNT portno;
    METACH chrout;
    {
    UTINY siodr;
    UTINY siocr;

    if (portno == 1)
        {
        siodr = SIO1DR;
        siocr = SIO1CR;
        }
    else
        {
        siodr = SIO2DR;
        siocr = SIO2CR;
        }
    while (YES)
        {
        if ((in(siocr) & XMTMASK) == XMTRDY)
            {
            out(siodr, chrout);
            break;
            }
        }
    }
    
