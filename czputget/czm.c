/*  czm.c - zmodem primitives and other code common to czput and czget
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

#include <std.h>
#include "zmodem.h"
#include "czm.h"
#include "crctab.h"

GLOBAL int infp = {0};               /* input file descriptor */
GLOBAL int outfp = {0};              /* output file descriptor */
GLOBAL UTINY rxdhdr[ZMAXHLEN] = {0}; /* last received header */
GLOBAL int rxdhdlen = {0};           /* last received header size */

/*
 * receiver capability flags
 * extracted from the ZRINIT frame as received
 */
GLOBAL BOOL canfudu = {0};            /* can full duplex */
GLOBAL BOOL canovio = {0};            /* can do overlapped disk i/o */
GLOBAL BOOL escachr = {0};            /* escape all control characters */
GLOBAL BOOL canbrk = {0};             /* can handle break */
GLOBAL BOOL canfc32 = {0};            /* can do 32 bit checksum */
GLOBAL BOOL esc8bit = {0};            /* escape characters with 8th bit set */
GLOBAL BOOL usevhdr = {0};            /* use variable length headers */

/* my own recieve capabilities */
GLOBAL BOOL wantfc32 = {TRUE};

int rx32bdat = {0};
long ackfpos = {0};

int lastsnt = {-1};

/*
 * transmit a character ZDLE escaped
 */
VOID txesc(c)
    int c;
    {
    txraw(ZDLE);
    /*
     * exclusive or; not an or so ZDLE becomes ZDLEE
     */
    txraw(c ^ 0x40);
    }

/*
 * transmit a character; ZDLE escaping if appropriate
 */
VOID tx(c)
    unsigned char c;
    {
    switch (c)
        {
    case ZDLE:
        txesc(c);
        return;
    case 0x8d:
    case 0x0d:
        if (escachr && txlast == '@')
            {
            txesc(c);
            return;
            }
        break;
    case 0x10:
    case 0x90:
    case 0x11:
    case 0x91:
    case 0x13:
    case 0x93:
        txesc(c);
        return;
    default:
        if (escachr && (c & 0x60) == 0)
            {
            txesc(c);
            return;
            }
        break;
        }
    /*
     * anything that ends here is so normal we might as well transmit it.
     */
    txraw((int)c);
    }

/*
 * transmit a hex header.
 * these routines use txraw because we're sure that all the
 * characters are not to be escaped.
 */

VOID txnibb(n)
    int n;
    {
    n &= 0x0f;
    if (n < 10)
        n += '0';
    else
        n += 'a' - 10;
    txraw(n);
    }

VOID txhex(h)
    int h;
    {
    txnibb(h >> 4);
    txnibb(h);
    }

VOID txhexhdr(p)
    UTINY *p;
    {
    int i;
    UCOUNT crc;

    txraw(ZPAD);
    txraw(ZPAD);
    txraw(ZDLE);
    if (usevhdr)
        {
        txraw(ZVHEX);
        txhex(HDRLEN);
        }
    else
        {
        txraw(ZHEX);
        }

    /*
     * initialise the crc
     */
    crc = 0;

    /*
     * transmit the header
     */
    for (i = 0; i < HDRLEN; i++)
        {
        txhex(*p);
        crc = UPDCRC16(*p, crc);
        p++;
        }

    /*
     * update the crc as though it were zero
     */
    crc = UPDCRC16(0, crc);
    crc = UPDCRC16(0, crc);

    /*
     * transmit the crc
     */
    txhex(crc >> 8);
    txhex(crc);

    /*
     * end of line sequence
     */
    txraw(0x0d);
    txraw(0x0a);

    txraw(XON);
    txflush();
    }

VOID txb32hd(p)
    unsigned char *p;
    {
    int i;
    unsigned long crc;

    txraw(ZPAD);
    txraw(ZPAD);
    txraw(ZDLE);

    if (usevhdr)
        {
        txraw(ZVBIN32);
        tx(HDRLEN);
        }
    else
        txraw(ZBIN32);

    crc = 0xffffffffL;

    for (i = 0; i < HDRLEN; i++)
        {
        crc = UPDCRC32(*p, crc);
        tx(*p++);
        }

    crc = ~crc;

    tx((unsigned char)(crc & 0xff));
    tx((unsigned char)((crc >> 8) & 0xff));
    tx((unsigned char)((crc >> 16) & 0xff));
    tx((unsigned char)((crc >> 24) & 0xff));
    }

VOID txb16hd(p)
    unsigned char *p;
    {
    int i;
    unsigned int crc;

    txraw(ZPAD);
    txraw(ZPAD);
    txraw(ZDLE);

    if (usevhdr)
        {
        txraw(ZVBIN);
        tx(HDRLEN);
        }
    else
        txraw(ZBIN);

    crc = 0;

    for (i = 0; i < HDRLEN; i++)
        {
        crc = UPDCRC16(*p, crc);
        tx(*p++);
        }

    crc = UPDCRC16(0, crc);
    crc = UPDCRC16(0, crc);

    tx(crc >> 8);
    tx(crc);
    }

