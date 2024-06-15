/*  czput.c - a Zmodem send program for CP/M
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
#include <cpm.h>
#include "zmodem.h"
#include "czm.h"

#define MAXFNAMES 128

IMPORT LONG ltell();

/*      flags:
 *      -d        debug flag
 *      -o        overwrite file if it exists
 *      -p        protect (don't overwrite file if it exists)
 *      -v        verbose output
 *      -n#       port number
 */
GLOBAL BOOL dflag = {NO};
GLOBAL BOOL oflag = {NO};
GLOBAL BOOL pflag = {NO};
GLOBAL BOOL vflag = {NO};

TEXT **pfnms = {0};
TEXT *fnms[128] = {0};

TEXT *_pname = {"czput"};
LOCAL VOID *(*czlink)() = {NULL};

long curfsiz = {0};  /* Current file size */

int nfilrem = {0}; /* Files remaining to send */

#define TXDASUPSIZE 1024
unsigned char txdasup[TXDASUPSIZE]  = {0};  /* Tx data subpacket */
unsigned char *ptxend = &txdasup[TXDASUPSIZE - 1];

#define MAXASUPSIZE 1024
int subpsize = {MAXASUPSIZE};               /* data subpacket size. may be modified
                                               during a session */

/*
 * send from the current position in the file
 * all the way to end of file or until something goes wrong.
 * (ZNAK or ZRPOS received)
 * the name is only used to show progress
 */
int sndfrom(fname, fd)
    char *fname;
    FILE *fd;
    {
    unsigned long n;
    long fpos;
    int type = ZCRCG;
    unsigned char zdatfrm[5];
    int type2;
    int c;

    /*
     * put the file position in the ZDATA frame
     */
    fpos = ltell(fd);
    zdatfrm[FTYPE] = ZDATA;
    zdatfrm[ZP0] = fpos & 0xff;
    zdatfrm[ZP1] = (fpos >> 8) & 0xff;
    zdatfrm[ZP2] = (fpos >> 16) & 0xff;
    zdatfrm[ZP3] = (fpos >> 24) & 0xff;

    txhdr(zdatfrm);

    /*
     * send the data in the file
     */
    while (ltell(fd) <= curfsiz)
        {
        /*
         * read a block from the file
         */
        n = read(fd, txdasup, subpsize);

        if (n == 0)
            {
            /*
             * nothing to send ?
             */
            break;
            }

        /*
         * at end of file wait for an ACK
         */
        if (ltell(fd) == curfsiz)
            type = ZCRCW;

        txdata(type, txdasup, (int)n);

        if (type == ZCRCW)
            {
            do {
                type2 = rxhdr(10);
                if (type2 == ZNAK || type2 == ZRPOS)
                    return (type2);
               } while (type2 != ZACK);

            if (ltell(fd) == curfsiz)
                {
                if (dflag)
                    putfmt("end of file\\n");
                }
                return (ZACK);
            }

        /*
         * characters from the other side
         * check out that header
         */

        while (rxpoll())
            {
            c = rxraw(0);
            if (c == ZPAD)
                {
                type2 = rxhdr(10);
                if (type2 != TIMEOUT && type2 != ACK)
                    return (type2);
                }
            }
        }

    /*
     * end of file reached.
     * should receive something... so fake ZACK
     */

    return (ZACK);
    }

/*
 * send a file; returns true when session is aborted.
 * (using ZABORT frame)
 */

