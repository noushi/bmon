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
#include <bmon/utils.h>

struct reader_timing rtiming;

static LIST_HEAD(primary_list);
static LIST_HEAD(secondary_list);

static struct input_module *preferred;

#define FOREACH_SECONDARY_INPUT(F)				 \
	do {							 \
		struct input_module *i;				 \
		list_for_each_entry(i, &secondary_list, im_list) \
			if (i->im_enable && i->im_##F)		 \
				i->im_##F ();			 \
	} while (0)

void input_register(struct input_module *ops)
{
	list_add_tail(&ops->im_list, &primary_list);
}

void input_register_secondary(struct input_module *ops)
{
	list_add_tail(&ops->im_list, &secondary_list);
}

static struct input_module *__get_input_module(const char *name,
					       struct list_head *list)
{
	struct input_module *i;

	list_for_each_entry(i, list, im_list)
		if (!strcmp(i->im_name, name))
			return i;

	return NULL;
}

static inline struct input_module *get_input_module(const char *name)
{
	return __get_input_module(name, &primary_list);
}

static inline struct input_module *get_sec_input_module(const char *name)
{
	return __get_input_module(name, &secondary_list);
}

static void find_preferred_input_module(void)
{
	if (preferred == NULL) {
		struct input_module *i;
		/* User has not specified an input module */

#if defined SYS_SUNOS
		preferred = get_input_module("kstat");
#elif defined SYS_BSD
		preferred = get_input_module("sysctl");
#elif defined SYS_LINUX
		preferred = get_input_module("netlink");

		if (preferred == NULL)
			preferred = get_input_module("proc");

		if (preferred == NULL)
			preferred = get_input_module("sysfs");
#endif

		/*
		 * All failed, search for any working module suitable
		 * as default.
		 */
		if (preferred == NULL) {
			list_for_each_entry(i, &primary_list, im_list) {
				if (!i->im_no_default) {
					preferred = i;
					break;
				}
			}
		}

		if (preferred == NULL)
			quit("No input method found\n");
	}
}

void input_read(void)
{
	find_preferred_input_module();
	preferred->im_read();

	FOREACH_SECONDARY_INPUT(read);
}

static void list_input(void)
{
	struct input_module *i;

	printf("Input modules:\n");
	if (list_empty(&primary_list))
		printf("\tNo input modules found.\n");
	else
		list_for_each_entry(i, &primary_list, im_list)
			printf("\t%s\n", i->im_name);
}

void input_set(const char *name)
{
	static int set;
	LIST_HEAD(list);
	module_conf_t *m;
	
	if (set)
		return;
	set = 1;

	if (name == NULL || !strcasecmp(name, "list")) {
		list_input();
		exit(0);
	}

	parse_module_param(name, &list);

	list_for_each_entry(m, &list, m_list) {
		preferred = get_input_module(m->m_name);

		if (preferred == NULL)
			quit("Unknown input module: %s\n", name);

		if (preferred->im_parse_opt) {
			tv_t *tv;

			list_for_each_entry(tv, &m->m_attrs, tv_list)
				preferred->im_parse_opt(tv->tv_type, tv->tv_value);
		}

		if (preferred->im_probe && preferred->im_probe())
			return;
	}

	quit("No (working) input module found\n");
}

static void list_sec_input(void)
{
	struct input_module *i;

	printf("Secondary input modules:\n");
	if (list_empty(&secondary_list))
		printf("\tNo secondary input modules found.\n");
	else
		list_for_each_entry(i, &secondary_list, im_list)
			printf("\t%s\n", i->im_name);
}

void input_set_secondary(const char *name)
{
	static int set;
	module_conf_t *m;
	LIST_HEAD(list);

	if (set)
		return;
	set = 1;

	if (name == NULL || !strcasecmp(name, "list")) {
		list_sec_input();
		exit(0);
	}
	
	parse_module_param(name, &list);
	list_for_each_entry(m, &list, m_list) {
		struct input_module *i = get_sec_input_module(m->m_name);

		if (i == NULL)
			quit("Unknown input module: %s\n", name);

		if (i->im_parse_opt) {
			tv_t *tv;
			list_for_each_entry(tv, &m->m_attrs, tv_list)
				i->im_parse_opt(tv->tv_type, tv->tv_value);
		}

		if (i->im_probe && i->im_probe())
			i->im_enable = 1;
	}
}

void input_init(void)
{
	find_preferred_input_module();

	if (preferred->im_init)
		preferred->im_init();

	FOREACH_SECONDARY_INPUT(init);
}

void input_shutdown(void)
{
	if (preferred && preferred->im_shutdown)
		preferred->im_shutdown();

	FOREACH_SECONDARY_INPUT(shutdown);
}

static void __init init_input(void)
{
	memset(&rtiming, 0, sizeof(rtiming));

	rtiming.rt_variance.v_min = FLT_MAX;
}
