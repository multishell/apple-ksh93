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
 * export [-p] [arg...]
 * readonly [-p] [arg...]
 * typeset [options]  [arg...]
 * alias [-ptx] [arg...]
 * unalias [arg...]
 * builtin [-sd] [-f file] [name...]
 * set [options] [name...]
 * unset [-fnv] [name...]
 *
 *   David Korn
 *   AT&T Labs
 *
 */

#include	"defs.h"
#include	<error.h>
#include	"path.h"
#include	"name.h"
#include	"history.h"
#include	"builtins.h"
#include	"variables.h"
#include	<dlldefs.h>

struct tdata
{
	Shell_t 	*sh;
	Sfio_t  	*outfile;
	char    	*prefix;
	int     	aflag;
	int     	argnum;
	int     	scanmask;
	Dt_t 		*scanroot;
	char    	**argnam;
	Namval_t	*tp;
};


static int	print_namval(Sfio_t*, Namval_t*, int, struct tdata*);
static void	print_attribute(Namval_t*,void*);
static void	print_all(Sfio_t*, Dt_t*, struct tdata*);
static void	print_scan(Sfio_t*, int, Dt_t*, int, struct tdata*t);
static int	b_unall(int, char**, Dt_t*, Shell_t*);
static int	b_common(char**, int, Dt_t*, struct tdata*);
static void	pushname(Namval_t*,void*);
static void(*nullscan)(Namval_t*,void*);

static Namval_t *load_class(const char *name)
{
	errormsg(SH_DICT,ERROR_exit(1),"%s: type not loadable",name);
	return(0);
}

/*
 * Note export and readonly are the same
 */
#if 0
    /* for the dictionary generator */
    int    b_export(int argc,char *argv[],void *extra){}
#endif
int    b_readonly(int argc,char *argv[],void *extra)
{
	register int flag;
	char *command = argv[0];
	struct tdata tdata;
	NOT_USED(argc);
	memset((void*)&tdata,0,sizeof(tdata));
	tdata.sh = (Shell_t*)extra;
	tdata.aflag = '-';
	while((flag = optget(argv,*command=='e'?sh_optexport:sh_optreadonly))) switch(flag)
	{
		case 'p':
			tdata.prefix = command;
			break;
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			errormsg(SH_DICT,ERROR_usage(0), "%s", opt_info.arg);
			return(2);
	}
	if(error_info.errors)
		errormsg(SH_DICT,ERROR_usage(2),optusage(NIL(char*)));
	argv += (opt_info.index-1);
	if(*command=='r')
		flag = (NV_ASSIGN|NV_RDONLY|NV_VARNAME);
#ifdef _ENV_H
	else if(!argv[1])
	{
		char *cp,**env=env_get(tdata.sh->env);
		while(cp = *env++)
		{
			if(tdata.prefix)
				sfputr(sfstdout,tdata.prefix,' ');
			sfprintf(sfstdout,"%s\n",sh_fmtq(cp));
		}
		return(0);
	}
#endif
	else
		flag = (NV_ASSIGN|NV_EXPORT|NV_IDENT);
	return(b_common(argv,flag,tdata.sh->var_tree, &tdata));
	return(0);
}


int    b_alias(int argc,register char *argv[],void *extra)
{
	register unsigned flag = NV_ARRAY|NV_NOSCOPE|NV_ASSIGN;
	register Dt_t *troot;
	register int n;
	struct tdata tdata;
	NOT_USED(argc);
	memset((void*)&tdata,0,sizeof(tdata));
	tdata.sh = (Shell_t*)extra;
	troot = tdata.sh->alias_tree;
	if(*argv[0]=='h')
		flag = NV_TAGGED;
	if(argv[1])
	{
		opt_info.offset = 0;
		opt_info.index = 1;
		*opt_info.option = 0;
		tdata.argnum = 0;
		tdata.aflag = *argv[1];
		while((n = optget(argv,sh_optalias))) switch(n)
		{
		    case 'p':
			tdata.prefix = argv[0];
			break;
		    case 't':
			flag |= NV_TAGGED;
			break;
		    case 'x':
			flag |= NV_EXPORT;
			break;
		    case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		    case '?':
			errormsg(SH_DICT,ERROR_usage(0), "%s", opt_info.arg);
			return(2);
		}
		if(error_info.errors)
			errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NIL(char*)));
		argv += (opt_info.index-1);
		if(flag&NV_TAGGED)
		{
			if(argv[1] && strcmp(argv[1],"-r")==0)
			{
				/* hack to handle hash -r */
				nv_putval(PATHNOD,nv_getval(PATHNOD),NV_RDONLY);
				return(0);
			}
			troot = tdata.sh->track_tree;
		}
	}
	return(b_common(argv,flag,troot,&tdata));
}


