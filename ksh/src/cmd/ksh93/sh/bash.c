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
/*
 * bash specific extensions
 * originally provided by Karsten Fleischer
 */

#include <string.h>

#include "defs.h"
#include "path.h"
#include "io.h"
#include "builtins.h"
#include "name.h"

#ifndef BASH_MAJOR
#   define BASH_MAJOR	"1"
#   define BASH_MINOR	"0"
#   define BASH_PATCH	"0"
#   define BASH_BUILD	"0"
#   define BASH_RELEASE	"experimental"
#endif 
#define BASH_VERSION	BASH_MAJOR "." BASH_MINOR "." BASH_PATCH "(" BASH_BUILD ")-" BASH_RELEASE 


void	sh_applyopts(Shopt_t);

extern const char	bash_pre_rc[];

static const char	e_bash_logout[] = "$HOME/.bash_logout";
static char *login_files[4];

const char sh_bash1[] =
	"[B?Enable brace group expansion. This option is only availabe in bash "
	"compatibility mode. In ksh mode, brace group expansion is always on.]"
	"[P?Do not follow symbolic links, use physical directory structure "
	"instead. Only available in bash compatibility mode.]";
const char sh_bash2[] =
"[l:login?Make the shell act as if it had been invoked as a login shell. "
"Only available if invoked as \bbash\b.]"
"[O]:?[shopt_option?\ashopt_option\a is one of the shell options accepted by "
	"the \bshopt\b builtin. If \ashopt_option\a is present, \b-O\b sets "
	"the value of that option; \b+O\b unsets it. If \ashopt_option\a is "
	"not supplied, the names and values of the shell options accepted by "
	"\bshopt\b are printed on the standard output. If the invocation "
	"option is \b+O\b, the output is displayed in a format that may be "
	"reused as input. Only available if invoked as \bbash\b.]"
"[01:init-file|rcfile]:[file?Execute commands from \afile\a instead of the "
	"standard personal initialization file ~/.bashrc if the shell is "
	"interactive. Only available if invoked as \bbash\b.]"
"[02:noediting?For option compatibility with \bbash\b only. Ignored.]"
"[03:noprofile?Do not read either the system-wide startup file or any of the "
	"personal initialization files. Only available if invoked as \bbash\b.]"
"[04:norc?Do not read and execute the personal initialization file "
	"~/.bashrc if the shell is interactive. Only available if invoked as "
	"\bbash\b.]"
"[05:posix?If invoked as \bbash\b, turn on POSIX compatibility. \bBash\b in "
	"POSIX mode is not the same as \bksh\b.]"
"[06:version?Print version number and exit.]";

const char sh_optshopt[] =
"+[-1c?\n@(#)$Id: shopt (AT&T Labs Research) 2003-02-13 $\n]"
"[-author?Karsten Fleischer <K.Fleischer@omnium.de>]"
USAGE_LICENSE
"[+NAME?shopt - set/unset variables controlling optional shell behavior]"
"[+DESCRIPTION?\bshopt\b sets or unsets variables controlling optional shell "
	"behavior. With no options, or with the \b-p\b option, a list of all "
	"settable options is displayed, with an indication of whether or not "
	"each is set.]"
"[p?Causes output to be displayed in a form that may be reused as input.]"
"[s?Set each \aoptname\a.]"
"[u?Unset each \aoptname\a.]"
"[q?Suppress output (quiet mode). The return status indicates whether the "
	"\aoptname\a is set or unset. If multiple \aoptname\a arguments are "
	"given with \b-q\b, the return status is zero if all \aoptname\as are "
	"enabled; non-zero otherwise.]"
"[o?Restricts the values of \aoptname\a to be those defined for the \b-o\b "
	"option to the set builtin.]"
"[+?If either \b-s\b or \b-u\b is used with no \aoptname\a arguments, the "
	"display is limited to those options which are set or unset.]"
"[+?\bshopt\b supports all bash options. Some settings do not have any effect "
	"or are are always on and cannot be changed."
