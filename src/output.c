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
#include <bmon/conf.h>
#include <bmon/signal.h>
#include <bmon/group.h>
#include <bmon/utils.h>

static LIST_HEAD(primary_list);
static LIST_HEAD(secondary_list);

static struct output_module *preferred;

void output_register(struct output_module *om)
{
	list_add_tail(&om->om_list, &primary_list);
}

void output_register_secondary(struct output_module *om)
{
	list_add_tail(&om->om_list, &secondary_list);
}

static struct output_module *__output_get(const char *name,
					  struct list_head *list)
{
	struct output_module *om;

	list_for_each_entry(om, list, om_list)
		if (!strcmp(om->om_name, name))
			return om;

	return NULL;
}

static struct output_module *output_get(const char *name)
{
	return __output_get(name, &primary_list);
}

static struct output_module * output_get_secondary(const char *name)
{
	return __output_get(name, &secondary_list);
}

#define FOR_ALL_OUTPUT(F) \
	do { \
		struct output_module *om; \
		if (preferred->om_##F) \
			preferred->om_##F (); \
		list_for_each_entry(om, &secondary_list, om_list) \
			if (om->om_enable && om->om_##F) \
				om->om_##F (); \
	} while (0)

static void find_preferred(int quiet)
{
	if (preferred == NULL)
		preferred = output_get("curses");

	if (preferred == NULL)
		preferred = output_get("ascii");

	if (preferred == NULL)
		quit("No output module found.\n");
}

void output_init(void)
{
	find_preferred(0);
	FOR_ALL_OUTPUT(init);
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

	FOR_ALL_OUTPUT(draw);
}

void output_post(void)
{
	FOR_ALL_OUTPUT(post);
}

void output_shutdown(void)
{
	if (!preferred)
		return;

	FOR_ALL_OUTPUT(shutdown);
}

static void list_output(void)
{
	struct output_module *o;

	printf("Output modules:\n");
	if (list_empty(&primary_list))
		printf("\tNo output modules found.\n");
	else
		list_for_each_entry(o, &primary_list, om_list)
			printf("\t%s\n", o->om_name);
}

void output_set(const char *name)
{
	static int set = 0;
	module_conf_t *m;
	LIST_HEAD(list);

	if (set)
		return;
	set = 1;

	if (NULL == name || !strcasecmp(name, "list")) {
		list_output();
		exit(0);
	}
	
	parse_module_param(name, &list);

	list_for_each_entry(m, &list, m_list) {
		if (!(preferred = output_get(m->m_name)))
			continue;

		if (preferred->om_parse_opt) {
			tv_t *tv;

			list_for_each_entry(tv, &m->m_attrs, tv_list)
				preferred->om_parse_opt(tv->tv_type, tv->tv_value);
		}

		if (preferred->om_probe)
			if (preferred->om_probe())
				return;
	}
	
	quit("No (working) output module found\n");
}

static void list_sec_output(void)
{
	struct output_module *o;

	printf("Secondary output modules:\n");
	if (list_empty(&secondary_list))
		printf("\tNo secondary output modules found.\n");
	else
		list_for_each_entry(o, &secondary_list, om_list)
			printf("\t%s\n", o->om_name);
}

void output_set_secondary(const char *name)
{
	module_conf_t *m;
	LIST_HEAD(list);

	if (NULL == name || !strcasecmp(name, "list")) {
		list_sec_output();
		exit(0);
	}
	
	parse_module_param(name, &list);

	list_for_each_entry(m, &list, m_list) {
		struct output_module *o = output_get_secondary(m->m_name);

		if (NULL == o) 
			quit("Unknown output module: %s\n", m->m_name);

		if (o->om_parse_opt) {
			tv_t *tv;

			list_for_each_entry(tv, &m->m_attrs, tv_list)
				o->om_parse_opt(tv->tv_type, tv->tv_value);
		}

		if (o->om_probe) {
			if (o->om_probe() == 1)
				o->om_enable = 1;
		}
	}
}