#if 0
    /* for the dictionary generator */
    int    b_local(int argc,char *argv[],void *extra){}
#endif
int    b_typeset(int argc,register char *argv[],void *extra)
{
	register int flag = NV_VARNAME|NV_ASSIGN;
	register int n;
	struct tdata tdata;
	Namtype_t *ntp = (Namtype_t*)extra;
	Dt_t *troot;
	int isfloat = 0;
	NOT_USED(argc);
	memset((void*)&tdata,0,sizeof(tdata));
	tdata.sh = ntp->shp;
	tdata.tp = ntp->np;
	troot = tdata.sh->var_tree;
	opt_info.disc = (Optdisc_t*)ntp->optinfof;
	while((n = optget(argv,ntp->optstring)))
	{
		switch(n)
		{
			case 'A':
				flag |= NV_ARRAY;
				break;
			case 'E':
				/* The following is for ksh88 compatibility */
				if(opt_info.offset && !strchr(argv[opt_info.index],'E'))
				{
					tdata.argnum = (int)opt_info.num;
					break;
				}
			case 'F':
				if(!opt_info.arg || (tdata.argnum = opt_info.num) <0)
					tdata.argnum = 10;
				isfloat = 1;
				if(n=='E')
					flag |= NV_EXPNOTE;
				break;
			case 'b':
				flag |= NV_BINARY;
				break;
			case 'n':
				flag &= ~NV_VARNAME;
				flag |= (NV_REF|NV_IDENT);
				break;
			case 'H':
				flag |= NV_HOST;
				break;
			case 'T':
				flag |= NV_TYPE;
				tdata.prefix = opt_info.arg;
				break;
			case 'L':
				if(tdata.argnum==0)
					tdata.argnum = (int)opt_info.num;
				if(tdata.argnum < 0)
					errormsg(SH_DICT,ERROR_exit(1), e_badfield, tdata.argnum);
				flag &= ~NV_RJUST;
				flag |= NV_LJUST;
				break;
			case 'Z':
				flag |= NV_ZFILL;
				/* FALL THRU*/
			case 'R':
				if(tdata.argnum==0)
					tdata.argnum = (int)opt_info.num;
				if(tdata.argnum < 0)
					errormsg(SH_DICT,ERROR_exit(1), e_badfield, tdata.argnum);
				flag &= ~NV_LJUST;
				flag |= NV_RJUST;
				break;
			case 'f':
				flag &= ~(NV_VARNAME|NV_ASSIGN);
				troot = tdata.sh->fun_tree;
				break;
			case 'i':
				if(!opt_info.arg || (tdata.argnum = opt_info.num) <0)
					tdata.argnum = 10;
				flag |= NV_INTEGER;
				break;
			case 'l':
				flag |= NV_UTOL;
				break;
			case 'p':
				tdata.prefix = argv[0];
				continue;
			case 'r':
				flag |= NV_RDONLY;
				break;
			case 't':
				flag |= NV_TAGGED;
				break;
			case 'u':
				flag |= NV_LTOU;
				break;
			case 'x':
				flag &= ~NV_VARNAME;
				flag |= (NV_EXPORT|NV_IDENT);
				break;
			case ':':
				errormsg(SH_DICT,2, "%s", opt_info.arg);
				break;
			case '?':
				errormsg(SH_DICT,ERROR_usage(0), "%s", opt_info.arg);
				opt_info.disc = 0;
				return(2);
		}
		if(tdata.aflag==0)
			tdata.aflag = *opt_info.option;
	}
	argv += opt_info.index;
	opt_info.disc = 0;
	/* handle argument of + and - specially */
	if(*argv && argv[0][1]==0 && (*argv[0]=='+' || *argv[0]=='-'))
		tdata.aflag = *argv[0];
	else
		argv--;
	if((flag&NV_INTEGER) && (flag&(NV_LJUST|NV_RJUST|NV_ZFILL)))
		error_info.errors++;
	if((flag&NV_BINARY) && (flag&(NV_LJUST|NV_UTOL|NV_LTOU)))
		error_info.errors++;
	if(troot==tdata.sh->fun_tree && ((isfloat || flag&~(NV_FUNCT|NV_TAGGED|NV_EXPORT|NV_LTOU))))
		error_info.errors++;
	if(error_info.errors)
		errormsg(SH_DICT,ERROR_usage(2),"%s", optusage(NIL(char*)));
	if(isfloat)
		flag |= NV_INTEGER|NV_DOUBLE;
	if(tdata.sh->fn_depth)
		flag |= NV_NOSCOPE;
	if(flag&NV_TYPE)
	{
		int offset = staktell();
		stakputs(NV_CLASS);
		stakputs(tdata.prefix);
		stakputc(0);
		tdata.tp = nv_open(stakptr(offset),tdata.sh->var_tree,NV_VARNAME|NV_NOARRAY|NV_NOASSIGN);
		stakseek(offset);
		if(!tdata.tp)
			errormsg(SH_DICT,ERROR_exit(1),"%s: unknown type",tdata.prefix);
		flag &= ~NV_TYPE;
	}
	else if(tdata.aflag==0 && ntp->np)
		tdata.aflag = '-';
	return(b_common(argv,flag,troot,&tdata));
}

