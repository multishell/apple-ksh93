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
/*
 * test expression
 * [ expression ]
 *
 *   David Korn
 *   AT&T Labs
 *
 */


#include	"defs.h"
#include	<ctype.h>
#include	<error.h>
#include	<ls.h>
#include	"io.h"
#include	"terminal.h"
#include	"test.h"
#include	"builtins.h"
#include	"FEATURE/externs"
#include	"FEATURE/poll"

#ifndef _lib_setregid
#   undef _lib_setreuid
#endif /* _lib_setregid */

#ifdef _lib_setreuid
    extern int setreuid(uid_t,uid_t);
    extern int setregid(uid_t,uid_t);
#endif /* _lib_setreuid */

#ifdef S_ISSOCK
#   if _pipe_socketpair
#      if _socketpair_shutdown_mode
#         define isapipe(f,p) (test_stat(f,p)>=0&&S_ISFIFO((p)->st_mode)||S_ISSOCK((p)->st_mode)&&(p)->st_ino&&((p)->st_mode&(S_IRUSR|S_IWUSR))!=(S_IRUSR|S_IWUSR))
#      else
#         define isapipe(f,p) (test_stat(f,p)>=0&&S_ISFIFO((p)->st_mode)||S_ISSOCK((p)->st_mode)&&(p)->st_ino)
#      endif
#   else
#      define isapipe(f,p) (test_stat(f,p)>=0&&S_ISFIFO((p)->st_mode))
#   endif
#   define isasock(f,p) (test_stat(f,p)>=0&&S_ISSOCK((p)->st_mode))
#else
#   define isapipe(f,p) (test_stat(f,p)>=0&&S_ISFIFO((p)->st_mode))
#   define isasock(f,p) (0)
#endif

#define	permission(a,f)		(sh_access(a,f)==0)
static time_t	test_time(const char*, const char*);
static int	test_stat(const char*, struct stat*);
static int	test_mode(const char*);

/* single char string compare */
#define c_eq(a,c)	(*a==c && *(a+1)==0)
/* two character string compare */
#define c2_eq(a,c1,c2)	(*a==c1 && *(a+1)==c2 && *(a+2)==0)

struct test
{
        Shell_t *sh;
        int     ap;
        int     ac;
        char    **av;
};

static char *nxtarg(struct test*,int);
static int expr(struct test*,int);
static int e3(struct test*);

int b_test(int argc, char *argv[],void *extra)
{
	struct test tdata;
	register char *cp = argv[0];
	register int not;
	tdata.sh = (Shell_t*)extra;
	tdata.av = argv;
	tdata.ap = 1;
	if(c_eq(cp,'['))
	{
		cp = argv[--argc];
		if(!c_eq(cp, ']'))
			errormsg(SH_DICT,ERROR_exit(2),e_missing,"']'");
	}
	if(argc <= 1)
		return(1);
	cp = argv[1];
	not = c_eq(cp,'!');
	/* posix portion for test */
	switch(argc)
	{
		case 5:
			if(!not)
				break;
			argv++;
			/* fall through */
		case 4:
		{
			register int op = sh_lookup(cp=argv[2],shtab_testops);
			if(op&TEST_BINOP)
				break;
			if(!op)
			{
				if(argc==5)
					break;
				if(not && cp[0]=='-' && cp[2]==0)
					return(test_unop(cp[1],argv[3])!=0);
				else if(argv[1][0]=='-' && argv[1][2]==0)
					return(!test_unop(argv[1][1],cp));
				errormsg(SH_DICT,ERROR_exit(2),e_badop,cp);
			}
			return(test_binop(op,argv[1],argv[3])^(argc!=5));
		}
		case 3:
			if(not)
				return(*argv[2]!=0);
			if(cp[0] != '-' || cp[2] || cp[1]=='?')
			{
				if(cp[0]=='-' && (cp[1]=='-' || cp[1]=='?') &&
					strcmp(argv[2],"--")==0)
				{
					char *av[3];
					av[0] = argv[0];
					av[1] = argv[1];
					av[2] = 0;
					optget(av,sh_opttest);
					errormsg(SH_DICT,ERROR_usage(2), "%s",opt_info.arg);
					return(2);
				}
				break;
			}
			return(!test_unop(cp[1],argv[2]));
		case 2:
			return(*cp==0);
	}
	if(argc==5)
		argv--;
	tdata.ac = argc;
	return(!expr(&tdata,0));
}