int sendfile(fd, fname)
    FILE fd;
    char *fname;
    {
    long pos;
    long fsize;
    FILE *fp;
    char *p;
    char *pf;
    unsigned char zfilefrm[5];
    unsigned char zeoffrm[5];
    int type;
    int nchars;
    char *n;
    int tries; /* not in original code */

    if (vflag)
        putfmt("czput: sending file %p\n", fname);

    pos = 0L;
    /* Get file size */
    if (lseek(fd, 0L, 2) != -1)
        fsize = curfsiz = ltell(fd);
    else
        fsize = curfsiz = 0;

    /*
     * now build the ZFILE frame
     */
    zfilefrm[FTYPE] = ZFILE;
    /*
     * set conversion option
     * (not used; always binary)
     */
    zfilefrm[ZF0] = ZF0_ZCBIN;

    /*
     * management option
     */
    zfilefrm[ZF1] = 0;
    if (pflag)
        {
        zfilefrm[ZF1] = ZF1_ZMPROT;
        if (dflag)
            putfmt("czput: protecting destination\n");
        }

    if (oflag)
        {
        zfilefrm[ZF1] = ZF1_ZMCLOB;
        if (dflag)
            putfmt("czput: overwriting destination\n");
        }

    /*
     * transport options
     * (just plain normal transfer)
     */
    zfilefrm[ZF2] = ZF2_ZTNOR;

    /*
     * extended options
     */
    zfilefrm[ZF3] = 0;

    /*
     * now build the data subpacket with the file name and lots of other
     * useful information.
     */
    p = (char *)txdasup;

    /*
     * first enter the name without drive letter
     * and in lower case, finally add a terminating 0
     */

    /* the filename part is CP/M specific
     * and should be moved to a separate source file
     * before this program is ported to Idris and/or MS-DOS
     */
    pf = fname + 2;  /* skip drive letter */

    for (; *pf; p++, pf++)
        {
        /* filename in lower case for CP/M */
        *p = tolower(*pf); 
        }
    *p++ = 0;

    /*
     * next the file size
     */
    nchars = decode(p, ptxend - p, "%l ", fsize);
    p += nchars;

    /*
     * modification date, CP/M 2.2 does not support this
     */
    nchars = decode(p, ptxend - p, "%lo ", 0L);
    p += nchars;

    /*
     * file mode, CP/M 2.2 does not support this
     */
    nchars = decode(p, ptxend - p, "%i ", 0);
    p += nchars;

    /*
     * serial number (??), CP/M 2.2 does not support this
     */
    nchars = decode(p, ptxend - p, "%i ", 0);
    p += nchars;

    /*
     * number of files remaining
     */
    nchars = decode(p, ptxend - p, "%i ", nfilrem);
    p += nchars;

    /*
     * file type, CP/M 2.2 does not support this
     */
    nchars = decode(p, ptxend - p, "%i", 0);
    p += nchars;

    tries = 0;
    do  {
        /*
         * send the header and the data
         */

        txhdr(zfilefrm);
        txdata(ZCRCW, txdasup, p - (char *)txdasup);

        /*
         * wait for anything but an ZACK packet
         */
        do {
            type = rxhdr(10);
           } while (type == ZACK);

        if (type == ZSKIP)
            {
            close(fd);
            if (vflag)
                putfmt("czput: skipped file \"%s\"\n", fname);
            return (FALSE);
            }
        tries++;
        if (10 < tries) /* make maximum 10 tries */
            return (FALSE);

        } while (type != ZRPOS);

    /* transfer_start = time(NULL); does CP/M support time ? */

    do {
        /*
         * fetch pos from the ZRPOS header
         */
        if (type == ZRPOS)
            {
            pos = (long)rxdhdr[ZP0] | ((long)rxdhdr[ZP1] << 8) |
                  ((long)rxdhdr[ZP2] << 16) | ((long)rxdhdr[ZP3] << 24);
            }

        /*
         * seek to the right place in the file
         */
        lseek(fd, pos, 0);

        /*
         * and start sending
         */

        type = sndfrom(n, fd);

        if (type == ZFERR || type == ZABORT)
            {
            /* close(fd); closed in main loop, I hope */
            return (TRUE);
            }

       } while (type == ZRPOS || type == ZNAK);

    /*
     * file sent. send end of file frame
     * and wait for zrinit. if it doesnt come then try again
     */
    zeoffrm[FTYPE] = ZEOF;
    zeoffrm[ZP0] = fsize & 0xff;
    zeoffrm[ZP1] = (fsize >> 8) & 0xff;
    zeoffrm[ZP2] = (fsize >> 16) & 0xff;
    zeoffrm[ZP3] = (fsize >> 24) & 0xff;

    do {
        txhexhdr(zeoffrm);
        type = rxhdr(10);
    } while (type != ZRINIT);

    /*
     * and close the input file
     */
    if (vflag) {
        putfmt("czput: sent file \"%s\"\n", fname);
    }

    /* close(fd); closed in main loop, I hope */

    return (FALSE);
    }

/* Cleanup things at exit
 * (remember not to write anything
 * to STDOUT here as it is closed)
 */
VOID *cleanup()
    {
    czstop();
    return (czlink);
    }

/*  Send files with Zmodem protocol
 */
