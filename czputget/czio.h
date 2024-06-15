/*  czio.h - Low level character input/output
 *
 *  Written for  Whitesmiths C compiler version 2.2
 *  You are free to use, modify, and redistribute
 *  this source code. No warranties given.
 *  Hastily Cobbled Together 2024 by Hans-Ake Lund
 */

/* I/O port definition table */
#define IOPORTS struct ioports
struct ioports {
    COUNT iopnum;      /* I/O port number, -1 is end of table */
    TEXT *iopname;     /* I/O port name */
    UTINY iopdr;       /* I/O port data register address */
    UTINY iopsr;       /* I/O port status register address */
    UTINY txmask;      /* Mask to isolate transmit ready bit */
    UTINY txrdy;       /* Value when ready */
    UTINY rxmask;      /* Mask to isolate receive ready bit */
    UTINY rxrdy;       /* Value when ready */
    };

/* Currently selected i/o port */
IMPORT COUNT portno;