/*
 * evaluate a test expression.
 * flag is 0 on outer level
 * flag is 1 when in parenthesis
 * flag is 2 when evaluating -a 
 */
static int expr(struct test *tp,register int flag)
{
	register int r;
	register char *p;
	r = e3(tp);
	while(tp->ap < tp->ac)
	{
		p = nxtarg(tp,0);
		/* check for -o and -a */
		if(flag && c_eq(p,')'))
		{
			tp->ap--;
			break;
		}
		if(*p=='-' && *(p+2)==0)
		{
			if(*++p == 'o')
			{
				if(flag==2)
				{
					tp->ap--;
					break;
				}
				r |= expr(tp,3);
				continue;
			}
			else if(*p == 'a')
			{
				r &= expr(tp,2);
				continue;
			}
		}
		if(flag==0)
			break;
		errormsg(SH_DICT,ERROR_exit(2),e_badsyntax);
	}
	return(r);
}

static char *nxtarg(struct test *tp,int mt)
{
	if(tp->ap >= tp->ac)
	{
		if(mt)
		{
			tp->ap++;
			return(0);
		}
		errormsg(SH_DICT,ERROR_exit(2),e_argument);
	}
	return(tp->av[tp->ap++]);
}


static int e3(struct test *tp)
{
	register char *arg, *cp;
	register int op;
	char *binop;
	arg=nxtarg(tp,0);
	if(c_eq(arg, '!'))
		return(!e3(tp));
	if(c_eq(arg, '('))
	{
		op = expr(tp,1);
		cp = nxtarg(tp,0);
		if(!c_eq(cp, ')'))
			errormsg(SH_DICT,ERROR_exit(2),e_missing,"')'");
		return(op);
	}
	cp = nxtarg(tp,1);
	if(cp!=0 && (c_eq(cp,'=') || c2_eq(cp,'!','=')))
		goto skip;
	if(c2_eq(arg,'-','t'))
	{
		if(cp && isdigit(*cp))
			 return(*(cp+1)?0:tty_check(*cp-'0'));
		else
		{
		/* test -t with no arguments */
			tp->ap--;
			return(tty_check(1));
		}
	}
	if(*arg=='-' && arg[2]==0)
	{
		op = arg[1];
		if(!cp)
		{
			/* for backward compatibility with new flags */
			if(op==0 || !strchr(test_opchars+10,op))
				return(1);
			errormsg(SH_DICT,ERROR_exit(2),e_argument);
		}
		if(strchr(test_opchars,op))
			return(test_unop(op,cp));
	}
	if(!cp)
	{
		tp->ap--;
		return(*arg!=0);
	}
skip:
	op = sh_lookup(binop=cp,shtab_testops);
	if(!(op&TEST_BINOP))
		cp = nxtarg(tp,0);
	if(!op)
		errormsg(SH_DICT,ERROR_exit(2),e_badop,binop);
	if(op==TEST_AND | op==TEST_OR)
		tp->ap--;
	return(test_binop(op,arg,cp));
}

