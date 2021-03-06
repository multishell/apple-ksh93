/*******************************************************************
*                                                                  *
*             This software is part of the ast package             *
*                Copyright (c) 1985-2004 AT&T Corp.                *
*        and it may only be used by you under license from         *
*                       AT&T Corp. ("AT&T")                        *
*         A copy of the Source Code Agreement is available         *
*                at the AT&T Internet web site URL                 *
*                                                                  *
*       http://www.research.att.com/sw/license/ast-open.html       *
*                                                                  *
*    If you have copied or used this software without agreeing     *
*        to the terms of the license you are infringing on         *
*           the license and copyright and are violating            *
*               AT&T's intellectual property rights.               *
*                                                                  *
*            Information and Software Systems Research             *
*                        AT&T Labs Research                        *
*                         Florham Park NJ                          *
*                                                                  *
*               Glenn Fowler <gsf@research.att.com>                *
*                David Korn <dgk@research.att.com>                 *
*                 Phong Vo <kpv@research.att.com>                  *
*                                                                  *
*******************************************************************/
#include "FEATURE/uwin"

#if !_UWIN || _lib_getpass

void _STUB_getpass(){}

#else

#pragma prototyped

#define getpass	______getpass

#include	<ast.h>
#include	<termios.h>
#include	<signal.h>

#undef	getpass

#if defined(__EXPORT__)
#define extern		__EXPORT__
#endif

static int interrupt;
static void handler(int sig)
{
	interrupt++;
}

extern char*	getpass(const char *prompt)
{
	struct termios told,tnew;
	Sfio_t *iop;
	static char *cp, passwd[32];
	void (*savesig)(int);
	if(!(iop = sfopen((Sfio_t*)0, "/dev/tty", "r")))
		return(0);
	if(tcgetattr(sffileno(iop),&told) < 0)
		return(0);
	interrupt = 0;
	tnew = told;
	tnew.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL);
	if(tcsetattr(sffileno(iop),TCSANOW,&tnew) < 0)
		return(0);
	savesig = signal(SIGINT, handler);
	sfputr(sfstderr,prompt,-1);
	if(cp = sfgetr(iop,'\n',1))
		strncpy(passwd,cp,sizeof(passwd)-1);
	tcsetattr(sffileno(iop),TCSANOW,&told);
	sfputc(sfstderr,'\n');
	sfclose(iop);
	signal(SIGINT, savesig);
	if(interrupt)
		kill(getpid(),SIGINT);
	return(cp?passwd:0);
}


#endif
