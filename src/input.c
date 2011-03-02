/*
 * input.c            Input API
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
#include <bmon/input.h>
#include <bmon/module.h>
#include <bmon/utils.h>

static struct bmon_subsys input_subsys;

struct reader_timing rtiming;

#define FOREACH_SECONDARY_INPUT(F)				 \
	do {							 \
		struct bmon_module *m;				 \
		list_for_each_entry(m, &input_subsys.s_secondary_list, m_list) \
			if (m->m_flags & BMON_MODULE_ENABLED && m->m_##F)		 \
				m->m_##F ();			 \
	} while (0)

void input_register(struct bmon_module *m)
{
	module_register(&input_subsys, m);
}

static struct bmon_module *get_input(const char *name)
{
	return module_lookup(name, &input_subsys.s_primary_list);
}

static void find_primary(void)
{
	if (!input_subsys.s_primary) {
		/* User has not specified an input module */

#if defined SYS_SUNOS
		input_subsys.s_primary = get_input("kstat");
#elif defined SYS_BSD
		input_subsys.s_primary = get_input("sysctl");
#elif defined SYS_LINUX
		input_subsys.s_primary = get_input("netlink");

		if (!input_subsys.s_primary)
			input_subsys.s_primary = get_input("proc");

		if (!input_subsys.s_primary)
			input_subsys.s_primary = get_input("sysfs");
#endif

		/*
		 * All failed, search for any working module suitable
		 * as default.
		 */
		if (!input_subsys.s_primary) {
			struct bmon_module *m;
	
			list_for_each_entry(m, &input_subsys.s_primary_list, m_list) {
				if (!(m->m_flags & BMON_MODULE_NO_DEFAULT)) {
					input_subsys.s_primary = m;
					break;
				}
			}
		}

		if (!input_subsys.s_primary)
			quit("No input method found\n");
	}
}

void input_read(void)
{
	if (input_subsys.s_primary)
		input_subsys.s_primary->m_do();

	FOREACH_SECONDARY_INPUT(do);
}

void input_set(const char *name)
{
	return module_set(&input_subsys, BMON_PRIMARY_MODULE, name);
}

void input_set_secondary(const char *name)
{
	return module_set(&input_subsys, BMON_SECONDARY_MODULE, name);
}

static struct bmon_subsys input_subsys = {
	.s_name			= "input",
	.s_find_primary		= &find_primary,
	.s_primary_list		= LIST_SELF(input_subsys.s_primary_list),
	.s_secondary_list	= LIST_SELF(input_subsys.s_secondary_list),
};

static void __init __input_init(void)
{
	memset(&rtiming, 0, sizeof(rtiming));

	rtiming.rt_variance.v_min = FLT_MAX;

	module_register_subsys(&input_subsys);
}
