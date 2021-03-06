/*******************************************************************
*                                                                  *
*             This software is part of the ast package             *
*                Copyright (c) 1992-2004 AT&T Corp.                *
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
*                                                                  *
*******************************************************************/
#pragma prototyped
/*
 * David Korn
 * Glenn Fowler
 * AT&T Research
 *
 * chmod
 */

static const char usage[] =
"[-?\n@(#)$Id: chmod (AT&T Labs Research) 2002-11-14 $\n]"
USAGE_LICENSE
"[+NAME?chmod - change the access permissions of files]"
"[+DESCRIPTION?\bchmod\b changes the permission of each file "
	"according to mode, which can be either a symbolic representation "
	"of changes to make, or an octal number representing the bit "
	"pattern for the new permissions.]"
"[+?Symbolic mode strings consist of one or more comma separated list "
	"of operations that can be perfomed on the mode. Each operation is of "
	"the form \auser\a \aop\a \aperm\a where \auser\a is zero or more of "
	"the following letters:]{"
	"[+u?User permission bits.]"
	"[+g?Group permission bits.]"
	"[+o?Other permission bits.]"
	"[+a?All permission bits. This is the default if none are specified.]"
	"}"
"[+?The \aperm\a portion consists of zero or more of the following letters:]{"
	"[+r?Read permission.]"
	"[+s?Setuid when \bu\b is selected for \awho\a and setgid when \bg\b "
		"is selected for \awho\a.]"
	"[+w?Write permission.]"
	"[+x?Execute permission for files, search permission for directories.]"
	"[+X?Same as \bx\b except that it is ignored for files that do not "
		"already have at least one \bx\b bit set.]"
	"[+l?Exclusive lock bit on systems that support it. Group execute "
		"must be off.]"
	"[+t?Sticky bit on systems that support it.]"
	"}"
"[+?The \aop\a portion consists of one or more of the following characters:]{"
	"[++?Cause the permission selected to be added to the existing "
		"permissions. | is equivalent to +.]"
	"[+-?Cause the permission selected to be removed to the existing "
		"permissions.]"
	"[+=?Cause the permission to be set to the given permissions.]"
	"[+&?Cause the permission selected to be \aand\aed with the existing "
		"permissions.]"
	"}"
"[+?Symbolic modes with the \auser\a portion omitted are subject to "
	"\bumask\b(2) settings unless the \b=\b \aop\a or the "
	"\b--ignore-umask\b option is specified.]"
"[+?A numeric mode is from one to four octal digits (0-7), "
	"derived by adding up the bits with values 4, 2, and 1. "
	"Any omitted digits are assumed to be leading zeros. The "
	"first digit selects the set user ID (4) and set group ID "
	"(2) and save text image (1) attributes. The second digit "
	"selects permissions for the user who owns the file: read "
	"(4), write (2), and execute (1); the third selects permissions"
	"for other users in the file's group, with the same values; "
	"and the fourth for other users not in the file's group, with "
	"the same values.]"

"[+?For symbolic links, by default, \bchmod\b changes the mode on the file "
	"referenced by the symbolic link, not on the symbolic link itself. "
	"The \b-h\b options can be specified to change the mode of the link. "
	"When traversing directories with \b-R\b, \bchmod\b either follows "
	"symbolic links or does not follow symbolic links, based on the "
	"options \b-H\b, \b-L\b, and \b-P\b. The configuration parameter "
	"\bPATH_RESOLVE\b determines the default behavior if none of these "
	"options is specified.]"

"[+?When the \b-c\b or \b-v\b options are specified, change notifications "
	"are written to standard output using the format, "
	"\bmode of %s changed to %0.4o (%s)\b, with arguments of the "
	"pathname, the numeric mode, and the resulting permission bits as "
	"would be displayed by the \bls\b command.]"

"[+?For backwards compatibility, if an invalid option is given that is a valid "
	"symbolic mode specification, \bchmod\b treats this as a mode "
	"specification rather than as an option specification.]"

"[H:metaphysical?Follow symbolic links for command arguments; otherwise don't "
	"follow symbolic links when traversing directories.]"
"[L:logical|follow?Follow symbolic links when traversing directories.]"
"[P:physical|nofollow?Don't follow symbolic links when traversing directories.]"
"[R:recursive?Change the mode for files in subdirectories recursively.]"
"[c:changes?Describe only files whose permission actually change.]"
"[f:quiet|silent?Do not report files whose permissioins fail to change.]"
"[h:symlink?Change the mode of the symbolic links on systems that "
	"support this.]"
"[i:ignore-umask?Ignore the \bumask\b(2) value in symbolic mode "
	"expressions. This is probably how you expect \bchmod\b to work.]"