/*
 * transmit a header using either hex 16 bit crc or binary 32 bit crc
 * depending on the receivers capabilities
 * we dont bother with variable length headers. I dont really see their
 * advantage and they would clutter the code unneccesarily
 */
VOID txhdr(p)
    unsigned char *p;
    {
    if (canfc32)
        {
        if (wantfc32)
            txb32hd(p);
        else
            txb16hd(p);
        }
    else
        txhexhdr(p);
    }

/*
 * data subpacket transmission
 */

VOID tx32dat(subfrty, p, l)
    int subfrty; /* sub frame type */
    unsigned char *p;
    int l;
    {
    unsigned long crc;

    crc = 0xffffffffl;

    while (l > 0)
        {
        crc = UPDCRC32(*p, crc);
        tx(*p++);
        l--;
        }

    crc = UPDCRC32(subfrty, crc);

    txraw(ZDLE);
    txraw(subfrty);

    crc = ~crc;

    tx((int)(crc) & 0xff);
    tx((int)(crc >> 8) & 0xff);
    tx((int)(crc >> 16) & 0xff);
    tx((int)(crc >> 24) & 0xff);
    }

VOID tx16dat(subfrty, p, l)
    int subfrty; /* sub frame type */
    unsigned char *p;
    int l;
    {
    unsigned short crc;

    crc = 0;

    while (l > 0)
        {
        crc = UPDCRC16(*p, crc);
        tx(*p++);
        l--;
        }

    crc = UPDCRC16(subfrty, crc);

    txraw(ZDLE);
    txraw(subfrty);

    crc = UPDCRC16(0, crc);
    crc = UPDCRC16(0, crc);

    tx(crc >> 8);
    tx(crc);
    }

/*
 * send a data subpacket using crc 16 or crc 32 as desired by the receiver
 */
VOID txdata(subfrty, p, l)
    int subfrty; /* sub frame type */
    unsigned char *p;
    int l;
    {
    if (wantfc32 && canfc32)
        tx32dat(subfrty, p, l);
    else
        tx16dat(subfrty, p, l);

    if (subfrty == ZCRCW)
        txraw(XON);
    txflush();
    }

/*
 * rx; receive a single byte undoing any escaping at the
 * sending site. this bit looks like a mess. sorry for that
 * but there seems to be no other way without incurring a lot
 * of overhead. at least like this the path for a normal character
 * is relatively short.
 */
int rx(timeout)
    int timeout;
    {
    int c;

    /*
     * outer loop for ever so for sure something valid
     * will come in; a timeout will occur or a session abort
     * will be received.
     */

    while (TRUE)
        {

        /*
         * fake do loop so we may continue
         * in case a character should be dropped.
         */

        do  {
            c = rxraw(timeout);
            if (c == TIMEOUT)
                return c;

            switch (c)
                {
            case ZDLE:
                break;
            case 0x11:
            case 0x91:
            case 0x13:
            case 0x93:
                continue;
            default:
                /*
                 * if all control characters should be escaped
                 * and this one wasnt then its spurious and
                 * should be dropped.
                 */
                if (escachr && (c & 0x60) == 0)
                    continue;
                /*
                 * normal character; return it.
                 */
                return (c);
                }
            } while (FALSE);

        /*
         * ZDLE encoded sequence or session abort.
         * (or something illegal; then back timeout the top)
         */

        do  {
            c = rxraw(timeout);

            if (c == 0x11 || c == 0x13 || c == 0x91 || c == 0x93 || c == ZDLE)
                {
                /*
                 * these can be dropped.
                 */
                continue;
                }

            switch (c)
                {
            /*
             * these four are really nasty.
             * for convenience we just change them into
             * special characters by setting a bit outside the
             * first 8. that way they can be recognized and still
             * be processed as characters by the rest of the code.
             */
            case ZCRCE:
            case ZCRCG:
            case ZCRCQ:
            case ZCRCW:
                return (c | ZDLEESC);
            case ZRUB0:
                return 0x7f;
            case ZRUB1:
                return 0xff;
            default:
                if (escachr && (c & 0x60) == 0)
                    {
                    /*
                     * a not escaped control character;
                     * probably something from a network.
                     * just drop it.
                     */
                    continue;
                    }
                /*
                 * legitimate escape sequence.
                 * rebuild the original and return it.
                 */
                if ((c & 0x60) == 0x40)
                    return c ^ 0x40;
                break;
                }
            } while (FALSE);
        }
    return (c);
    }

int rxnibb(timeout)
    int timeout;
    {
    int c;

    c = rx(timeout);
    if (c == TIMEOUT)
        return c;
    if (c > '9')
        {
        if (c < 'a' || c > 'f')
            {
            /*
             * illegal hex; different than expected.
             * we might as well time out.
             */
            return (TIMEOUT);
            }
        c -= 'a' - 10;
        }
    else
        {
        if (c < '0')
            {
            /*
             * illegal hex; different than expected.
             * we might as well time out.
             */
            return (TIMEOUT);
            }
        c -= '0';
        }
    return (c);
    }

