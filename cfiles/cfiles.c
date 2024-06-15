/*  cfiles.c - show file information for CP/M
 *
 *
 *  Written for  Whitesmiths C compiler version 2.2
 *  You are free to use, modify, and redistribute
 *  these source code modifications. No warranties given.
 *  Hastily Cobbled Together 2024 by Hans-Ake Lund
 */

#include <std.h>
#include <cpm.h>

/*      flags:
 *      -d        debug flag
 *      -r        sort file names in reverse lexical order
 *      -s        sort file names in lexical order
 *      -v        verbose output
 */
GLOBAL BOOL dflag = {NO};
GLOBAL BOOL rflag = {NO};
GLOBAL BOOL sflag = {NO};
GLOBAL BOOL vflag = {NO};

IMPORT LONG ltell();

#define MAXFNAMES 128

TEXT **pfnms = {0};
TEXT *fnms[MAXFNAMES] = {0};

TEXT *_pname = {"cfiles"};

/* Compare filnames in lexical order
 */
COUNT nmord(i, j, dummy)
    COUNT i;
    COUNT j;
    TEXT *dummy[];
    {
    IMPORT BOOL dflag;
    COUNT retval;

    retval = ordbuf(fnms[i], fnms[j], 15);
    if (rflag)
        retval = -retval;      
    return (retval); 
    }

/* Swap pointers to filenames
 */
VOID nmswap(i, j, dummy)
    COUNT i;
    COUNT j;
    TEXT *dummy[];
    {
    TEXT *tmpswp;

    tmpswp = fnms[i];
    fnms[i] = fnms[j];
    fnms[j] = tmpswp;
    }

/*  Show file information
 */
BOOL main(ac, av)
    BYTES ac;
    TEXT **av;
    {
    IMPORT BOOL dflag;
    IMPORT BOOL sflag;
    IMPORT BOOL vflag;
    COUNT cmdargs;
    COUNT mfiles;
    COUNT nfiles;
    COUNT tfiles;
    COUNT fidx;
    LONG fsize;
    FILE fd;

    getflags(&ac, &av, "d,r,s,v:F <args>", &dflag, &rflag, &sflag, &vflag);
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
    if (0 < nfiles)
        {
        if (sflag || rflag)
            {
            if (dflag)
                putfmt("sorting filenames\n");
            sort(nfiles, &nmord, &nmswap, NULL);
            }
        tfiles = nfiles;
        pfnms = fnms;
        while (0 <= (fd = getbfiles(&nfiles, &pfnms, STDIN, STDERR, 1)))
            {
            if (fd == STDERR)
                error("can't read: ", pfnms[-1]);
            /* show the files */
            if (lseek(fd, 0L, 2) != -1)
                fsize = ltell(fd);
            else
                fsize = -1;
            putfmt("  file %p, size: %l bytes", pfnms[-1], fsize);
            if (vflag)
                putfmt(" (fd=%i)", fd);
            putfmt("\n");
            close(fd);
            }
        }
    else
        error("no files found", NULL);
    putfmt("Listed files: %i\n", tfiles);
    exit(YES);
    }

