/*  CYPUT.C - a Ymodem send program for CP/M
 *  Written for  Whitesmiths C compiler version 2.2
 *  You are free to use, modify, and redistribute
 *  this source code. No warranties given.
 *  Hastily Cobbled Together 2021 by Hans-Ake Lund
 */
#include <std.h>
#include <cpm.h>
#include "ymodem.h"

/*      flags:
 *      -d        debug flag
 *      -k        send 1024 byte blocks instead of 128 byte blocks
 *      -p#       port number
 */
BOOL dflag = {NO};
BOOL kflag = {NO};
COUNT portno = {0};
TEXT **pfnms {0};
TEXT *fnms[128] {0};
TEXT blkbuf[1030] {0};

TEXT *_pname {"cyput"};

/*  Send block over serial link
 */
VOID cputblk(blkbuf, addcrc)
    TEXT *blkbuf;
    BOOL addcrc;
    {
    BYTES blklen;

    if (*blkbuf == SOH)
        blklen = 128 + 4;
    else
        blklen = 1024 + 4;
    if (addcrc)
        blklen++;
    for (; blklen > 0; blklen--)
       cputchr(portno, *blkbuf++); 
    }

/*  Send files with Ymodem protocol over serial link
 */
BOOL main(ac, av)
    BYTES ac;
    TEXT **av;
    {
    IMPORT BOOL dflag;
    IMPORT BOOL kflag;
    IMPORT COUNT portno;
    BOOL addcrc;
    BYTES blkno;
    COUNT blksize;
    COUNT cmdargs;
    COUNT eotsent;
    COUNT readlen;
    COUNT nfiles;
    COUNT sntfiles;
    FILE fd;
    METACH inchr;
    TEXT *blkptr;
    TEXT *fnmptr;

    portno = cginit();
    getflags(&ac, &av, "d,k,p#:F <args>", &dflag, &kflag, &portno);

    if (dflag)
        {
        putfmt("dflag = %p, kflag = %p\n", dflag?"YES":"NO", kflag?"YES":"NO");
        putfmt("Filenames on command line:\n");
        for (cmdargs = 0; cmdargs < ac; cmdargs++)
            putfmt("  %p\n", av[cmdargs]);
        }
    if (kflag)
        blksize = 1024;
    else
        blksize = 128;
    sntfiles = 0;

    if (cgportok(portno))
        {
        putfmt("Sending files using YMODEM on port %i (%p)\n",
            portno, cgname(portno));
        }
    else
        error("invalid port number", NULL);
    for (cmdargs = 0; cmdargs < ac; cmdargs++)
        {
        nfiles = searchf(av[cmdargs], fnms, 128);
        pfnms = fnms;
        while (0 < nfiles)
            {
            fd = getbfiles(&nfiles, &pfnms, STDIN, STDERR, 1);
            if (fd == STDERR)
                error("can't read: ", pfnms[-1]);

            cgstart(portno);
            /* Wait for start request from reciever,
             * NAK: checksum, C: CRC
             */
            while (YES)
                {
                inchr = cgetchr(portno, 60); /* one minute timeout */
                if (inchr == 'C')
                    {
                    addcrc = YES;
                    break;
                    }
                else if (inchr == NAK)
                    {
                    addcrc = NO;
                    break;
                    }
                else if (inchr == -1) /* timeout */
                    {
                    cgstop(portno);
                    error("reciever not ready yet", NULL);
                    }
                }
            /* Put together filename */
            fill(blkbuf, sizeof (blkbuf), 0);
            blkbuf[0] = SOH;
            blkbuf[1] = 0;
            blkbuf[2] = 0xff;
            blkptr = &blkbuf[3];
            fnmptr = pfnms[-1] + 2; /* skip drive letter */
            /* filename in lower case for CP/M */
            while (*fnmptr)
                 {
                 *blkptr++ = tolower(*fnmptr);
                 fnmptr++;
                 }
            crccalc(blkbuf, addcrc);
            cputblk(blkbuf, addcrc);
            /* Wait for <ACK>, <NAK> or timeout */
            while (YES)
               {
               inchr = cgetchr(portno, 60); /* one minute timeout */
               if (inchr == ACK)
                   break;
               else if (inchr == NAK)
                   cputblk(blkbuf, addcrc); /* send the block again */
               else if (inchr == 0x03)
                   {
                   cgstop(portno);
                   error("transfer interupted by Ctrl-C", NULL);
                   }
               else if (inchr == -1) /* timeout */
                   {
                   cgstop(portno);
                   error("reciever not responding to filename: ", pfnms[-1]);
                   }
               }

            /* Send file blocks */
            blkno = 1;
            while (YES)
                {
                /* Create block to send */
                fill(blkbuf, sizeof (blkbuf), CPMEOF);
                readlen = read(fd, &blkbuf[3], blksize);
                if (readlen < 0)
                    error("read error: ", pfnms[-1]);
                else if (readlen == 0)
                    break;
                if (kflag & (128 < readlen))          
                    blkbuf[0] = STX;
                else
                    blkbuf[0] = SOH;
                blkbuf[1] = blkno;
                blkbuf[2] = ~blkno;
                crccalc(blkbuf, addcrc);
                cputblk(blkbuf, addcrc);
                /* Wait for <ACK>, <NAK> or timeout */
                while (YES)
                   {
                   inchr = cgetchr(portno, 60);
                   if (inchr == ACK)
                       {
                       blkno++;
                       break;
                       }
                   else if (inchr == NAK)
                       cputblk(blkbuf, addcrc); /* send the block again */
                   else if (inchr == 0x03)
                       {
                       close(fd);
                       cgstop(portno);
                       error("transfer interupted by Ctrl-C", NULL);
                       }
                   else if (inchr == -1) /* timeout */
                       {
                       cgstop(portno);
                       error("reciever not responding to data block", NULL);
                       }
                   }
                if (128 <= readlen)
                    continue; /* not last block yet */
                else
                    break;
                }
            sntfiles++;
            close(fd);
            /* Last block of this file, send EOT max 10 times */
            for (eotsent = 10; eotsent >= 0; eotsent--)
                {
                cputchr(portno, EOT);
                /* Wait for <ACK>, <NAK> or timeout */
                while (YES)
                    {
                    inchr = cgetchr(portno, 10);
                    if (inchr == ACK)
                        break;
                    else if (inchr == 0x03)
                        {
                        cgstop(portno);
                        error("transfer interupted by Ctrl-C", NULL);
                        }
                    else if ((inchr == NAK) && (inchr == -1))
                        cputchr(portno, EOT);
                    }
                if (inchr == ACK)
                    break;
                }
            if (nfiles == -1)
                break;
            }
        }
    if (sntfiles == 0) /* No files were sent */
        error("no files sent", NULL);
    /* All files are sent
       Wait for start request from reciever again */
    while (YES)
        {
        inchr = cgetchr(portno, 60); /* one minute timeout */
        if (inchr == 'C')
            break;
        else if (inchr == -1) /* timeout */
            {
            cgstop(portno);
            error("reciever not ready for end of transmission", NULL);
            }
        }
    /* Put together empty filename */
    fill(blkbuf, sizeof (blkbuf), 0);
    blkbuf[0] = SOH;
    blkbuf[1] = 0;
    blkbuf[2] = 0xff;
    crccalc(blkbuf, addcrc);
    cputblk(blkbuf, addcrc);
    /* Wait for <ACK>, <NAK> or timeout */
    while (YES)
       {
       inchr = cgetchr(portno, 60); /* one minute timeout */
       if (inchr == ACK)
           break;
       else if (inchr == NAK) /* timeout */
           cputblk(blkbuf, addcrc); /* send the block again */
       else if (inchr == 0x03)
           {
           cgstop(portno);
           error("transfer interupted by Ctrl-C", NULL);
           }
       else if (inchr == -1) /* timeout */
           {
           cgstop(portno);
           error("reciever not responding to transmission end", NULL);
           }
       }
    putfmt("Transfer complete, %i files sent\n", sntfiles);
    exit(YES);
    }
      cgstop(portno);
            error("reciever not ready for end of transmission", NULL);
            }
    