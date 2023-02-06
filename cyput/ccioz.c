/*  CCIOZ.C - Input/output characters over serial
 *  channal on Zendex ZX-85 Processor Board
 *  Written for  Whitesmiths C compiler version 2.2
 *  You are free to use, modify, and redistribute
 *  this source code. No warranties given.
 *  Hastily Cobbled Together 2021 by Hans-Ake Lund
 */

#include <std.h>
#include "cyio.h"

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

/* I/O port table and default i/o port
 * and max number of ports
 */
GLOBAL IOPORTS ioportab[2] = {0};
GLOBAL COUNT iopdeflt = {0};
GLOBAL COUNT iopmax = {0};

/* The number of recieve loops needed for one second
 * recieve timeout on the Zendex ZX-85 Processor Board
 */
GLOBAL ULONG timloops = {2200};

/* Initialize device information for this system
 * return default port number
 */
COUNT cginit()
    {
    ioportab[0].iopnum = 1;
    ioportab[0].iopname = "TTY";
    ioportab[0].iopdr = SIO1DR;
    ioportab[0].iopsr = SIO1CR;
    ioportab[0].txmask = XMTMASK;
    ioportab[0].txrdy = XMTRDY;
    ioportab[0].rxmask = RCVMASK;
    ioportab[0].rxrdy = RCVRDY;
    ioportab[1].iopnum = 2;
    ioportab[1].iopname = "CRT";
    ioportab[1].iopdr = SIO2DR;
    ioportab[1].iopsr = SIO2CR;
    ioportab[1].txmask = XMTMASK;
    ioportab[1].txrdy = XMTRDY;
    ioportab[1].rxmask = RCVMASK;
    ioportab[1].rxrdy = RCVRDY;
    iopdeflt = 2;
    iopmax = 2;
    return (iopdeflt);
    }

/* Check if port number is valid
 */
TEXT *cgportok(portno)
    COUNT portno;
    {
    if (0 < portno && portno <= iopmax)
      return (YES);
    else
      return (NO);
    }

/* Get port name
 */
TEXT *cgname(portno)
    COUNT portno;
    {
    return (ioportab[portno - 1].iopname);
    }

/* Do what is needed to make raw character i/o
 * Empty routine for Zendex
 */
VOID cgstart(portno)
    COUNT portno;
    {
    }

/* Restore to normal character i/o
 */
VOID cgstop(portno)
    COUNT portno;
    {
    }

/* Input character from serial port
 */
METACH cgetchr(portno, timeout)
    COUNT portno;
    COUNT timeout;
    {
    UTINY siodr;
    UTINY siocr;
    ULONG wtimloops;

    siodr = ioportab[portno - 1].iopdr;
    siocr = ioportab[portno - 1].iopsr;

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

/* Output character on serial port
 */
VOID cputchr(portno, chrout)
    COUNT portno;
    METACH chrout;
    {
    UTINY siodr;
    UTINY siocr;

    siodr = ioportab[portno - 1].iopdr;
    siocr = ioportab[portno - 1].iopsr;

    while (YES)
        {
        if ((in(siocr) & XMTMASK) == XMTRDY)
            {
            out(siodr, chrout);
            break;
            }
        }
    }
