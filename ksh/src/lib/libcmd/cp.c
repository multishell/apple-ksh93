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
 * Glenn Fowler
 * AT&T Research
 *
 * cp/ln/mv -- copy/link/move files
 */

static const char usage_head[] =
"[-?@(#)$Id: cp (AT&T Labs Research) 2004-02-29 $\n]"
USAGE_LICENSE
;

static const char usage_cp[] =
"[+NAME?cp - copy files]"
"[+DESCRIPTION?If the last argument names an existing directory, \bcp\b"
"	copies each \afile\a into a file with the same name in that"
"	directory. Otherwise, if only two files are given, \bcp\b copies"
"	the first onto the second. It is an error if the last argument is"
"	not a directory and more than two files are given. By default"
"	directories are not copied.]"

"[a:archive?Preserve as much as possible of the structure and attributes of"
"	the original files in the copy. Equivalent to \b--physical\b"
"	\b--preserve\b \b--recursive\b.]"
"[p:preserve?Preserve file owner, group, permissions and timestamps.]"
"[h:hierarchy|parents?Form the name of each destination file by appending"
"	to the target directory a slash and the specified source file name."
"	The last argument must be an existing directory. Missing destination"
"	directories are created.]"
"[H:metaphysical?Follow command argument symbolic links, otherwise don't"
"	follow.]"
"[l:link?Make hard links to destination files instead of copies.]"
"[L:logical|dereference?Follow symbolic links and copy the files"
"	they point to.]"
"[P|d:physical|nodereference?Don't follow symbolic links; copy symbolic"
"	rather than the files they point to.]"
;

static const char usage_ln[] =
"[+NAME?ln - link files]"
"[+DESCRIPTION?If the last argument names an existing directory, \bln\b"
"	links each \afile\a into a file with the same name in that"
"	directory. Otherwise, if only two files are given, \bln\b links"
"	the first onto the second. It is an error if the last argument is"
"	not a directory and more than two files are given. By default"
"	directories are not linked.]"
;

static const char usage_mv[] =
"[+NAME?mv - rename files]"
"[+DESCRIPTION?If the last argument names an existing directory, \bmv\b"
"	renames each \afile\a into a file with the same name in that"
"	directory. Otherwise, if only two files are given, \bmv\b renames"
"	the first onto the second. It is an error if the last argument is"
"	not a directory and more than two files are given. If a source and"
"	destination file reside on different filesystems then \bmv\b copies"
"	the file contents to the destination and then deletes the source"
"	file.]"
;

static const char usage_tail[] =
"[f:force?Replace existing destination files.]"
"[i:interactive|prompt?Prompt whether to replace existing destination files."
"	An affirmative response (\by\b or \bY\b) replaces the file, a quit"
"	response (\bq\b or \bQ\b) exits immediately, and all other"
"	responses skip the file.]"
"[r|R:recursive?Operate on the contents of directories recursively.]"
"[s:symlink|symbolic-link?Make symbolic links to destination files.]"
"[u:update?Replace a destination file only if its modification time is older"
"	than the corresponding source file modification time.]"
"[v:verbose?Print the name of each file before operating on it.]"
"[b:backup?Make backups of files that are about to be replaced. See"
"	\b--suffix\b and \b--version-control\b for more information.]"
"[F:fsync|sync?\bfsync\b(2) each file after it is copied.]"
"[S:backup-suffix|suffix?A backup file is made by renaming the file to the"
"	same name with the backup suffix appended. The backup suffix is"
"	determined in this order: this option, the \bSIMPLE_BACKUP_SUFFIX\b,"
"	environment variable, or the default value \b~\b.]:[suffix]"
"[V:backup-type|version-control?The backup type is determined in this order:"
"	this option, the \bVERSION_CONTROL\b environment variable, or the"
"	default value \bexisting\b. \atype\a may be one of:]:[type]{"
"		[+numbered|t?Always make numbered backups. The numbered backup"
"			suffix is \b.\aSNS\a, where \aS\a is the"
"			\bbackup-suffix\b and \aN\a is the version number,"
"			starting at 1, incremented with each version.]"
"		[+existing|nil?Make numbered backups of files that already"
"			have them, otherwise simple backups.]"
"		[+simple|never?Always make simple backups.]"
"}"
"[x|X|l:xdev|local|mount|one-file-system?Do not descend into directories in"
"	different filesystems than their parents.]"