"[+?The value of \aoptname\a must be one of the following:]{"
		"[+cdable_vars?If set, arguments to the \bcd\b command are "
			"assumed to be names of variables whose values are to "
			"be used if the usual \bcd\b proceeding fails.]"
		"[+cdspell?Currently ignored.]"
		"[+checkhash?Always on.]"
		"[+checkwinsize?Currently ignored.]"
		"[+cmdhist?Always on.]"
		"[+dotglob?If set, include filenames beginning with a \b.\b "
			"in the results of pathname expansion.]"
		"[+execfail?Always on.]"
		"[+expand_aliases?Always on.]"
		"[+extglob?Enable extended pattern matching features.]"
		"[+histappend?Always on.]"
		"[+histreedit?If set and an edit mode is selected, the user "
			"is given the opportunity to re-edit a failed history "
			"substitution.]"
		"[+histverify?If set and an edit mode is selected, the result "
			"of a history substitution will not be executed "
			"immediately but be placed in the edit buffer for "
			"further modifications.]"
		"[+hostcomplete?Currently ignored.]"
		"[+huponexit?Currently ignored.]"
		"[+interactive_comments?Always on.]"
		"[+lithist?Always on.]"
		"[+login_shell?This option is set if the shell is started as "
			"a login shell. The value cannot be changed.]"
		"[+mailwarn?Currently ignored.]"
		"[+no_empty_cmd_completion?Always on.]"
		"[+nocaseglob?Match filenames in a case-insensitive fashion "
			"when performing filename expansion.]"
		"[+nullglob?Allows filename patterns which match no files to "
			"expand to a null string, rather than themselves.]"
		"[+progcomp?Currently ignored.]"
		"[+promptvars?Currently ignored.]"
		"[+restricted_shell?This option is set if the shell is started "
			"as a restricted shell. The value cannot be changed. "
			"It is not reset during execution of startup files, "
			"allowing the startup files to determine whether the "
			"shell is restricted.]"
		"[+shift_verbose?Currently ignored.]"
		"[+sourcepath?If set, the \b.\b builtin uses the value of PATH "
			"to find the directory containing the file supplied "
			"as an argument.]"
		"[+xpg_echo?If set, the \becho\b and \bprint\b builtins "
			"expand backslash-escape sequences.]"
"}"
"\n"
"\n[optname ...]\n"
"\n"
"[+EXIT STATUS?]{"
	"[+?The return status when listing options is zero if all \aoptnames\a "
	"are enabled, non-zero otherwise. When setting or unsetting options, "
	"the return status is zero unless an \aoptname\a is not a valid shell "
	"option.]"
"}"

"[+SEE ALSO?\bset\b(1)]"
;

/* GLOBIGNORE discipline. Turn on SH_DOTGLOB on set, turn off on unset. */

static void put_globignore(register Namval_t* np, const char *val, int flags, Namfun_t *fp)
{
	if(val)
		sh_onoption(SH_DOTGLOB);
	else
		sh_offoption(SH_DOTGLOB);

	nv_putv(np,val,flags,fp);
}

const Namdisc_t SH_GLOBIGNORE_disc  = { sizeof(Namfun_t), put_globignore };

/* FUNCNAME discipline */

struct	funcname
{
	Namfun_t	hdr;
};

static void put_funcname(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	/* bash silently returns with an error when FUNCNAME is set,
	   unsetting FUNCNAME is allowed */
	if(val && !(flags&NV_RDONLY))
		error_info.exit(1);

	nv_putv(np,val,flags,fp);
}

const Namdisc_t SH_FUNCNAME_disc  = { sizeof(struct funcname), put_funcname };

#define	SET_SET		1
#define	SET_UNSET	2
#define	SET_NOARGS	4

/* shopt builtin */