static int     b_common(char **argv,register int flag,Dt_t *troot,struct tdata *tp)
{
	register char *name;
	char *last = 0;
	int nvflags=(flag&(NV_NOARRAY|NV_NOSCOPE|NV_VARNAME|NV_IDENT|NV_ASSIGN));
	int r=0, ref=0;
	Shell_t *shp =tp->sh;
	flag &= ~(NV_ARRAY|NV_NOSCOPE|NV_VARNAME|NV_IDENT);
	if(argv[1])
	{
		if(flag&NV_REF)
		{
			flag &= ~NV_REF;
			ref=1;
			if(tp->aflag!='-')
				nvflags |= NV_NOREF;
		}
		while(name = *++argv)
		{
			register unsigned newflag;
			register Namval_t *np;
			unsigned curflag;
			if(troot == shp->fun_tree)
			{
				/*
				 *functions can be exported or
				 * traced but not set
				 */
				flag &= ~NV_ASSIGN;
				if(flag&NV_LTOU)
				{
					/* Function names cannot be special builtin */
					if((np=nv_search(name,shp->bltin_tree,0)) && nv_isattr(np,BLT_SPC))
						errormsg(SH_DICT,ERROR_exit(1),e_badfun,name);
					np = nv_open(name,shp->fun_tree,NV_NOARRAY|NV_IDENT|NV_NOSCOPE);
				}
				else
					np = nv_search(name,shp->fun_tree,HASH_NOSCOPE);
				if(np && ((flag&NV_LTOU) || !nv_isnull(np) || nv_isattr(np,NV_LTOU)))
				{
					if(flag==0)
					{
						print_namval(sfstdout,np,tp->aflag=='+',tp);
						continue;
					}
					if(shp->subshell)
						sh_subfork();
					if(tp->aflag=='-')
						nv_onattr(np,flag|NV_FUNCTION);
					else if(tp->aflag=='+')
						nv_offattr(np,flag);
				}
				else
					r++;
				continue;
			}
			np = nv_open(name,troot,nvflags);
			/* tracked alias */
			if(troot==shp->track_tree && tp->aflag=='-')
			{
#ifdef PATH_BFPATH
				path_alias(np,path_absolute(nv_name(np),NIL(Pathcomp_t*)));
#else
				nv_onattr(np,NV_NOALIAS);
				path_alias(np,path_absolute(nv_name(np),NIL(char*)));
#endif
				continue;
			}
			if(flag==NV_ASSIGN && !ref && tp->aflag!='-' && !strchr(name,'='))
			{
				if(troot!=shp->var_tree && (nv_isnull(np) || !print_namval(sfstdout,np,0,tp)))
				{
					sfprintf(sfstderr,sh_translate(e_noalias),name);
					r++;
				}
				continue;
			}
			if(troot==shp->var_tree && (nvflags&NV_ARRAY))
				nv_setarray(np,nv_associative);
			if(tp->tp)
				nv_settype(np,tp->tp,tp->aflag=='-'?0:NV_APPEND);
			curflag = np->nvflag;
			flag &= ~NV_ASSIGN;
			if(last=strchr(name,'='))
				*last = 0;
			if (tp->aflag == '-')
			{
				if((flag&NV_EXPORT) && strchr(name,'.'))
					errormsg(SH_DICT,ERROR_exit(1),e_badexport,name);
#if SHOPT_BSH
				if(flag&NV_EXPORT)
					nv_offattr(np,NV_IMPORT);
#endif /* SHOPT_BSH */
				newflag = curflag;
				if(flag&~NV_NOCHANGE)
					newflag &= NV_NOCHANGE;
				newflag |= flag;
				if (flag & (NV_LJUST|NV_RJUST))
				{
					if(!(flag&NV_RJUST))
						newflag &= ~NV_RJUST;
					
					else if(!(flag&NV_LJUST))
						newflag &= ~NV_LJUST;
				}
				if (flag & NV_UTOL)
					newflag &= ~NV_LTOU;
				else if (flag & NV_LTOU)
					newflag &= ~NV_UTOL;
			}
			else
			{
				if((flag&NV_RDONLY) && (curflag&NV_RDONLY))
					errormsg(SH_DICT,ERROR_exit(1),e_readonly,nv_name(np));
				newflag = curflag & ~flag;
			}
			if (tp->aflag && (tp->argnum>0 || (curflag!=newflag)))
			{
				if(shp->subshell)
					sh_assignok(np,1);
				if(troot!=shp->var_tree)
					nv_setattr(np,newflag&~NV_ASSIGN);
				else
				{
					char *oldname=0;
					if(tp->argnum==1 && newflag==NV_INTEGER && nv_isattr(np,NV_INTEGER))
						tp->argnum = 10;
					/* use reference name for export */
					if((newflag^curflag)&NV_EXPORT)
					{
						oldname = np->nvname;
						np->nvname = name;
					}
					nv_newattr (np, newflag&~NV_ASSIGN,tp->argnum);
					if(oldname)
						np->nvname = oldname;
				}
			}
			if(last)
				*last = '=';
			/* set or unset references */
			if(ref)
			{
				if(tp->aflag=='-')
					nv_setref(np);
				else
					nv_unref(np);
			}
			nv_close(np);
		}
	}
	else
	{
		if(tp->aflag)
		{
			if(troot==shp->fun_tree)
			{
				flag |= NV_FUNCTION;
				tp->prefix = 0;
			}
			else if(troot==shp->var_tree)
				flag |= (nvflags&NV_ARRAY);
			print_scan(sfstdout,flag,troot,tp->aflag=='+',tp);
		}
		else if(troot==shp->alias_tree)
			print_scan(sfstdout,0,troot,0,tp);
		else
			print_all(sfstdout,troot,tp);
		sfsync(sfstdout);
	}
	return(r);
}

