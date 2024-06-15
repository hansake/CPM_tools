/*  czm.h - zmodem primitives prototypes and data
 *
 *  Based on https://github.com/codesmythe/zmtx-zmrx
 *  (C) Mattheij Computer Service 1994
 *  GNU General Public License v2.0
 *
 *  Rewritten for  Whitesmiths C compiler version 2.2
 *  You are free to use, modify, and redistribute
 *  these source code modifications. No warranties given.
 *  Hastily Cobbled Together 2024 by Hans-Ake Lund
 */

#ifndef _CZM_H
#define _CZM_H

#define TRUE 1
#define FALSE 0

#define ENDOFFRAME 2
#define FRAMEOK 1
#define TIMEOUT -1    /* rx routine did not receive a character within timeout */
#define INVHDR -2     /* invalid header received; but within timeout */
#define INVDATA -3    /* invalid data subpacket received */
#define ZDLEESC 0x800 /* one of ZCRCE; ZCRCG; ZCRCQ or ZCRCW was received */
                      /* ZDLE escaped */
#define HDRLEN 5      /* size of a zmodme header */

#define SMALL_TIMEOUT 2 /* Two seconds (approx.) */

IMPORT int infp;               /* input file descriptor */
IMPORT int outfp;              /* output file descriptor */
IMPORT UTINY rxdhdr[ZMAXHLEN]; /* last received header */
IMPORT int rxdhdlen;           /* last received header size */

IMPORT METACH txlast;

/*
 * receiver capability flags
 * extracted from the ZRINIT frame as received
 */
IMPORT BOOL canfudu;            /* can full duplex */
IMPORT BOOL canovio;            /* can do overlapped disk i/o */
IMPORT BOOL escachr;            /* escape all control characters */
IMPORT BOOL canbrk;             /* can handle break */
IMPORT BOOL canfc32;            /* can do 32 bit checksum */
IMPORT BOOL esc8bit;            /* escape characters with 8th bit set */
IMPORT BOOL usevhdr;            /* use variable length headers */

#endif /* _CZM_H */

