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
#ifndef SH_INTERACTIVE
/*
 * David Korn
 * AT&T Labs
 *
 * Interface definitions for shell command language
 *
 */

#include	<cmd.h>
#include	<cdt.h>
#ifdef _SH_PRIVATE
#   include	"name.h"
#else
#   include	<nval.h>
#endif /* _SH_PRIVATE */

#define SH_VERSION	20030620

/*
 *  The following will disappear in a future release so change all sh_fun
 *  calls to use three arguments and sh_waitnotify specify a function
 *  that will be called with three arguments.  The third argument to
 *  the waitnotify function will 0 for input, 1 for output.
 */
#define sh_waitnotify	_sh_waitnotify

#undef NOT_USED
#define NOT_USED(x)	(&x,1)

/* options */
typedef struct
{
	unsigned long v[4];
}
Shopt_t;

typedef void	(*Shinit_f)(int);
typedef int     (*Shbltin_f)(int, char*[], void*);
typedef int	(*Shwait_f)(int, long, int);

#define SH_CFLAG	0
#define SH_HISTORY	1	/* used also as a state */
#define	SH_ERREXIT	2	/* used also as a state */
#define	SH_VERBOSE	3	/* used also as a state */
#define SH_MONITOR	4	/* used also as a state */
#define	SH_INTERACTIVE	5	/* used also as a state */
#define	SH_RESTRICTED	6
#define	SH_XTRACE	7
#define	SH_KEYWORD	8
#define SH_NOUNSET	9
#define SH_NOGLOB	10
#define SH_ALLEXPORT	11
#define SH_PFSH		12
#define SH_IGNOREEOF	13
#define SH_NOCLOBBER	14
#define SH_MARKDIRS	15
#define SH_BGNICE	16
#define SH_VI		17
#define SH_VIRAW	18
#define	SH_TFLAG	19
#define SH_TRACKALL	20
#define	SH_SFLAG	21
#define	SH_NOEXEC	22
#define SH_GMACS	24
#define SH_EMACS	25
#define SH_PRIVILEGED	26
#define SH_SUBSHARE	27	/* subshell shares state with parent */
#define SH_NOLOG	28
#define SH_NOTIFY	29
#define SH_DICTIONARY	30
#define SH_PIPEFAIL	32
#define SH_GLOBSTARS	33
#define SH_XARGS	34

/*
 * passed as flags to builtins in Nambltin_t struct when BLT_OPTIM is on
 */
#define SH_BEGIN_OPTIM	0x1
#define SH_END_OPTIM	0x2

/* The following type is used for error messages */

/* error messages */
extern const char	e_defpath[];
extern const char	e_found[];
extern const char	e_nospace[];
extern const char	e_format[];
extern const char 	e_number[];
extern const char	e_restricted[];
extern const char	e_recursive[];
extern char		e_version[];

typedef struct sh_scope
{
	struct sh_scope	*par_scope;
	int		argc;
	char		**argv;
	char		*cmdname;
	char		*filename;
	int		lineno;
	Dt_t		*var_tree;
	struct sh_scope	*self;
} Shscope_t;

/*
 * Saves the state of the shell
 */

typedef struct sh_static
{
	Shopt_t		options;	/* set -o options */
	Dt_t		*var_tree;	/* for shell variables */
	Dt_t		*fun_tree;	/* for shell functions */
	Dt_t		*alias_tree;	/* for alias names */
	Dt_t		*bltin_tree;    /* for builtin commands */
	Shscope_t	*topscope;	/* pointer to top-level scope */
	int		inlineno;	/* line number of current input file */
	int		exitval;	/* most recent exit value */
	unsigned char	trapnote;	/* set when trap/signal is pending */
	char		subshell;	/* set for virtual subshell */
#ifdef _SH_PRIVATE
	_SH_PRIVATE
#endif /* _SH_PRIVATE */
} Shell_t;

/* flags for sh_parse */
#define SH_NL		1	/* Treat new-lines as ; */
#define SH_EOF		2	/* EOF causes syntax error */