typedef int (*Fptr_t)(int, char*[], void*);

#define GROWLIB	4
static void **liblist;

/*
 * This allows external routines to load from the same library */
void **sh_getliblist(void)
{
	return(liblist);
}

/*
 * add change or list built-ins
 * adding builtins requires dlopen() interface
 */
int	b_builtin(int argc,char *argv[],void *extra)
{
	register char *arg=0, *name;
	register int n, r=0, flag=0;
	register Namval_t *np;
	int dlete=0;
	struct tdata tdata;
	static int maxlib, nlib;
	Fptr_t addr;
	void *library=0;
	char *errmsg;
	NOT_USED(argc);
	tdata.sh = (Shell_t*)extra;
	while (n = optget(argv,sh_optbuiltin)) switch (n)
	{
	    case 's':
		flag = BLT_SPC;
		break;
	    case 'd':
		dlete=1;
		break;
	    case 'f':
#if SHOPT_DYNAMIC
		arg = opt_info.arg;
#else
		errormsg(SH_DICT,2, "adding built-ins not supported");
		error_info.errors++;
#endif /* SHOPT_DYNAMIC */
		break;
	    case ':':
		errormsg(SH_DICT,2, "%s", opt_info.arg);
		break;
	    case '?':
		errormsg(SH_DICT,ERROR_usage(2), "%s", opt_info.arg);
		break;
	}
	argv += opt_info.index;
	if(error_info.errors)
		errormsg(SH_DICT,ERROR_usage(2),"%s", optusage(NIL(char*)));
	if(arg || *argv)
	{
		if(sh_isoption(SH_RESTRICTED))
			errormsg(SH_DICT,ERROR_exit(1),e_restricted,argv[-opt_info.index]);
		if(sh_isoption(SH_PFSH))
			errormsg(SH_DICT,ERROR_exit(1),e_pfsh,argv[-opt_info.index]);
		if(tdata.sh->subshell)
			sh_subfork();
	}
	if(arg)
	{
#ifdef _hdr_dlldefs
#if (_AST_VERSION>=20021118)
		if(!(library = dllfind(arg,NIL(char*),RTLD_LAZY,NIL(char*),0)))
#else
		if(!(library = dllfind(arg,NIL(char*),RTLD_LAZY)))
#endif
#else
		if(!(library = dlopen(arg,DL_MODE)))
#endif
		{
			errormsg(SH_DICT,ERROR_exit(0),"%s: %s",arg,dlerror());
			return(1);
		}
		/* 
		 * see if library is already on search list
		 * if it is, move to head of search list
		 */
		for(n=r=0; n < nlib; n++)
		{
			if(r)
				liblist[n-1] = liblist[n];
			else if(liblist[n]==library)
				r++;
		}
		if(r)
			nlib--;
		else
		{
			typedef void (*Iptr_t)(int);
			Iptr_t initfn;
			if((initfn = (Iptr_t)dlllook(library,"lib_init")))
				(*initfn)(0);
		}
		if(nlib >= maxlib)
		{
			/* add library to search list */
			maxlib += GROWLIB;
			if(nlib>0)
				liblist = (void**)realloc((void*)liblist,(maxlib+1)*sizeof(void**));
			else
				liblist = (void**)malloc((maxlib+1)*sizeof(void**));
		}
		liblist[nlib++] = library;
		liblist[nlib] = 0;
	}
	else if(*argv==0 && !dlete)
	{
		print_scan(sfstdout, flag, tdata.sh->bltin_tree, 1, &tdata);
		return(0);
	}
	r = 0;
	flag = staktell();
	while(arg = *argv)
	{
		name = path_basename(arg);
		stakputs("b_");
		stakputs(name);
		errmsg = 0;
		addr = 0;
		for(n=(nlib?nlib:dlete); --n>=0;)
		{
			/* (char*) added for some sgi-mips compilers */ 
			if(dlete || (addr = (Fptr_t)dlllook(liblist[n],stakptr(flag))))
			{
				if(np = sh_addbuiltin(arg, addr,(void*)dlete))
				{
					if(dlete || nv_isattr(np,BLT_SPC))
						errmsg = "restricted name";
				}
				break;
			}
		}
		if(!dlete && !addr)
		{
			np = sh_addbuiltin(arg, 0 ,0);
			if(np && nv_isattr(np,BLT_SPC))
				errmsg = "restricted name";
			else if(!np)
				errmsg = "not found";
		}
		if(errmsg)
		{
			errormsg(SH_DICT,ERROR_exit(0),"%s: %s",*argv,errmsg);
			r = 1;
		}
		stakseek(flag);
		argv++;
	}
	return(r);
}

