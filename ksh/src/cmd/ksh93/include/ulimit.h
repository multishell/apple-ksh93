/*******************************************************************
*                                                                  *
*             This software is part of the ast package             *
*                Copyright (c) 1982-2004 AT&T Corp.                *
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
*                David Korn <dgk@research.att.com>                 *
*                                                                  *
*******************************************************************/
#pragma prototyped
#ifndef _ULIMIT_H
#define _ULIMIT_H 1
/*
 * This is for the ulimit built-in command
 */

#include	"FEATURE/time"
#include	"FEATURE/rlimits"
#if defined(_sys_resource) && defined(_lib_getrlimit)
#   include	<sys/resource.h>
#   if !defined(RLIMIT_FSIZE) && defined(_sys_vlimit)
	/* This handles hp/ux problem */ 
#	include	<sys/vlimit.h>
#	define RLIMIT_FSIZE	(LIM_FSIZE-1)
#	define RLIMIT_DATA	(LIM_DATA-1)
#	define RLIMIT_STACK	(LIM_STACK-1)
#	define RLIMIT_CORE	(LIM_CORE-1)
#	define RLIMIT_CPU	(LIM_CPU-1)
#	ifdef LIM_MAXRSS
#		define RLIMIT_RSS       (LIM_MAXRSS-1)
#	endif /* LIM_MAXRSS */
#   endif
#   undef _lib_ulimit
#else
#   ifdef _sys_vlimit
#	include	<sys/vlimit.h>
#	undef _lib_ulimit
#	define RLIMIT_FSIZE	LIM_FSIZE
#	define RLIMIT_DATA	LIM_DATA
#	define RLIMIT_STACK	LIM_STACK
#	define RLIMIT_CORE	LIM_CORE
#	define RLIMIT_CPU	LIM_CPU
#	ifdef LIM_MAXRSS
#		define RLIMIT_RSS       LIM_MAXRSS
#	endif /* LIM_MAXRSS */
#   else
#	ifdef _lib_ulimit
#	    define vlimit ulimit
#	endif /* _lib_ulimit */
#   endif /* _lib_vlimit */
#endif

#ifdef RLIM_INFINITY
#   define INFINITY	RLIM_INFINITY
#else
#   ifndef INFINITY
#	define INFINITY	((rlim_t)-1L)
#   endif /* INFINITY */
#endif /* RLIM_INFINITY */

#if defined(_lib_getrlimit) || defined(_lib_vlimit) || defined(_lib_ulimit)
#   ifndef RLIMIT_CPU
#	define RLIMIT_CPU	0
#   endif /* !RLIMIT_CPU */
#   ifndef RLIMIT_DATA
#	define RLIMIT_DATA	0
#   endif /* !RLIMIT_DATA */
#   ifndef RLIMIT_RSS
#	define RLIMIT_RSS	0
#   endif /* !RLIMIT_RSS */
#   ifndef RLIMIT_STACK
#	define RLIMIT_STACK	0
#   endif /* !RLIMIT_STACK */
#   ifndef RLIMIT_CORE
#	define RLIMIT_CORE	0
#   endif /* !RLIMIT_CORE */
#   ifndef RLIMIT_VMEM
#	define RLIMIT_VMEM	0
#   endif /* !RLIMIT_VMEM */
#   ifndef RLIMIT_NOFILE
#	define RLIMIT_NOFILE	0
#   endif /* !RLIMIT_NOFILE */
#else
#   define _no_ulimit
#endif
#ifndef _typ_rlim_t
    typedef long rlim_t;
#endif

#if !defined(RLIMIT_NOFILE) && defined(RLIMIT_OFILE)
#define RLIMIT_NOFILE	RLIMIT_OFILE
#endif

#ifndef RLIMIT_UNKNOWN
#define RLIMIT_UNKNOWN	(-9999)
#endif
#ifndef RLIMIT_AS
#define RLIMIT_AS	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_CORE
#define RLIMIT_CORE	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_CPU
#define RLIMIT_CPU	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_DATA
#define RLIMIT_DATA	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_FSIZE
#define RLIMIT_FSIZE	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_LOCKS
#define RLIMIT_LOCKS	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_MEMLOCK
#define RLIMIT_MEMLOCK	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_NOFILE
#define RLIMIT_NOFILE	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_NPROC
#define RLIMIT_NPROC	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_PIPE
#define RLIMIT_PIPE	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_RSS
#define RLIMIT_RSS	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_SBSIZE
#define RLIMIT_SBSIZE	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_STACK
#define RLIMIT_STACK	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_PTHREAD
#define RLIMIT_PTHREAD	RLIMIT_UNKNOWN
#endif
#ifndef RLIMIT_VMEM
#define RLIMIT_VMEM	RLIMIT_UNKNOWN
#endif

#define LIM_COUNT	0
#define LIM_BLOCK	1
#define LIM_BYTE	2
#define LIM_KBYTE	3
#define LIM_SECOND	4

typedef struct Limit_s
{
	const char	name[8];
	const char*	description;
	int		index;
	const char*	conf;
	unsigned char	option;
	unsigned char	type;
} Limit_t;

extern const Limit_t	shtab_limits[];
extern const int	shtab_units[];

extern const char	e_unlimited[];
extern const char*	e_units[];

#endif /* _ULIMIT_H */