int test_unop(register int op,register const char *arg)
{
	struct stat statb;
	switch(op)
	{
	    case 'r':
		return(permission(arg, R_OK));
	    case 'w':
		return(permission(arg, W_OK));
	    case 'x':
		return(permission(arg, X_OK));
	    case 'V':
#if SHOPT_FS_3D
	    {
		register int offset = staktell();
		if(stat(arg,&statb)<0 || !S_ISREG(statb.st_mode))
			return(0);
		/* add trailing / */
		stakputs(arg);
		stakputc('/');
		stakputc(0);
		arg = (const char*)stakptr(offset);
		stakseek(offset);
		/* FALL THRU */
	    }
#else
		return(0);
#endif /* SHOPT_FS_3D */
	    case 'd':
		return(test_stat(arg,&statb)>=0 && S_ISDIR(statb.st_mode));
	    case 'c':
		return(test_stat(arg,&statb)>=0 && S_ISCHR(statb.st_mode));
	    case 'b':
		return(test_stat(arg,&statb)>=0 && S_ISBLK(statb.st_mode));
	    case 'f':
		return(test_stat(arg,&statb)>=0 && S_ISREG(statb.st_mode));
	    case 'u':
		return(test_mode(arg)&S_ISUID);
	    case 'g':
		return(test_mode(arg)&S_ISGID);
	    case 'k':
#ifdef S_ISVTX
		return(test_mode(arg)&S_ISVTX);
#else
		return(0);
#endif /* S_ISVTX */
#if SHOPT_TEST_L
	    case 'l':
#endif
	    case 'L':
	    case 'h': /* undocumented, and hopefully will disappear */
	    {
		struct stat statb;
		if(*arg==0 || arg[strlen(arg)-1]=='/' || lstat(arg,&statb)<0)
			return(0);
		return(S_ISLNK(statb.st_mode));
	    }

	    case 'C':
#ifdef S_ISCTG
		return(test_stat(arg,&statb)>=0 && S_ISCTG(statb.st_mode));
#else
		return(0);
#endif	/* S_ISCTG */
	    case 'H':
#ifdef S_ISCDF
	    {
		register int offset = staktell();
		if(test_stat(arg,&statb)>=0 && S_ISCDF(statb.st_mode))
			return(1);
		stakputs(arg);
		stakputc('+');
		stakputc(0);
		arg = (const char*)stakptr(offset);
		stakseek(offset);
		return(test_stat(arg,&statb)>=0 && S_ISCDF(statb.st_mode));
	    }
#else
		return(0);
#endif	/* S_ISCDF */

	    case 'S':
		return(isasock(arg,&statb));
	    case 'N':
		return(test_stat(arg,&statb)>=0 && (statb.st_mtime>statb.st_atime));
	    case 'p':
		return(isapipe(arg,&statb));
	    case 'n':
		return(*arg != 0);
	    case 'z':
		return(*arg == 0);
	    case 's':
		sfsync(sfstdout);
	    case 'O':
	    case 'G':
	    {
		struct stat statb;
		if(*arg==0 || test_stat(arg,&statb)<0)
			return(0);
		if(op=='s')
			return(statb.st_size>0);
		else if(op=='O')
			return(statb.st_uid==sh.userid);
		return(statb.st_gid==sh.groupid);
	    }
	    case 'a':
	    case 'e':
		return(permission(arg, F_OK));
	    case 'o':
		op = sh_lookup(arg,shtab_options);
		return(op && sh_isoption(op)!=0);
	    case 't':
		if(isdigit(*arg) && arg[1]==0)
			 return(tty_check(*arg-'0'));
		return(0);
	    default:
	    {
		static char a[3] = "-?";
		a[1]= op;
		errormsg(SH_DICT,ERROR_exit(2),e_badop,a);
		/* NOTREACHED  */
		return(0);
	    }
	}
}

int test_binop(register int op,const char *left,const char *right)
{
	register double lnum,rnum;
	if(op&TEST_ARITH)
	{
		while(*left=='0')
			left++;
		while(*right=='0')
			right++;
		lnum = sh_arith(left);
		rnum = sh_arith(right);
	}
	switch(op)
	{
		/* op must be one of the following values */
		case TEST_AND:
		case TEST_OR:
			return(*left!=0);
		case TEST_PEQ:
			return(strmatch(left, right));
		case TEST_PNE:
			return(!strmatch(left, right));
		case TEST_SGT:
			return(strcoll(left, right)>0);
		case TEST_SLT:
			return(strcoll(left, right)<0);
		case TEST_SEQ:
			return(strcmp(left, right)==0);
		case TEST_SNE:
			return(strcmp(left, right)!=0);
		case TEST_EF:
			return(test_inode(left,right));
		case TEST_NT:
			return(test_time(left,right)>0);
		case TEST_OT:
			return(test_time(left,right)<0);
		case TEST_EQ:
			return(lnum==rnum);
		case TEST_NE:
			return(lnum!=rnum);
		case TEST_GT:
			return(lnum>rnum);
		case TEST_LT:
			return(lnum<rnum);
		case TEST_GE:
			return(lnum>=rnum);
		case TEST_LE:
			return(lnum<=rnum);
	}
	/* NOTREACHED */
	return(0);
}