int    b_set(int argc,register char *argv[],void *extra)
{
	struct tdata tdata;
	tdata.sh = (Shell_t*)extra;
	tdata.prefix=0;
	if(argv[1])
	{
		if(sh_argopts(argc,argv) < 0)
			return(2);
		if(sh_isoption(SH_VERBOSE))
			sh_onstate(SH_VERBOSE);
		else
			sh_offstate(SH_VERBOSE);
		if(sh_isoption(SH_MONITOR))
			sh_onstate(SH_MONITOR);
		else
			sh_offstate(SH_MONITOR);
	}
	else
		/*scan name chain and print*/
		print_scan(sfstdout,0,tdata.sh->var_tree,0,&tdata);
	return(0);
}

/*
 * The removing of Shell variable names, aliases, and functions
 * is performed here.
 * Unset functions with unset -f
 * Non-existent items being deleted give non-zero exit status
 */

int    b_unalias(int argc,register char *argv[],void *extra)
{
	Shell_t *shp = (Shell_t*)extra;
	return(b_unall(argc,argv,shp->alias_tree,shp));
}

int    b_unset(int argc,register char *argv[],void *extra)
{
	Shell_t *shp = (Shell_t*)extra;
	return(b_unall(argc,argv,shp->var_tree,shp));
}

