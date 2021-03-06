/*  CCIO86.C - Input/output characters over the serial
 *  console channal on a CP/M-86 computer
 *  Written for  Whitesmiths C compiler version 2.2
 *  You are free to use, modify, and redistribute
 *  this source code. No warranties given.
 *  Hastily Cobbled Together 2021 by Hans-Ake Lund
 */

#include <std.h>
#include <cpm.h>
#include "cyio.h"

#define BIOSFUN 50 /* BDOS call to BIOS routines */

#define CONST 2  /* Returns its status in A;
                    0 if no character is ready, 0FFh if one is */
#define CONIN 3  /* Wait until the keyboard is ready to provide a character,
                    and return it in A */
#define CONOUT 4 /* Write the character in C to the screen */

/* CP/M BDOS BIOS (function 50) call structure */
#define BDOSBIOS struct bdosbios
struct bdosbios {
    UTINY biosfun;    /* BIOS function code */
    UTINY bioscl;     /* CL register */
    UTINY biosch;     /* CH register */
    UTINY biosdl;     /* DL register */
    UTINY biosdh;     /* DH register */
    };

LOCAL BDOSBIOS biosregs = {0};

/* I/O port table and default i/o port
 * and max number of ports
 */
GLOBAL IOPORTS ioportab[1] = {0};
GLOBAL COUNT iopdeflt = {0};
GLOBAL COUNT iopmax = {0};

/* The number of recieve loops needed for one second
 * recieve timeout on the computer
 */
GLOBAL ULONG timloops = {5600};

/* Initialize device information for this system
 * return default port number
 */
COUNT cginit()
    {
    ioportab[0].iopnum = 1;
    ioportab[0].iopname = "Console";
    iopdeflt = 1;
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
 * Empty routine for BIOS calls via BDOS
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

/* Input character from console port
 */
METACH cgetchr(portno, timeout)
    COUNT portno; /* unused, only console valid, but not checked (yet) */
    COUNT timeout;
    {
    METACH inchar;
    ULONG wtimloops;

    biosregs.biosfun = CONST;
    biosregs.bioscl = 0;
    biosregs.biosch = 0;
    biosregs.biosdl = 0;
    biosregs.biosdh = 0;
    while (0 < timeout--)
        {
        wtimloops = timloops;
        while (0 < wtimloops--)
            {
            if (cpm(BIOSFUN, &biosregs) != 0)
                {
                biosregs.biosfun = CONIN;
                biosregs.bioscl = 0;
                biosregs.biosch = 0;
                biosregs.biosdl = 0;
                biosregs.biosdh = 0;
                inchar = cpm(BIOSFUN, &biosregs);
                return (inchar);
                }
            }
        }
    return (-1); /* timeout */
    }

/* Output character on console port
 */
VOID cputchr(portno, chrout)
    COUNT portno;  /* unused, only console valid, but not checked (yet) */
    METACH chrout;
    {
    biosregs.biosfun = CONOUT;
    biosregs.bioscl = chrout;
    biosregs.biosch = 0;
    biosregs.biosdl = 0;
    biosregs.biosdh = 0;
    cpm(BIOSFUN, &biosregs);
    }

           