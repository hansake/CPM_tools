/*  cziozen.c - Character input/output serial channal
 *  For serial channels on Zendex ZX-85 Processor Board
 *
 *  Written for  Whitesmiths C compiler version 2.2
 *  You are free to use, modify, and redistribute
 *  this source code. No warranties given.
 *  Hastily Cobbled Together 2024 by Hans-Ake Lund
 */

#include <std.h>
#include "zmodem.h"
#include "czio.h"

/* Serial ports on Zendex ZX-85 Processor Board
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

/* Currently selected serial port
 */
GLOBAL COUNT portno = {2};
int siosr = {SIO2CR};
int siodr = {SIO2DR};

/* Receive FIFO buffer
 */
#define RXBUFSIZE 1024 /* Must be power of two for the FIFO mask to work */
unsigned char rxbuf[RXBUFSIZE] = {0};
int rxinidx = {0};
int rxoutidx = {0};

/* Receive timeout loops between characters
 * for now a defined number, but should be
 * dependent on selected baudrate
 */
#define CTOUTLOP 50
int ctimlop = {0};

/* Last sent character
 * (with bit 8 reset )
 */
METACH txlast = -1;

GLOBAL int trxinidx = {0};
GLOBAL int charin = {0};
GLOBAL unsigned char inchr = {0};

/* Number of consecutive recieved CANs (Ctrl-X)
 */
int ncans = {0};

/* Initialize device information for this system
 * return default port number
 */
COUNT czinit()
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

/* Select port and check 
 * that port number is valid
 */
BOOL czportok(port)
    COUNT port;
    {
    if (0 < port && port <= iopmax)
      {
      siodr = ioportab[port - 1].iopdr;
      siosr = ioportab[port - 1].iopsr;
      return (YES);
      }
    else
      return (NO);
    }

/* Get port name
 */
TEXT *czname(port)
    COUNT port;
    {
    return (ioportab[port - 1].iopname);
    }

/* Do what is needed to make raw character i/o
 * Empty routine for Zendex
 */
VOID czstart()
    {
    }

/* Clear the reciever input
 */
VOID czpurge()
    {
    while ((in(siosr) & RCVMASK) == RCVRDY)
        in(siodr);
    }

/* Restore to normal character i/o
 * Empty routine for Zendex
 */
VOID czstop()
    {
    }

/*  Check if any input characters available
 */
int rxpoll()
    {
    if ((in(siosr) & RCVMASK) == RCVRDY)
        {
        rx2buf();
        return (1);
        }
    else
        return (0);
    }


/*  Serial input to FIFO buffer
 */
VOID rx2buf()
    {
    /* Get as many characters as possible with a small timeout
     * between them. (CTOUTLOP should be dependent on Baudrate.)
     * Put the received characters in a FIFO buffer.
     */
    for (ctimlop = 0; ctimlop < CTOUTLOP; ctimlop++)
        {
        if ((in(siosr) & RCVMASK) == RCVRDY)
            {
            inchr = in(siodr);
            trxinidx = (rxinidx + 1) & (RXBUFSIZE - 1);
            if (trxinidx != rxoutidx)
                {
                rxbuf[rxinidx] = inchr;
                rxinidx = trxinidx;
                }
            }
        }
    return;
    }

/* Get a single received character from the rx FIFO buffer.
 * Check the data stream for 5 consecutive CAN characters;
 * and if you see them abort.
 */
int rxraw(timeout)
    int timeout; /* not implemented yet */
    {
    rx2buf();
    if (rxoutidx == rxinidx)
        {
        return (-1);
        }
    else
        {
        charin = rxbuf[rxoutidx];
        rxoutidx = (rxoutidx + 1) & (RXBUFSIZE - 1);
        }
    if (charin == CAN) /* Ctrl-X */
        {
        ncans++;
        if (5 <= ncans)
            {
            /*
             * the other side is serious about this. just shut up;
             * clean up and exit.
             */
            cleanup();
            exit(0);
            }
        }
     else
        {
        ncans = 0;
        }
    return (charin);
    }

/* Transmit a character and check for recieved characters
 * while waiting for transmit ready.
 */
VOID txraw(chrout)
    METACH chrout;
    {
    txlast = chrout & 0x7f;
    while (YES)
        {
        if ((in(siosr) & XMTMASK) == XMTRDY)
            {
            out(siodr, chrout);
            break;
            }
        else
            {
            /* test also if characters recieved while transmitting */
            if ((in(siosr) & RCVMASK) == RCVRDY)
               {
               inchr = in(siodr);
               trxinidx = (rxinidx + 1) & (RXBUFSIZE - 1);
               if (trxinidx != rxoutidx)
                   {
                   rxbuf[rxinidx] = inchr;
                   rxinidx = trxinidx;
                   }
                }
            }
        }
    }

VOID txflush()
    {
    }

