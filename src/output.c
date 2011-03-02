/*
 * output.c               Output API
 *
 * Copyright (c) 2001-2011 Thomas Graf <tgraf@suug.ch>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <bmon/bmon.h>
#include <bmon/output.h>
#include <bmon/module.h>
#include <bmon/conf.h>
#include <bmon/signal.h>
#include <bmon/group.h>
#include <bmon/utils.h>

static struct bmon_subsys output_subsys;

void output_register(struct bmon_module *m)
{
	module_register(&output_subsys, m);
}

#define FOR_ALL_OUTPUT(F) \
	do { \
		struct bmon_module *m; \
		if (output_subsys.s_primary && \
		    output_subsys.s_primary->m_##F) \
			output_subsys.s_primary->m_##F (); \
		list_for_each_entry(m, &output_subsys.s_secondary_list, m_list) \
			if (m->m_flags & BMON_MODULE_ENABLED && m->m_##F) \
				m->m_##F (); \
	} while (0)

static struct bmon_module *output_get(const char *name)
{
	return module_lookup(name, &output_subsys.s_primary_list);
}

static void find_primary(void)
{
	if (!output_subsys.s_primary)
		output_subsys.s_primary = output_get("curses");

	if (!output_subsys.s_primary)
		output_subsys.s_primary = output_get("ascii");

	if (!output_subsys.s_primary)
		quit("No output module found.\n");
}

void output_pre(void)
{
	FOR_ALL_OUTPUT(pre);
}
					
void output_draw(void)
{
	static int cached, signal_driven;

	if (!cached) {
		signal_driven = cfg_getint(cfg, "signal_driven");
		cached = 1;
	}

	if (signal_driven && !signal_received())
		return;

	FOR_ALL_OUTPUT(do);
}

void output_post(void)
{
	FOR_ALL_OUTPUT(post);
}

void output_set(const char *name)
{
	return module_set(&output_subsys, BMON_PRIMARY_MODULE, name);
}

void output_set_secondary(const char *name)
{
	return module_set(&output_subsys, BMON_SECONDARY_MODULE, name);
}

static struct bmon_subsys output_subsys = {
	.s_name			= "output",
	.s_find_primary		= &find_primary,
	.s_primary_list		= LIST_SELF(output_subsys.s_primary_list),
	.s_secondary_list	= LIST_SELF(output_subsys.s_secondary_list),
};

static void __init __output_init(void)
{
	return module_register_subsys(&output_subsys);
}