"\n"
"\nsource destination\n"
"file ... directory\n"
"\n"

"[+SEE ALSO?\bpax\b(1), \bfsync\b(2), \brename\b(2), \bunlink\b(2),"
"	\bremove\b(3)]"
;

#include <cmd.h>
#include <ls.h>
#include <times.h>
#include <ftwalk.h>
#include <fs3d.h>
#include <hashkey.h>
#include <sfstr.h>
#include <stk.h>

#define PATH_CHUNK	256

#define CP		1
#define LN		2
#define MV		3

#define BAK_replace	0		/* no backup -- just replace	*/
#define BAK_existing	1		/* number if already else simple*/
#define BAK_number	2		/* append .suffix number suffix	*/
#define BAK_simple	3		/* append suffix		*/

static struct				/* program state		*/
{
	int		backup;		/* BAK_* type			*/
	int		directory;	/* destination is directory	*/
	int		flags;		/* FTW_* flags			*/
	int		force;		/* force approval		*/
	int		fs3d;		/* 3d fs enabled		*/
	int		hierarchy;	/* preserve hierarchy		*/
	int		interactive;	/* prompt for approval		*/
	int		interrupt;	/* interrupt -- punt		*/
	int		missmode;	/* default missing dir mode	*/
	int		official;	/* move to next view		*/
	int		op;		/* {CP,LN,MV}			*/
	int		pathsiz;	/* state.path buffer size	*/
	int		perm;		/* permissions to preserve	*/
	int		postsiz;	/* state.path post index	*/
	int		preserve;	/* preserve { id mode time }	*/
	int		recursive;	/* subtrees too			*/
	int		suflen;		/* strlen(state.suffix)		*/
	int		sync;		/* fsync() each file after copy	*/
	int		uid;		/* caller uid			*/
	int		update;		/* replace only if newer	*/
	int		verbose;	/* list each file before op	*/

	int		(*link)(const char*, const char*);	/* link	*/
	int		(*stat)(const char*, struct stat*);	/* stat	*/

	char*		path;		/* to pathname buffer		*/
	char*		opname;		/* state.op message string	*/
	char*		suffix;		/* backup suffix		*/

	Sfio_t*		tmp;		/* tmp string stream		*/

	char		text[PATH_MAX];	/* link text buffer		*/
} state;

static char	dot[2] = { '.' };

/*
 * preserve support
 */

static void
preserve(const char* path, struct stat* ns, struct stat* os)
{
	int	n;

	if (touch(path, os->st_atime, os->st_mtime, 0))
		error(ERROR_SYSTEM|2, "%s: cannot reset directory times", path);
	n = ((ns->st_uid != os->st_uid) << 1) | (ns->st_gid != os->st_gid);
	if (n && chown(state.path, os->st_uid, os->st_gid))
		switch (n)
		{
		case 01:
			error(ERROR_SYSTEM|2, "%s: cannot reset group to %s", path, fmtgid(os->st_gid));
			break;
		case 02:
			error(ERROR_SYSTEM|2, "%s: cannot reset owner to %s", path, fmtuid(os->st_uid));
			break;
		case 03:
			error(ERROR_SYSTEM|2, "%s: cannot reset owner to %s and group to %s", path, fmtuid(os->st_uid), fmtgid(os->st_gid));
			break;
		}
}

/*
 * visit a single file and state.op to the destination
 */

