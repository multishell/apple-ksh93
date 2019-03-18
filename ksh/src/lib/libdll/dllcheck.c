/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1997-2011 AT&T Intellectual Property          *
*                      and is licensed under the                       *
*                  Common Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
*                                                                      *
*                A copy of the License is available at                 *
*            http://www.opensource.org/licenses/cpl1.0.txt             *
*         (with md5 checksum 059e8cd6165cb4c31e351f2b69388fd9)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                                                                      *
***********************************************************************/
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Research
 */

#include "dlllib.h"

/*
 * return plugin version for dll
 * 0 if there is none
 * path!=0 enables library level diagnostics
 */

extern unsigned long
dllversion(void* dll, const char* path)
{
	Dll_plugin_version_f	pvf;

	if (pvf = (Dll_plugin_version_f)dlllook(dll, "plugin_version"))
		return (*pvf)();
	if (path)
	{
		state.error = 1;
		sfsprintf(state.errorbuf, sizeof(state.errorbuf), "plugin_version() not found");
		errorf("dll", NiL, 1, "%s: %s", path, state.errorbuf);
	}
	return 0;
}

/*
 * check if dll on path has plugin version >= ver
 * 1 returned on success, 0 on failure
 * path!=0 enables library level diagnostics
 * cur!=0 gets actual version
 */

extern int
dllcheck(void* dll, const char* path, unsigned long ver, unsigned long* cur)
{
	unsigned long		v;
	Dll_plugin_version_f	pvf;

	state.error = 0;
	if (ver || cur)
	{
		v = dllversion(dll, path);
		if (cur)
			*cur = v;
	}
	if (!ver)
		return 1;
	if (!v)
		return 0;
	if (v < ver)
	{
		if (path)
		{
			state.error = 1;
			sfsprintf(state.errorbuf, sizeof(state.errorbuf), "plugin version %lu older than caller %lu", v, ver);
			errorf("dll", NiL, 1, "%s: %s", path, state.errorbuf);
		}
		return 0;
	}
	return 1;
}