/*
 * returns the modification time of f1 - modification time of f2
 */

static time_t test_time(const char *file1,const char *file2)
{
	struct stat statb1,statb2;
	int r=test_stat(file2,&statb2);
	if(test_stat(file1,&statb1)<0)
		return(r<0?0:-1);
	if(r<0)
		return(1);
	return(statb1.st_mtime-statb2.st_mtime);
}

/*
 * return true if inode of two files are the same
 */

int test_inode(const char *file1,const char *file2)
{
	struct stat stat1,stat2;
	if(test_stat(file1,&stat1)>=0  && test_stat(file2,&stat2)>=0)
		if(stat1.st_dev == stat2.st_dev && stat1.st_ino == stat2.st_ino)
			return(1);
	return(0);
}


/*
 * This version of access checks against effective uid/gid
 * The static buffer statb is shared with test_mode.
 */

sh_access(register const char *name, register int mode)
{
	struct stat statb;
	if(*name==0)
		return(-1);
	if(strmatch(name,(char*)e_devfdNN))
		return(sh_ioaccess((int)strtol(name+8, (char**)0, 10),mode));
	/* can't use access function for execute permission with root */
	if(mode==X_OK && sh.euserid==0)
		goto skip;
	if(sh.userid==sh.euserid && sh.groupid==sh.egroupid)
		return(access(name,mode));
#ifdef _lib_setreuid
	/* swap the real uid to effective, check access then restore */
	/* first swap real and effective gid, if different */
	if(sh.groupid==sh.euserid || setregid(sh.egroupid,sh.groupid)==0) 
	{
		/* next swap real and effective uid, if needed */
		if(sh.userid==sh.euserid || setreuid(sh.euserid,sh.userid)==0)
		{
			mode = access(name,mode);
			/* restore ids */
			if(sh.userid!=sh.euserid)
				setreuid(sh.userid,sh.euserid);
			if(sh.groupid!=sh.egroupid)
				setregid(sh.groupid,sh.egroupid);
			return(mode);
		}
		else if(sh.groupid!=sh.egroupid)
			setregid(sh.groupid,sh.egroupid);
	}
#endif /* _lib_setreuid */
skip:
	if(test_stat(name, &statb) == 0)
	{
		if(mode == F_OK)
			return(mode);
		else if(sh.euserid == 0)
		{
			if(!S_ISREG(statb.st_mode) || mode!=X_OK)
				return(0);
		    	/* root needs execute permission for someone */
			mode = (S_IXUSR|S_IXGRP|S_IXOTH);
		}
		else if(sh.euserid == statb.st_uid)
			mode <<= 6;
		else if(sh.egroupid == statb.st_gid)
			mode <<= 3;
#ifdef _lib_getgroups
		/* on some systems you can be in several groups */
		else
		{
			static int maxgroups;
			gid_t *groups; 
			register int n;
			if(maxgroups==0)
			{
				/* first time */
				if((maxgroups=getgroups(0,(gid_t*)0)) <= 0)
				{
					/* pre-POSIX system */
					maxgroups=NGROUPS_MAX;
				}
			}
			groups = (gid_t*)stakalloc((maxgroups+1)*sizeof(gid_t));
			n = getgroups(maxgroups,groups);
			while(--n >= 0)
			{
				if(groups[n] == statb.st_gid)
				{
					mode <<= 3;
					break;
				}
			}
		}
#   endif /* _lib_getgroups */
		if(statb.st_mode & mode)
			return(0);
	}
	return(-1);
}

/*
 * Return the mode bits of file <file> 
 * If <file> is null, then the previous stat buffer is used.
 * The mode bits are zero if the file doesn't exist.
 */

static int test_mode(register const char *file)
{
	struct stat statb;
	if(file && (*file==0 || test_stat(file,&statb)<0))
		return(0);
	return(statb.st_mode);
}

/*
 * do an fstat() for /dev/fd/n, otherwise stat()
 */
static int test_stat(const char *name,struct stat *buff)
{
	if(*name==0)
	{
		errno = ENOENT;
		return(-1);
	}
	if(strmatch(name,(char*)e_devfdNN))
		return(fstat((int)strtol(name+8, (char**)0, 10),buff));
	else
		return(stat(name,buff));
}