static int b_unall(int argc, char **argv, register Dt_t *troot, Shell_t* shp)
{
	register Namval_t *np;
	register const char *name;
	register int r;
	int nflag=0,all=0,isfun;
	NOT_USED(argc);
	if(troot==shp->alias_tree)
	{
		name = sh_optunalias;
		if(shp->subshell)
			troot = sh_subaliastree(0);
	}
	else
		name = sh_optunset;
	while(r = optget(argv,name)) switch(r)
	{
		case 'f':
			troot = sh_subfuntree(0);
			break;
		case 'a':
			all=1;
			break;
		case 'n':
			nflag = NV_NOREF;
		case 'v':
			troot = shp->var_tree;
			break;
		case ':':
			errormsg(SH_DICT,2, "%s", opt_info.arg);
			break;
		case '?':
			errormsg(SH_DICT,ERROR_usage(0), "%s", opt_info.arg);
			return(2);
	}
	argv += opt_info.index;
	if(error_info.errors || (*argv==0 &&!all))
		errormsg(SH_DICT,ERROR_usage(2),"%s",optusage(NIL(char*)));
	if(!troot)
		return(1);
	r = 0;
	if(troot==shp->var_tree)
		nflag |= NV_VARNAME;
	else
		nflag = NV_NOSCOPE;
	if(all)
		dtclear(troot);
	else while(name = *argv++)
	{
		if(np=nv_open(name,troot,NV_NOADD|nflag))
		{
			if(is_abuiltin(np))
			{
				r = 1;
				continue;
			}
			isfun = is_afunction(np);
			if(shp->subshell && troot==shp->var_tree)
				np=sh_assignok(np,0);
			nv_unset(np);
			nv_close(np);
			if(isfun)
				dtdelete(troot,np);
		}
		else
			r = 1;
	}
	return(r);
}

/*
 * print out the name and value of a name-value pair <np>
 */