static int
visit(register Ftw_t* ftw)
{
	register char*	base;
	register int	n;
	register int	len;
	int		rm;
	int		rfd;
	int		wfd;
	int		m;
	int		v;
	char*		s;
	char*		e;
	char*		protection;
	Sfio_t*		ip;
	Sfio_t*		op;
	FTS*		fts;
	FTSENT*		ent;
	struct stat	st;

	static int	presiz;

	if (state.interrupt)
		return -1;
	if (ftw->level == 0)
	{
		base = ftw->name;
		len = ftw->namelen;
		if (state.hierarchy)
			presiz = -1;
		else
		{
			presiz = ftw->pathlen;
			while (*base == '.' && *(base + 1) == '/')
				for (base += 2; *base == '/'; base++);
			if (*base == '.' && !*(base + 1))
				presiz--;
			else if (*base)
				presiz -= base - ftw->name;
			base = ftw->name + len;
			while (base > ftw->name && *(base - 1) == '/')
				base--;
			while (base > ftw->name && *(base - 1) != '/')
				base--;
			len -= base - ftw->name;
			if (state.directory)
				presiz -= len + 1;
		}
	}
	else
	{
		base = ftw->path + presiz + 1;
		len = ftw->pathlen - presiz - 1;
	}
	len++;
	if (state.directory)
	{
		if ((state.postsiz + len) > state.pathsiz && !(state.path = newof(state.path, char, state.pathsiz = roundof(state.postsiz + len, PATH_CHUNK), 0)))
			error(3, "out of space");
		if (state.hierarchy && ftw->level == 0 && strchr(base, '/'))
		{
			s = state.path + state.postsiz;
			memcpy(s, base, len);
			while (e = strchr(s, '/'))
			{
				*e = 0;
				if (access(state.path, 0))
				{
					st.st_mode = state.missmode;
					if (s = strrchr(s, '/'))
					{
						*s = 0;
						stat(state.path, &st);
						*s = '/';
					}
					if (mkdir(state.path, st.st_mode & S_IPERM))
					{
						error(ERROR_SYSTEM|2, "%s: cannot create directory -- %s ignored", state.path, ftw->path);
						ftw->status = FTW_SKIP;
						return 0;
					}
				}
				*e++ = '/';
				s = e;
			}
		}
	}
	switch (ftw->info)
	{
	case FTW_DP:
		if (state.preserve && state.op != LN || ftw->level > 0 && (ftw->statb.st_mode & S_IRWXU) != S_IRWXU)
		{
			if (len && ftw->level > 0)
				memcpy(state.path + state.postsiz, base, len);
			else
				state.path[state.postsiz] = 0;
			if (stat(state.path, &st))
				error(ERROR_SYSTEM|2, "%s: cannot stat", state.path);
			else
			{
				if ((ftw->statb.st_mode & S_IPERM) != (st.st_mode & S_IPERM) && chmod(state.path, ftw->statb.st_mode & S_IPERM))
					error(ERROR_SYSTEM|2, "%s: cannot reset directory mode to %s", state.path, fmtmode(st.st_mode & S_IPERM, 0) + 1);
				if (state.preserve)
					preserve(state.path, &st, &ftw->statb);
			}
		}
		return 0;
	case FTW_DNR:
	case FTW_DNX:
	case FTW_D:
		if (!state.recursive)
		{
			ftw->status = FTW_SKIP;
			if (state.op == CP)
				error(1, "%s: directory -- copying as plain file", ftw->path);
			else if (state.link == link && !state.force)
			{
				error(2, "%s: cannot link directory", ftw->path);
				return 0;
			}
		}
		else switch (ftw->info)
		{
		case FTW_DNR:
			error(2, "%s: cannot read directory", ftw->path);
			return 0;
		case FTW_DNX:
			error(2, "%s: cannot search directory", ftw->path);
			ftw->status = FTW_SKIP;
			/*FALLTHROUGH*/
		case FTW_D:
			if (state.directory)
				memcpy(state.path + state.postsiz, base, len);
			if (!(*state.stat)(state.path, &st))
			{
				if (!S_ISDIR(st.st_mode))
				{
					error(2, "%s: not a directory -- %s ignored", state.path, ftw->path);
					return 0;
				}
			}
			else if (mkdir(state.path, (ftw->statb.st_mode & S_IPERM)|(ftw->info == FTW_D ? S_IRWXU : 0)))
			{
				error(ERROR_SYSTEM|2, "%s: cannot create directory -- %s ignored", state.path, ftw->path);
				ftw->status = FTW_SKIP;
			}
			if (!state.directory)
			{
				state.directory = 1;
				state.path[state.postsiz++] = '/';
				presiz--;
			}
			return 0;
		}
		break;
	case FTW_NS:
		if (state.link != pathsetlink)
		{
			error(2, "%s: not found", ftw->path);
			return 0;
		}
		break;
#if 0
	case FTW_SL:
		if (state.op == CP)
		{
			error(2, "%s: cannot copy non-terminal symbolic link", ftw->path);
			return 0;
		}
		break;
#endif
	}
	if (state.directory)
		memcpy(state.path + state.postsiz, base, len);
	if ((*state.stat)(state.path, &st))
		st.st_mode = 0;
	else if (state.update && !S_ISDIR(st.st_mode) && (unsigned long)ftw->statb.st_mtime < (unsigned long)st.st_mtime)
	{
		ftw->status = FTW_SKIP;
		return 0;
	}
	else if (!state.fs3d || !iview(&st))
	{
		/*
		 * target is in top 3d view
		 */

		if (st.st_dev == ftw->statb.st_dev && st.st_ino == ftw->statb.st_ino)
		{
			if (state.op == MV)
			{
				/*
				 * let rename() handle it
				 */

				if (state.verbose)
					sfputr(sfstdout, state.path, '\n');
				goto operate;
			}
			if (!state.official)
				error(2, "%s: identical to %s", state.path, ftw->path);
			return 0;
		}
		if (S_ISDIR(st.st_mode))
		{
			error(2, "%s: cannot %s existing directory", state.path, state.opname);
			return 0;
		}
		if (state.verbose)
			sfputr(sfstdout, state.path, '\n');
		rm = state.op == LN || ftw->info == FTW_SL;
		if (!rm || !state.force)
		{
			if ((n = open(state.path, O_RDWR|O_BINARY)) >= 0)
			{
				close(n);
				if (state.force)
					/* ok */;
				else if (state.interactive)
				{
					if (astquery(2, "%s %s? ", state.opname, state.path))
						return 0;
				}
				else if (state.op == LN)
				{
					error(2, "%s: cannot %s existing file", state.path, state.opname);
					return 0;
				}
			}
			else if (state.force)
				rm = 1;
			else
			{
				protection =
#ifdef ETXTBSY
				    errno == ETXTBSY ? "``running program''" : 
#endif
				    st.st_uid != state.uid ? "``not owner''" :
				    fmtmode(st.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO), 0) + 1;
				if (state.interactive)
				{
					if (astquery(2, "override protection %s for %s? ", protection, state.path))
						return 0;
					rm = 1;
				}
				else if (!rm)
				{
					error(2, "%s: cannot %s %s protection", state.path, state.opname, protection);
					return 0;
				}
			}
		}
		switch (state.backup)
		{
		case BAK_existing:
		case BAK_number:
			v = 0;
			if (s = strrchr(state.path, '/'))
			{
				e = state.path;
				*s++ = 0;
			}
			else
			{
				e = dot;
				s = state.path;
			}
			n = strlen(s);
			if (fts = fts_open((char**)e, FTS_NOCHDIR|FTS_ONEPATH|FTS_PHYSICAL|FTS_NOPOSTORDER|FTS_NOSTAT|FTS_NOSEEDOTDIR, NiL))
			{
				while (ent = fts_read(fts))
				{
					if (strneq(s, ent->fts_name, n) && ent->fts_name[n] == '.' && strneq(ent->fts_name + n + 1, state.suffix, state.suflen) && (m = strtol(ent->fts_name + n + state.suflen + 1, &e, 10)) && streq(e, state.suffix) && m > v)
						v = m;
					if (ent->fts_level)
						fts_set(NiL, ent, FTS_SKIP);
				}
				fts_close(fts);
			}
			if (s != state.path)
				*--s = '/';
			if (v || state.backup == BAK_number)
			{
				sfprintf(state.tmp, "%s.%s%d%s", state.path, state.suffix, v + 1, state.suffix);
				goto backup;
			}
			/*FALLTHROUGH*/
		case BAK_simple:
			sfprintf(state.tmp, "%s%s", state.path, state.suffix);
		backup:
			s = sfstruse(state.tmp);
			if (rename(state.path, s))
			{
				error(ERROR_SYSTEM|2, "%s: cannot backup to %s", state.path, s);
				return 0;
			}
			break;
		default:
			if (rm && remove(state.path))
			{
				error(ERROR_SYSTEM|2, "%s: cannot remove", state.path);
				return 0;
			}
			break;
		}
	}
 operate:
	switch (state.op)
	{
	case MV:
		for (;;)
		{
			if (!rename(ftw->path, state.path))
				return 0;
			if (!rm && st.st_mode && !remove(state.path))
			{
				rm = 1;
				continue;
			}
			if (errno != EXDEV && (rm || S_ISDIR(ftw->statb.st_mode)))
			{
				error(ERROR_SYSTEM|2, "%s: cannot rename to %s", ftw->path, state.path);
				return 0;
			}
			else
				break;
		}
		/*FALLTHROUGH*/
	case CP:
		if (S_ISLNK(ftw->statb.st_mode))
		{
			if ((n = pathgetlink(ftw->path, state.text, sizeof(state.text) - 1)) < 0)
			{
				error(ERROR_SYSTEM|2, "%s: cannot read symbolic link text", ftw->path);
				return 0;
			}
			state.text[n] = 0;
			if (pathsetlink(state.text, state.path))
			{
				error(ERROR_SYSTEM|2, "%s: cannot copy symbolic link to %s", ftw->path, state.path);
				return 0;
			}
		}
		else if (state.op == CP || S_ISREG(ftw->statb.st_mode) || S_ISDIR(ftw->statb.st_mode))
		{
			if (ftw->statb.st_size > 0 && (rfd = open(ftw->path, O_RDONLY|O_BINARY)) < 0)
			{
				error(ERROR_SYSTEM|2, "%s: cannot read", ftw->path);
				return 0;
			}
			else if ((wfd = open(state.path, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, ftw->statb.st_mode & state.perm)) < 0)
			{
				error(ERROR_SYSTEM|2, "%s: cannot write", state.path);
				if (ftw->statb.st_size > 0)
					close(rfd);
				return 0;
			}
			else if (ftw->statb.st_size > 0)
			{
				if (!(ip = sfnew(NiL, NiL, SF_UNBOUND, rfd, SF_READ)))
				{
					error(ERROR_SYSTEM|2, "%s: %s read stream error", ftw->path, state.path);
					close(rfd);
					close(wfd);
				}
				else
				{
					n = 0;
					if (!(op = sfnew(NiL, NiL, SF_UNBOUND, wfd, SF_WRITE)))
					{
						error(ERROR_SYSTEM|2, "%s: %s write stream error", ftw->path, state.path);
						close(wfd);
						sfclose(ip);
					}
					else
					{
						if (sfmove(ip, op, (Sfoff_t)SF_UNBOUND, -1) < 0)
							n |= 3;
						if (!sfeof(ip))
							n |= 1;
						if (sfsync(op) || state.sync && fsync(wfd) || sfclose(op))
							n |= 2;
						if (sfclose(ip))
							n |= 1;
						if (n)
							error(ERROR_SYSTEM|2, "%s: %s %s error", ftw->path, state.path, n == 1 ? ERROR_translate(0, 0, 0, "read") : n == 2 ? ERROR_translate(0, 0, 0, "write") : ERROR_translate(0, 0, 0, "io"));
					}
				}
			}
			else
				close(wfd);
		}
		else if (S_ISBLK(ftw->statb.st_mode) || S_ISCHR(ftw->statb.st_mode) || S_ISFIFO(ftw->statb.st_mode))
		{
			if (mknod(state.path, ftw->statb.st_mode, idevice(&ftw->statb)))
			{
				error(ERROR_SYSTEM|2, "%s: cannot copy special file to %s", ftw->path, state.path);
				return 0;
			}
		}
		else
		{
			error(2, "%s: cannot copy -- unknown file type 0%o", ftw->path, S_ITYPE(ftw->statb.st_mode));
			return 0;
		}
		if (state.preserve)
		{
			if (ftw->info != FTW_SL)
			{
				if (stat(state.path, &st))
					error(ERROR_SYSTEM|2, "%s: cannot stat", state.path);
				else
				{
					if ((ftw->statb.st_mode & state.perm) != (st.st_mode & state.perm) && chmod(state.path, ftw->statb.st_mode & state.perm))
						error(ERROR_SYSTEM|2, "%s: cannot reset mode to %s", state.path, fmtmode(st.st_mode & state.perm, 0) + 1);
					preserve(state.path, &st, &ftw->statb);
				}
			}
			if (state.op == MV && remove(ftw->path))
				error(ERROR_SYSTEM|1, "%s: cannot remove", ftw->path);
		}
		break;
	case LN:
		if ((*state.link)(ftw->path, state.path))
			error(ERROR_SYSTEM|2, "%s: cannot link to %s", ftw->path, state.path);
		break;
	}
	return 0;
}

