/* Input/output characters on Zendex */

#include <std.h>

/* Serial channels on Zendex ZX-85 Processor Board
 * with Intel 8251 USART
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

METACH cgetchr(portno, timeout)
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

VOID cputchr(portno, chrout)
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
    COUNT portno;
    COUNT tim