int     b_shopt(int argc,register char *argv[],void *extra)
{
        Shell_t *shp = (Shell_t*)extra;
	int n, ret=0;
	Shopt_t newflags=shp->options, opt;
	int verbose=PRINT_SHOPT|PRINT_ALL|PRINT_NO_HEADER|PRINT_VERBOSE;
	int setflag=0, quietflag=0, oflag=0;
	memset(&opt,0,sizeof(opt));
#ifdef SHOPT_RAWONLY
	on_option(&newflags,SH_VIRAW);
#endif
	while((n = optget(argv,sh_optshopt)))
	{
		switch(n)
		{
		case 'p':
			verbose&=~PRINT_VERBOSE;
			break;
		case 's':
		case 'u':
			setflag|=n=='s'?SET_SET:SET_UNSET;
			if(setflag==(SET_SET|SET_UNSET))
			{
				errormsg(SH_DICT,ERROR_ERROR,"cannot set and unset options simultaneously");
				error_info.errors++;
			}
			break;
		case 'q':
			quietflag=1;
			break;
		case 'o':
			oflag=1;
			verbose&=~PRINT_SHOPT;
			break;
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			continue;
		case '?':
			errormsg(SH_DICT,ERROR_usage(0), "%s", opt_info.arg);
			return(-1);
		}
	}
	if(error_info.errors)
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NIL(char*)));
	argc -= opt_info.index;
	if(argc==0)
	{
		/* no args, -s => mask=current options, -u mask=~(current options)
		   else mask=all bits */
		if(setflag&SET_SET)
			opt=newflags;
		else if(setflag&SET_UNSET)
			for(n=0;n<4;n++)
				opt.v[n]=~newflags.v[n];
		else
			memset(&opt,0xff,sizeof(opt));
		setflag=SET_NOARGS;
	}
	while(argc>0)
	{
		n=sh_lookup(argv[opt_info.index],shtab_options);
		if(n<=0||(setflag 
			&& (is_option(&opt,SH_INTERACTIVE)
			    || is_option(&opt,SH_RESTRICTED)
			    || is_option(&opt,SH_RESTRICTED2)
			    || is_option(&opt,SH_BASH)
			    || is_option(&opt,SH_LOGIN_SHELL)))
			||(oflag&&(n&SH_BASHOPT)))
		{
			errormsg(SH_DICT,ERROR_ERROR, e_option, argv[opt_info.index]);
			error_info.errors++;
			ret=1;
		}
		else
			on_option(&opt,n&0xff);
		opt_info.index++;
		argc--;
	}
	if(setflag&(SET_SET|SET_UNSET))
	{
		if(setflag&SET_SET)
		{
			if(sh_isoption(SH_INTERACTIVE))
				off_option(&opt,SH_NOEXEC);
			if(is_option(&opt,SH_VI)||is_option(&opt,SH_EMACS)||is_option(&opt,SH_GMACS))
			{
				off_option(&newflags,SH_VI);
				off_option(&newflags,SH_EMACS);
				off_option(&newflags,SH_GMACS);
			}
			for(n=0;n<4;n++)
				newflags.v[n] |= opt.v[n];
		}
		else if(setflag&SET_UNSET)
			for(n=0;n<4;n++)
				newflags.v[n] &= ~opt.v[n];
		sh_applyopts(newflags);
		shp->options = newflags;
		if(is_option(&newflags,SH_XTRACE))
			sh_trace(argv,1);
	}
	else if(!(setflag&SET_NOARGS)) /* no -s,-u but args, ret=0 if opt&mask==mask */
	{
		for(n=0;n<4;n++)
			ret+=((newflags.v[n]&opt.v[n])!=opt.v[n]);
	}
	if(!quietflag&&!(setflag&(SET_SET|SET_UNSET)))
		sh_printopts(newflags,verbose,&opt);
	return(ret);
}

/* mode = 0: init, called two times
        before parsing shell args with SH_PREINIT state turned on
	second time after sh_init() is through and with SH_PREINIT state turned off
   mode > 1: re-init
   mode < 0: shutdown
*/

void bash_init(int mode)
{
	Sfio_t		*iop;
	Namval_t	*np;
	int		n=0,xtrace,verbose;
	if(mode>0)
		goto reinit;
	if(mode < 0)
	{
		/* termination code */
		int fdin;
		if(sh_isoption(SH_LOGIN_SHELL) && !sh_isoption(SH_POSIX)
#ifdef PATH_BFPATH
			&& (fdin = path_open(sh_mactry((char*)e_bash_logout),NIL(Pathcomp_t*))) >= 0)
#else
			&& (fdin = path_open(sh_mactry((char*)e_bash_logout),"")) >= 0))
