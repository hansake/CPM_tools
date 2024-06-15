/*	CP/M TELL
 *  Created from Whitesmiths lseek.c
 *	copyright (c) 1979 by Whitesmiths, Ltd.
 *  Hastily Cobbled Together 2024 by Hans-Ake Lund
 */
#include <std.h>
#include <cpm.h>

LONG ltell(fd)
	FILE fd;
	{
	FAST FCB *pf;
	FAST WCB *p;

	if (!(p = _ckfd(fd)) || p->dev < 0)
		return (FAIL);
	pf = p->pf;
	return (pf->lseek);
	}