/* symbolic values for sh_iogetiop */
#define SH_IOCOPROCESS	(-2)
#define SH_IOHISTFILE	(-3)

/* symbolic value for sh_fdnotify */
#define SH_FDCLOSE	(-1)

#if defined(__EXPORT__) && defined(_DLL)
#   ifdef _BLD_shell
#	define extern __EXPORT__
#   endif /* _BLD_shell */
#endif /* _DLL */

extern Dt_t		*sh_bltin_tree(void);
extern void		sh_subfork(void);
extern Shell_t		*sh_init(int,char*[],Shinit_f);
extern int		sh_reinit(char*[]);
extern int 		sh_eval(Sfio_t*,int);
extern void 		sh_delay(double);
extern void		*sh_parse(Shell_t*, Sfio_t*,int);
extern int 		sh_trap(const char*,int);
extern int 		sh_fun(Namval_t*,Namval_t*, char*[]);
extern int 		sh_funscope(int,char*[],int(*)(void*),void*,int);
extern Sfio_t		*sh_iogetiop(int,int);
extern int		sh_main(int, char*[], void(*)(int));
extern void		sh_menu(Sfio_t*, int, char*[]);
extern Namval_t		*sh_addbuiltin(const char*, int(*)(int, char*[],void*), void*);
extern char		*sh_fmtq(const char*);
extern Sfdouble_t	sh_strnum(const char*, char**, int);
extern int		sh_access(const char*,int);
extern int 		sh_close(int);
extern int 		sh_dup(int);
extern void 		sh_exit(int);
extern int		sh_fcntl(int, int, ...);
extern int		(*sh_fdnotify(int(*)(int,int)))(int,int);
extern Shell_t		*sh_getinterp(void);
extern int		sh_open(const char*, int, ...);
extern int		sh_openmax(void);
extern Sfio_t		*sh_pathopen(const char*);
extern ssize_t 		sh_read(int, void*, size_t);
extern ssize_t 		sh_write(int, const void*, size_t);
extern off_t		sh_seek(int, off_t, int);
extern int 		sh_pipe(int[]);
extern void		*sh_waitnotify(Shwait_f);
extern Shscope_t	*sh_getscope(int,int);
extern Shscope_t	*sh_setscope(Shscope_t*);
extern void		sh_sigcheck(void);
extern unsigned long	sh_isoption(int);
extern unsigned long	sh_onoption(int);
extern unsigned long	sh_offoption(int);
extern int 		sh_waitsafe(void);



#if SHOPT_DYNAMIC
    extern void		**sh_getliblist(void);
#endif /* SHOPT_DYNAMIC */

/*
 * direct access to sh is obsolete, use sh_getinterp() instead
 */
#if !defined(_SH_PRIVATE) && defined(__IMPORT__) && !defined(_BLD_shell)
	extern __IMPORT__  Shell_t sh;
#else
	extern Shell_t sh;
#endif

#ifdef _DLL
#   undef extern
#endif /* _DLL */

#ifndef _SH_PRIVATE
#   define access(a,b)	sh_access(a,b)
#   define close(a)	sh_close(a)
#   define exit(a)	sh_exit(a)
#   define fcntl(a,b,c)	sh_fcntl(a,b,c)
#   define pipe(a)	sh_pipe(a)
#   define read(a,b,c)	sh_read(a,b,c)
#   define write(a,b,c)	sh_write(a,b,c)
#   define dup		sh_dup
#   if _lib_lseek64
#	define open64	sh_open
#	define lseek64	sh_seek
#   else
#	define open	sh_open
#	define lseek	sh_seek
#   endif
#endif /* !_SH_PRIVATE */

#define SH_SIGSET	4
#define SH_EXITSIG	0400	/* signal exit bit */
#define SH_EXITMASK	(SH_EXITSIG-1)	/* normal exit status bits */
#define SH_RUNPROG	-1022	/* needs to be negative and < 256 */

#endif /* SH_INTERACTIVE */