#endif		
		{
			char buff[IOBSIZE+1];
			sh_offstate(SH_INTERACTIVE);
			sh_onstate(SH_PROFILE);
			error_info.id = path_basename(e_bash_logout);
			iop = sfnew(NIL(Sfio_t*),buff,IOBSIZE,fdin,SF_READ);
			sh_eval(iop,0);
		}
		return;	
	}

	if(sh_isstate(SH_PREINIT))
	{	/* pre-init stage */
		if(sh_isoption(SH_RESTRICTED))
			sh_onoption(SH_RESTRICTED2);
		sh_onoption(SH_HISTORY2);
		sh_onoption(SH_INTERACTIVE_COMM);
		sh_onoption(SH_SOURCEPATH);
		sh_onoption(SH_HISTAPPEND);
		sh_onoption(SH_CMDHIST);
		sh_onoption(SH_LITHIST);
		sh_onoption(SH_NOEMPTYCMDCOMPL);
		if(sh.login_sh==2)
			sh_onoption(SH_LOGIN_SHELL);
		if(strcmp(astgetconf("CONFORMANCE",0,0,0),"standard")==0)
			sh_onoption(SH_POSIX);
		if(strcmp(astgetconf("UNIVERSE",0,0,0),"att")==0)
			sh_onoption(SH_XPG_ECHO);
		else
			sh_offoption(SH_XPG_ECHO);
		if(strcmp(astgetconf("PATH_RESOLVE",0,0,0),"physical")==0)
			sh_onoption(SH_PHYSICAL);
		else
			sh_offoption(SH_PHYSICAL);

		/* add builtins */
		sh_addbuiltin("shopt", b_shopt, &sh);

		/* set up some variables needed for --version
		 * needs to go here because --version option is parsed before the init script.
		 */
		if(np=nv_open("HOSTTYPE",sh.var_tree,0))
			nv_putval(np, BASH_HOSTTYPE, NV_NOFREE);
		if(np=nv_open("MACHTYPE",sh.var_tree,0))
			nv_putval(np, BASH_MACHTYPE, NV_NOFREE);
		if(np=nv_open("BASH_VERSION",sh.var_tree,0))
			nv_putval(np, BASH_VERSION, NV_NOFREE);
		if(np=nv_open("BASH_VERSINFO",sh.var_tree,0))
		{
			char *argv[7];
			argv[0] = BASH_MAJOR;
			argv[1] = BASH_MINOR;
			argv[2] = BASH_PATCH;
			argv[3] = BASH_BUILD;
			argv[4] = BASH_RELEASE;
			argv[5] = BASH_MACHTYPE;
			argv[6] = 0;
			nv_setvec(np, 0, 6, argv);
			nv_onattr(np,NV_RDONLY);
		}
		return;
	}

	/* rest of init stage */

	/* restrict BASH_ENV */
	if(np=nv_open("BASH_ENV",sh.var_tree,0))
	{
		const Namdisc_t *dp = nv_discfun(NV_DCRESTRICT);
		Namfun_t *fp = calloc(dp->dsize,1);
		fp->disc = dp;
		nv_disc(np, fp, 0);
	}

	/* open GLOBIGNORE node */
	if(np=nv_open("GLOBIGNORE",sh.var_tree,0))
	{
		const Namdisc_t *dp = &SH_GLOBIGNORE_disc;
		Namfun_t *fp = calloc(dp->dsize,1);
		fp->disc = dp;
		nv_disc(np, fp, 0);
	}

	/* set startup files */
	n=0;
	if(!sh_isoption(SH_NOPROFILE))
	{
		if(!sh_isoption(SH_POSIX))
		{
			login_files[n++] = "$HOME/.bash_profile";
			login_files[n++] = "$HOME/.bash_login";
		}
		login_files[n++] = (char*)e_profile;
	}
	sh.login_files = login_files;
	if(sh_isoption(SH_NORC))
		sh.rcfile=0;
	else if(!sh.rcfile)
		sh.rcfile = strdup(sh_mactry("$HOME/.bashrc"));
reinit:
	xtrace = sh_isoption(SH_XTRACE);
	sh_offoption(SH_XTRACE);
	verbose = sh_isoption(SH_VERBOSE);
	sh_offoption(SH_VERBOSE);
	if(np = nv_open("SHELLOPTS", sh.var_tree, NV_NOADD))
		nv_offattr(np,NV_RDONLY);
	iop = sfopen(NULL, bash_pre_rc, "s");
	sh_eval(iop,0);
	if(xtrace)
		sh_offoption(SH_XTRACE);
	if(verbose)
		sh_offoption(SH_VERBOSE);
}