int
b_cp(int argc, register char** argv, void* context)
{
	register char*	file;
	register char*	s;
	char**		v;
	char*		backup_type;
	const char*	usage;
	int		path_resolve;
	int		standard;
	struct stat	st;

	if (argc < 0)
	{
		state.interrupt = 1;
		return -1;
	}
	memset(&state, 0, sizeof(state));
	cmdinit(argv, context, ERROR_CATALOG, ERROR_NOTIFY);
	backup_type = 0;
	state.flags = FTW_DOT|FTW_MULTIPLE|FTW_TWICE|FTW_NOSEEDOTDIR;
	state.uid = geteuid();
	if (!(state.tmp = sfstropen()))
		error(ERROR_SYSTEM|3, "out of space [tmp string]");
	sfputr(state.tmp, usage_head, -1);
	standard = !strcmp(astconf("CONFORMANCE", NiL, NiL), "standard");
	switch (error_info.id[0])
	{
	case 'c':
	case 'C':
		sfputr(state.tmp, usage_cp, -1);
		state.op = CP;
		state.stat = stat;
		path_resolve = -1;
		break;
	case 'l':
	case 'L':
		sfputr(state.tmp, usage_ln, -1);
		state.op = LN;
		state.flags |= FTW_PHYSICAL;
		state.link = link;
		state.stat = lstat;
		path_resolve = 1;
		break;
	case 'm':
	case 'M':
		sfputr(state.tmp, usage_mv, -1);
		state.op = MV;
		state.flags |= FTW_PHYSICAL;
		state.preserve = 1;
		state.stat = lstat;
		path_resolve = 1;
		break;
	default:
		error(3, "not implemented");
		break;
	}
	sfputr(state.tmp, usage_tail, -1);
	usage = sfstruse(state.tmp);
	state.opname = state.op == CP ? ERROR_translate(0, 0, 0, "overwrite") : ERROR_translate(0, 0, 0, "replace");
	for (;;)
	{
		switch (optget(argv, usage))
		{
		case 'a':
			state.flags |= FTW_PHYSICAL;
			state.preserve = 1;
			state.recursive = 1;
			path_resolve = 1;
			continue;
		case 'b':
			state.backup = 1;
			continue;
		case 'f':
			state.force = 1;
			if (state.op != CP || !standard)
				state.interactive = 0;
			continue;
		case 'h':
			state.hierarchy = 1;
			continue;
		case 'i':
			state.interactive = 1;
			if (state.op != CP || !standard)
				state.force = 0;
			continue;
		case 'l':
			state.op = LN;
			state.link = link;
			state.stat = lstat;
			continue;
		case 'p':
			state.preserve = 1;
			continue;
		case 'r':
			state.recursive = 1;
			if (path_resolve < 0)
				path_resolve = 0;
			continue;
		case 's':
			state.op = LN;
			state.link = pathsetlink;
			state.stat = lstat;
			continue;
		case 'u':
			state.update = 1;
			continue;
		case 'v':
			state.verbose = 1;
			continue;
		case 'x':
			state.flags |= FTW_MOUNT;
			continue;
		case 'F':
#if _lib_fsync
			state.sync = 1;
#else
			error(1, "%s not implemented on this system", opt_info.name);
#endif
			continue;
		case 'H':
			state.flags |= FTW_META|FTW_PHYSICAL;
			path_resolve = 1;
			continue;
		case 'L':
			state.flags &= ~FTW_PHYSICAL;
			path_resolve = 1;
			continue;
		case 'P':
			state.flags &= ~FTW_META;
			state.flags |= FTW_PHYSICAL;
			path_resolve = 1;
			continue;
		case 'R':
			state.recursive = 1;
			state.flags &= ~FTW_META;
			state.flags |= FTW_PHYSICAL;
			path_resolve = 1;
			continue;
		case 'S':
			state.suffix = opt_info.arg;
			continue;
		case 'V':
			backup_type = opt_info.arg;
			continue;
		case '?':
			error(ERROR_USAGE|4, "%s", opt_info.arg);
			continue;
		case ':':
			error(2, "%s", opt_info.arg);
			continue;
		}
		break;
	}
	argc -= opt_info.index + 1;
	argv += opt_info.index;
	if (*argv && streq(*argv, "-") && !streq(*(argv - 1), "--"))
	{
		argc--;
		argv++;
	}
	if (!(v = (char**)stkalloc(stkstd, (argc + 2) * sizeof(char*))))
		error(3, "out of space");
	memcpy(v, argv, (argc + 1) * sizeof(char*));
	argv = v;
	if (!argc && !standard)
	{
		argc++;
		argv[1] = dot;
	}
	if (state.backup)
	{
		if (!(file = backup_type) && !(backup_type = getenv("VERSION_CONTROL")))
			state.backup = BAK_existing;
		else
			switch (strkey(backup_type))
			{
			case HASHKEY6('e','x','i','s','t','i'):
			case HASHKEY5('e','x','i','s','t'):
			case HASHKEY4('e','x','i','s'):
			case HASHKEY3('e','x','i'):
			case HASHKEY2('e','x'):
			case HASHKEY1('e'):
			case HASHKEY3('n','i','l'):
			case HASHKEY2('n','i'):
				state.backup = BAK_existing;
				break;
			case HASHKEY5('n','e','v','e','r'):
			case HASHKEY4('n','e','v','e'):
			case HASHKEY3('n','e','v'):
			case HASHKEY2('n','e'):
			case HASHKEY6('s','i','m','p','l','e'):
			case HASHKEY5('s','i','m','p','l'):
			case HASHKEY4('s','i','m','p'):
			case HASHKEY3('s','i','m'):
			case HASHKEY2('s','i'):
			case HASHKEY1('s'):
				state.backup = BAK_simple;
				break;
			case HASHKEY6('n','u','m','b','e','r'):
			case HASHKEY5('n','u','m','b','e'):
			case HASHKEY4('n','u','m','b'):
			case HASHKEY3('n','u','m'):
			case HASHKEY2('n','u'):
			case HASHKEY1('t'):
				state.backup = BAK_number;
				break;
			default:
				if (file)
					error(2, "%s: unknown backup type", backup_type);
				break;
			}
		if (!state.suffix && !(state.suffix = getenv("SIMPLE_BACKUP_SUFFIX")))
			state.suffix = "~";
		state.suflen = strlen(state.suffix);
	}
	if (argc <= 0 || error_info.errors)
		error(ERROR_USAGE|4, "%s", optusage(NiL));
	if (!path_resolve)
		state.flags |= ftwflags();
	file = argv[argc];
	argv[argc] = 0;
	if (s = strrchr(file, '/'))
	{
		while (*s == '/')
			s++;
		if (!(!*s || *s == '.' && (!*++s || *s == '.' && !*++s)))
			s = 0;
	}
	pathcanon(file, 0);
	if (!(state.directory = !stat(file, &st) && S_ISDIR(st.st_mode)) && argc > 1)
		error(ERROR_USAGE|4, "%s", optusage(NiL));
	if (s && !state.directory)
		error(3, "%s: not a directory", file);
	if ((state.fs3d = fs3d(FS3D_TEST)) && strmatch(file, "...|*/...|.../*"))
		state.official = 1;
	state.postsiz = strlen(file);
	state.pathsiz = roundof(state.postsiz + 2, PATH_CHUNK);
	if (!(state.path = newof(0, char, state.pathsiz, 0)))
		error(3, "out of space");
	memcpy(state.path, file, state.postsiz + 1);
	if (state.directory && state.path[state.postsiz - 1] != '/')
		state.path[state.postsiz++] = '/';
	if (state.hierarchy)
	{
		if (!state.directory)
			error(3, "%s: last argument must be a directory", file);
		state.missmode = st.st_mode;
	}
	state.perm = state.uid ? S_IPERM : (S_IPERM & ~S_ISVTX);
	if (!state.recursive)
		state.flags |= FTW_TOP;
	ftwalk((char*)argv, visit, state.flags, NiL);
	free(state.path);
	return error_info.errors != 0;
}