"[v:verbose?Describe changed permissions of all files.]"
"\n"
"\nmode file ...\n"
"\n"
"[+EXIT STATUS?]{"
	"[+0?All files changed successfully.]"
	"[+>0?Unable to change mode of one or more files.]"
"}"
"[+SEE ALSO?\bchgrp\b(1), \bchown\b(1), \btw\b(1), \bgetconf\b(1), \bls\b(1), "
	"\bumask\b(2)]"
;


#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide lchmod
#else
#define lchmod		______lchmod
#endif

#include <cmdlib.h>
#include <ls.h>
#include <fts.h>

#include "FEATURE/symlink"

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide lchmod
#else
#undef	lchmod
#endif

#define OPT_FORCE	(1<<2)		/* ignore errors		*/
#define OPT_LCHOWN	(1<<5)		/* lchown			*/

extern int	lchmod(const char*, mode_t);

static struct State_s
{
	int		interrupt;
} state;

int
b_chmod(int argc, char* argv[], void* context)
{
	register int	mode;
	register int	force = 0;
	register int	flags;
	register char*	amode;
	register FTS*	fts;
	register FTSENT*ent;
	char*		last;
	int		(*chmodf)(const char*, mode_t);
	int		notify = 0;
	int		ignore = 0;
#if _lib_lchmod
	int		chlink = 0;
#endif

	if (argc < 0)
	{
		state.interrupt = 1;
		return -1;
	}
	state.interrupt = 0;
	cmdinit(argv, context, ERROR_CATALOG, ERROR_NOTIFY);
	flags = fts_flags() | FTS_TOP | FTS_NOPOSTORDER | FTS_NOSEEDOTDIR;

	/*
	 * NOTE: we diverge from the normal optget boilerplate
	 *	 to allow `chmod -x etc' to fall through
	 */

	for (;;)
	{
		switch (optget(argv, usage))
		{
		case 'f':
			force = 1;
			continue;
		case 'h':
#if _lib_lchmod
			chlink = 1;
#endif
			continue;
		case 'c':
			notify = 1;
			continue;
		case 'v':
			notify = 2;
			continue;
		case 'i':
			ignore = 1;
			continue;
		case 'H':
			flags |= FTS_META|FTS_PHYSICAL;
			continue;
		case 'L':
			flags &= ~(FTS_META|FTS_PHYSICAL);
			continue;
		case 'P':
			flags &= ~FTS_META;
			flags |= FTS_PHYSICAL;
			continue;
		case 'R':
			flags &= ~FTS_TOP;
			continue;
		case '?':
			error(ERROR_usage(2), "%s", opt_info.arg);
			break;
		}
		break;
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if (error_info.errors || argc < 2)
		error(ERROR_usage(2), "%s", optusage(NiL));
	amode = *argv;
	if (ignore)
		ignore = umask(0);
	mode = strperm(amode, &last, 0);
	if (*last)
	{
		if (ignore)
			umask(ignore);
		error(ERROR_exit(1), "%s: invalid mode", amode);
	}
	chmodf =
#if _lib_lchmod
		chlink ? lchmod :
#endif
		chmod;
	if (!(fts = fts_open(argv + 1, flags, NiL)))
	{
		if (ignore)
			umask(ignore);
		error(ERROR_system(1), "%s: not found", argv[1]);
	}
	while (!state.interrupt && (ent = fts_read(fts)))
		switch (ent->fts_info)
		{
		case FTS_F:
		case FTS_D:
		case FTS_SL:
		case FTS_SLNONE:
		anyway:
			if (amode)
				mode = strperm(amode, &last, ent->fts_statp->st_mode);
			if ((*chmodf)(ent->fts_accpath, mode) >= 0 )
			{
				if(notify==2 || (notify==1 && (mode!=(ent->fts_statp->st_mode&S_IPERM))))
					sfprintf(sfstdout,"mode of %s changed to %0.4o (%s)\n" ,ent->fts_accpath,mode,fmtmode(mode,1)+1);
			}
			else if(!force)
				error(ERROR_system(0), "%s: cannot change mode", ent->fts_accpath);
			break;
		case FTS_DC:
			if (!force)
				error(ERROR_warn(0), "%s: directory causes cycle", ent->fts_accpath);
			break;
		case FTS_DNR:
			if (!force)
				error(ERROR_system(0), "%s: cannot read directory", ent->fts_accpath);
			goto anyway;
		case FTS_DNX:
			if (!force)
				error(ERROR_system(0), "%s: cannot search directory", ent->fts_accpath);
			goto anyway;
		case FTS_NS:
			if (!force)
				error(ERROR_system(0), "%s: not found", ent->fts_accpath);
			break;
		}
	fts_close(fts);
	if (ignore)
		umask(ignore);
	return error_info.errors != 0;
}