int rxhex(timeout)
    int timeout;
    {
    int n1;
    int n0;

    n1 = rxnibb(timeout);
    if (n1 == TIMEOUT)
        return n1;
    n0 = rxnibb(timeout);
    if (n0 == TIMEOUT)
        return n0;
    return ((n1 << 4) | n0);
    }

/*
 * receive routines for each of the six different styles of header.
 * each of these leaves rxdhdlen set timeout 0 if the end result is
 * not a valid header.
 */

VOID rxb16hd(timeout)
    int timeout;
    {
    int c;
    int n;
    unsigned short int crc;
    unsigned short int rxd_crc;

    crc = 0;

    for (n = 0; n < 5; n++)
        {
        c = rx(timeout);
        if (c == TIMEOUT)
            return;
        crc = UPDCRC16(c, crc);
        rxdhdr[n] = c;
        }

    crc = UPDCRC16(0, crc);
    crc = UPDCRC16(0, crc);

    rxd_crc = rx(SMALL_TIMEOUT) << 8;
    rxd_crc |= rx(SMALL_TIMEOUT);

    if (rxd_crc != crc)
        return;

    rxdhdlen = 5;
    }

VOID rxb32hd(to)
    int to;
    {
    int c;
    int n;
    unsigned long crc;
    unsigned long rxd_crc;

    crc = 0xffffffffL;
    for (n = 0; n < 5; n++)
        {
        c = rx(SMALL_TIMEOUT);
        if (c == TIMEOUT)
            return;
        crc = UPDCRC32(c, crc);
        rxdhdr[n] = c;
        }
    crc = ~crc;
    rxd_crc = rx(SMALL_TIMEOUT);
    rxd_crc |= ((long)rx(SMALL_TIMEOUT)) << 8;
    rxd_crc |= ((long)rx(SMALL_TIMEOUT)) << 16;
    rxd_crc |= ((long)rx(SMALL_TIMEOUT)) << 24;
    if (rxd_crc != crc)
        return;
    rxdhdlen = 5;
    }

/*
 * receive any style header
 * if the errors flag is set than whenever an invalid header packet is
 * received INVHDR will be returned. otherwise we wait for a good header
 * also; a flag (rx32bdat) will be set timeout indicate whether data
 * packets following this header will have 16 or 32 bit data attached.
 * variable headers are not implemented.
 */

int rxhdraw(timeout, errors)
    int timeout;
    int errors;
    {
    int c;

    rxdhdlen = 0;

    do  {
        do  {
            c = rxraw(timeout);
            if (c == TIMEOUT)
                return (c);
            } while (c != ZPAD);
        c = rxraw(timeout);
        if (c == TIMEOUT)
            return c;
        if (c == ZPAD)
            {
            c = rxraw(timeout);
            if (c == TIMEOUT)
                return (c);
            }

        /*
         * spurious ZPAD check
         */
        if (c != ZDLE)
            continue;

        /*
         * now read the header style
         */

        c = rx(timeout);
        if (c == TIMEOUT)
            return c;

        switch (c)
            {
        case ZBIN:
            rxb16hdr(timeout);
            rx32bdat = FALSE;
            break;
        case ZHEX:
            rxhexhdr(timeout);
            rx32bdat = FALSE;
            break;
        case ZBIN32:
            rxb32hdr(timeout);
            rx32bdat = TRUE;
            break;
        default:
            /*
             * unrecognized header style
             */
            if (errors)
                return (INVHDR);
            continue;
            }
        if (errors && rxdhdlen == 0)
            return INVHDR;

        } while (rxdhdlen == 0);

    /*
     * this appears timeout have been a valid header.
     * return its type.
     */

    if (rxdhdr[0] == ZDATA)
        {
        ackfpos = rxdhdr[ZP0] | ((long)rxdhdr[ZP1] << 8) |
            ((long)rxdhdr[ZP2] << 16) | ((long)rxdhdr[ZP3] << 24);
        }
    if (rxdhdr[0] == ZFILE)
        ackfpos = 0l;
    return (rxdhdr[0]);
    }

/* Receive any header with timeout in seconds
 */
int rxhdr(tmout)
    int tmout;
    {
    return rxhdraw(tmout, FALSE);
    }

VOID rxhexhd(timeout)
    int timeout;
    {
    int c;
    int i;
    unsigned short int crc = 0;
    unsigned short int rxd_crc;

    for (i = 0; i < 5; i++)
        {
        c = rxhex(timeout);
        if (c == TIMEOUT)
            return;
        crc = UPDCRC16(c, crc);

        rxdhdr[i] = c;
        }

    crc = UPDCRC16(0, crc);

    crc = UPDCRC16(0, crc);

    /*
     * receive the crc
     */

    c = rxhex(timeout);

    if (c == TIMEOUT)
        return;

    rxd_crc = c << 8;

    c = rxhex(timeout);

    if (c == TIMEOUT)
        return;

    rxd_crc |= c;

    if (rxd_crc == crc)
        rxdhdlen = 5;

    /*
     * drop the end of line sequence after a hex header
     */
    c = rx(timeout);
    if (c == CR)
        {
        /*
         * both are expected with CR
         */
        rx(timeout);
        }
    }