static int print_namval(Sfio_t *file,register Namval_t *np,register int flag, struct tdata *tp)
{
	register char *cp;
	sh_sigcheck();
	if(flag)
		flag = '\n';
	if(nv_isattr(np,NV_NOPRINT)==NV_NOPRINT)
	{
		if(is_abuiltin(np))
			sfputr(file,nv_name(np),'\n');
		return(0);
	}
	if(tp->prefix)
		sfputr(file,tp->prefix,' ');
	if(is_afunction(np))
	{
		Sfio_t *iop=0;
		char *fname=0;
		if(!flag && !np->nvalue.ip)
			sfputr(file,"typeset -fu",' ');
		else if(!flag && !nv_isattr(np,NV_FPOSIX))
			sfputr(file,"function",' ');
		sfputr(file,nv_name(np),-1);
		if(nv_isattr(np,NV_FPOSIX))
			sfwrite(file,"()",2);
		if(np->nvalue.ip && np->nvalue.rp->hoffset>=0)
			fname = np->nvalue.rp->fname;
		else
			flag = '\n';
		if(flag)
		{
			if(np->nvalue.ip && np->nvalue.rp->hoffset>=0)
				sfprintf(file," #line %d %s\n",np->nvalue.rp->lineno,fname?sh_fmtq(fname):"");
			else
				sfputc(file, '\n');
		}
		else
		{
			if(nv_isattr(np,NV_FTMP))
			{
				fname = 0;
				iop = tp->sh->heredocs;
			}
			else if(fname)
				iop = sfopen(iop,fname,"r");
			else if(tp->sh->hist_ptr)
				iop = (tp->sh->hist_ptr)->histfp;
			if(iop && sfseek(iop,(Sfoff_t)np->nvalue.rp->hoffset,SEEK_SET)>=0)
				sfmove(iop,file, nv_size(np), -1);
			else
				flag = '\n';
			if(fname)
				sfclose(iop);
		}
		return(nv_size(np)+1);
	}
	if(cp=nv_getval(np))
	{
		sfputr(file,nv_name(np),-1);
		if(!flag)
		{
			flag = '=';
		        if(nv_arrayptr(np))
				sfprintf(file,"[%s]", sh_fmtq(nv_getsub(np)));
		}
		sfputc(file,flag);
		if(flag != '\n')
		{
			if(nv_isref(np) && np->nvenv)
			{
				sfputr(file,sh_fmtq(cp),-1);
				sfprintf(file,"[%s]\n", sh_fmtq(np->nvenv));
			}
			else
				sfputr(file,sh_fmtq(cp),'\n');
		}
		return(1);
	}
	else if(tp->scanmask && tp->scanroot==tp->sh->var_tree)
		sfputr(file,nv_name(np),'\n');
	return(0);
}

/*
 * print attributes at all nodes
 */
static void	print_all(Sfio_t *file,Dt_t *root, struct tdata *tp)
{
	tp->outfile = file;
	nv_scan(root, print_attribute, (void*)tp, 0, 0);
}

/*
 * print the attributes of name value pair give by <np>
 */
static void	print_attribute(register Namval_t *np,void *data)
{
	register struct tdata *dp = (struct tdata*)data;
	nv_attribute(np,dp->outfile,dp->prefix,dp->aflag);
}

/*
 * print the nodes in tree <root> which have attributes <flag> set
 * of <option> is non-zero, no subscript or value is printed.
 */

static void print_scan(Sfio_t *file, int flag, Dt_t *root, int option,struct tdata *tp)
{
	register char **argv;
	register Namval_t *np;
	register int namec;
	Namval_t *onp = 0;
	sh.last_table=0;
	flag &= ~NV_ASSIGN;
	tp->scanmask = flag&~NV_NOSCOPE;
	tp->scanroot = root;
	tp->outfile = file;
	if(flag&NV_INTEGER)
		tp->scanmask |= (NV_DOUBLE|NV_EXPNOTE);
	namec = nv_scan(root,nullscan,(void*)0,tp->scanmask,flag);
	argv = tp->argnam  = (char**)stakalloc((namec+1)*sizeof(char*));
	namec = nv_scan(root, pushname, (void*)tp, tp->scanmask, flag);
	strsort(argv,namec,strcoll);
	while(namec--)
	{
		if((np=nv_search(*argv++,root,0)) && np!=onp && (!nv_isnull(np) || np->nvfun || nv_isattr(np,~NV_NOFREE)))
		{
			onp = np;
			if((flag&NV_ARRAY) && nv_aindex(np)>=0)
				continue;
			if(!flag && nv_isattr(np,NV_ARRAY))
			{
				if(array_elem(nv_arrayptr(np))==0)
					continue;
				nv_putsub(np,NIL(char*),ARRAY_SCAN);
				do
				{
					print_namval(file,np,option,tp);
				}
				while(!option && nv_nextsub(np));
			}
			else
				print_namval(file,np,option,tp);
		}
	}
}

/*
 * add the name of the node to the argument list argnam
 */

static void pushname(Namval_t *np,void *data)
{
	struct tdata *tp = (struct tdata*)data;
	*tp->argnam++ = nv_name(np);
}