BOOL main(ac, av)
    BYTES ac;
    TEXT **av;
    {
    IMPORT BOOL dflag;
    IMPORT BOOL oflag;
    IMPORT BOOL pflag;
    IMPORT BOOL vflag;
    IMPORT COUNT portno;
	IMPORT VOID *(*czlink)();
    IMPORT unsigned char rxbuf[1024];
    COUNT cmdargs;
    COUNT mfiles;
    COUNT nfiles;
    COUNT fidx;
    COUNT sntfiles;
    COUNT tries;
    FILE fd;
    METACH inchr;
    TEXT *blkptr;
    TEXT *fnmptr;
    unsigned char zrqihdr[5];
    int rxhtyp;
    int type;
    unsigned char zfinhdr[5];

    long delloop;

    portno = czinit();
    if (!czlink)
        czlink = onexit(&cleanup);
    getflags(&ac, &av, "d,o,p,v,n#:F <args>",
        &dflag, &oflag, &pflag, &vflag, &portno);
    if (dflag)
        {
        putfmt("debug(dflag) = %p, overwrite(oflag) = %p\n",
            dflag?"YES":"NO", oflag?"YES":"NO");
        putfmt("protect(pflag) = %p, verbose(vflag) = %p, portnumber = %i\n",
            pflag?"YES":"NO", vflag?"YES":"NO", portno);
        putfmt("Filenames on command line:\n");
        for (cmdargs = 0; cmdargs < ac; cmdargs++)
            putfmt("  %p\n", av[cmdargs]);
        }
    if (czportok(portno))
        {
        if (vflag)
            {
            putfmt("Sending files using ZMODEM on port %i (%p)\n",
                portno, czname(portno));
            }
        }
    else
        error("invalid port number", NULL);



    /* Set the io device to transparent */
    czstart();
    /* Clear the input queue from any possible garbage
     * this also clears a possible ZRINIT from an already started
     * zmodem receiver. This doesn't harm because we reinvite to
     * receive again below and it may be that the receiver whose
     * ZRINIT we are about to wipe has already died. */
    czpurge();

    /* Establish contact with the receiver */
    tries = 0;
    do
        {
        zrqihdr[0] = ZRQINIT;
        zrqihdr[1] = 0;
        zrqihdr[2] = 0;
        zrqihdr[3] = 0;
        zrqihdr[4] = 0;
        tries++;
        if (tries > 10)
            error("can't establish contact with receiver", NULL);
        txhexhdr(zrqihdr);
        rxhtyp = rxhdr(7);
        } while (rxhtyp != ZRINIT);

    if (vflag)
        {
        putfmt("czput: contact established\n");
        putfmt("czput: starting file transfer\n");
        }

    /*
     * Decode receiver capability flags,
     * don't care about encryption and compression.
     */
    canfudu = (rxdhdr[ZF0] & ZF0_CANFDX) != 0;
    canovio = (rxdhdr[ZF0] & ZF0_CANOVIO) != 0;
    canbrk = (rxdhdr[ZF0] & ZF0_CANBRK) != 0;
    canfc32 = (rxdhdr[ZF0] & ZF0_CANFC32) != 0;
    escachr = (rxdhdr[ZF0] & ZF0_ESCCTL) != 0;
    esc8bit = (rxdhdr[ZF0] & ZF0_ESC8) != 0;
    usevhdr = (rxdhdr[ZF1] & ZF1_CANVHDR) != 0;

    if (dflag)
        {
        putfmt("receiver %p full duplex\n", canfudu ? "can" : "can't");
        putfmt("receiver %p overlap io\n", canovio ? "can" : "can't");
        putfmt("receiver %p break\n", canbrk ? "can" : "can't");
        putfmt("receiver %p fcs 32\n", canfc32 ? "can" : "can't");
        putfmt("receiver %p escaped control chars\n",
                escachr ? "requests" : "doesn't request");
        putfmt("receiver %p escaped 8th bit\n",
                esc8bit ? "requests" : "doesn't request");
        putfmt("receiver %p use variable headers\n", usevhdr ? "can" : "can't");
        }

    /* Send each file in turn */
    /* fix this so that all file names are collected
     * and maybe sorted alphabetically before getbfiles
     * is called, mainly because remaininf files is
     * include in the header
     */
    sntfiles = 0;
    for (nfiles = 0, cmdargs = 0; cmdargs < ac; cmdargs++)
        {
        mfiles = cglob(av[cmdargs], &fnms[nfiles], MAXFNAMES - nfiles);
        nfiles += mfiles;
        if (dflag)
            {
            putfmt("Expanded filenames: %i from: %p\n", mfiles, av[cmdargs]);
            putfmt("Total expanded filenames: %i\n", nfiles);
            }
        }
    if (dflag)
        {
        for (fidx = 0; fidx < nfiles; fidx++)
            putfmt("Filename[%i]: %p\n", fidx, fnms[fidx]);
        }
    nfilrem = nfiles;
    if (0 < nfiles)
        {
        pfnms = fnms;
        while (0 <= (fd = getbfiles(&nfiles, &pfnms, STDIN, STDERR, 1)))
            {
            if (fd == STDERR)
                error("can't read: ", pfnms[-1]);
            if (dflag)
                putfmt("  sending file %p (fd=%i)\n", pfnms[-1], fd);
            /* send the file */
            if (sendfile(fd, pfnms[-1]))
                {
                if (vflag)
                    putfmt("czput: remote aborted.\n");
                break;
                }
            sntfiles++;
            close(fd);
            nfilrem--;
            }
        }

    /*
     * close the session
     */
    if (vflag)
        putfmt("czput: closing the session\\n");

    zfinhdr[0] = ZFIN;
    zfinhdr[1] = 0;
    zfinhdr[2] = 0;
    zfinhdr[3] = 0;
    zfinhdr[4] = 0;

    txhexhdr(zfinhdr);
    do  {
        type = rxhdr(10);
        } while (type != ZFIN && type != TIMEOUT);

    /*
     * these Os are formally required; but they don't do a thing
     * unfortunately many programs require them to exit
     * (both programs already sent a ZFIN so why bother ?)
     */
    if (type != TIMEOUT)
        {
        txraw('O');
        txraw('O');
        }

    if (sntfiles == 0) /* No files were sent */
        error("no files sent", NULL);
    putfmt("\nTransfer complete, %i files sent\n", sntfiles);
    exit(YES);
    }

