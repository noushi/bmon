/*
 * module.c               Module API
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
#include <bmon/module.h>
#include <bmon/utils.h>

static LIST_HEAD(subsys_list);

void module_foreach(struct bmon_subsys *ss, void (*cb)(struct bmon_module *))
{
	struct bmon_module *m;

	if (ss->s_primary)
		cb(ss->s_primary);

	list_for_each_entry(m, &ss->s_secondary_list, m_list)
		cb(m);
}

struct bmon_module *module_lookup(const char *name, struct list_head *list)
{
	struct bmon_module *m;

	list_for_each_entry(m, list, m_list)
		if (!strcmp(m->m_name, name))
			return m;

	return NULL;
}

static void module_list(struct bmon_subsys *ss, struct list_head *list)
{
	printf("%s modules:\n", ss->s_name);
	if (list_empty(list))
		printf("\tNo %s modules found.\n", ss->s_name);
	else {
		struct bmon_module *m;

		list_for_each_entry(m, list, m_list)
			printf("\t%s (%s)\n",
				m->m_name,
				m->m_type == BMON_PRIMARY_MODULE ?
				"primary" : "secondary");
	}
}

static int module_configure(struct bmon_module *m, module_conf_t *cfg)
{
	if (m->m_parse_opt) {
		tv_t *tv;

		list_for_each_entry(tv, &cfg->m_attrs, tv_list)
			m->m_parse_opt(tv->tv_type, tv->tv_value);
	}

	if (m->m_probe && !m->m_probe())
		return -EINVAL;

	return 0;
}

void module_register(struct bmon_subsys *ss, struct bmon_module *m)
{
	switch (m->m_type) {
	case BMON_PRIMARY_MODULE:
		list_add_tail(&m->m_list, &ss->s_primary_list);
		break;
	
	case BMON_SECONDARY_MODULE:
		list_add_tail(&m->m_list, &ss->s_secondary_list);
		break;
	}
}

void module_set(struct bmon_subsys *ss, mod_type_t type, const char *name)
{
	struct bmon_module *mod;
	struct list_head *list;
	LIST_HEAD(tmp_list);
	module_conf_t *m;

	switch (type) {
	case BMON_PRIMARY_MODULE:
		list = &ss->s_primary_list;
		break;

	case BMON_SECONDARY_MODULE:
		list = &ss->s_secondary_list;
		break;

	default:
		BUG();
	}

	if (!name || !strcasecmp(name, "list")) {
		module_list(ss, list);
		exit(0);
	}
	
	parse_module_param(name, &tmp_list);

	list_for_each_entry(m, &tmp_list, m_list) {
		switch (type) {
		case BMON_PRIMARY_MODULE:
			if (!(ss->s_primary = module_lookup(m->m_name, list)))
				continue;

			if (module_configure(ss->s_primary, m) < 0)
				continue;

			return;

		case BMON_SECONDARY_MODULE:
			if (!(mod = module_lookup(m->m_name, list)))
				quit("Unknown %s module: %s\n",
					ss->s_name, m->m_name);

			if (module_configure(mod, m) == 0)
				mod->m_flags |= BMON_MODULE_ENABLED;
			break;
		}
	}

	if (type == BMON_PRIMARY_MODULE)
		quit("No working %s module found\n", ss->s_name);
}

void __module_init(struct bmon_module *m)
{
	if (m->m_init)
		m->m_init();
}

void module_init(void)
{
	struct bmon_subsys *ss;

	list_for_each_entry(ss, &subsys_list, s_list) {
		if (ss->s_find_primary)
			ss->s_find_primary();

		module_foreach(ss, __module_init);
	}
}

void __module_shutdown(struct bmon_module *m)
{
	if (m->m_shutdown)
		m->m_shutdown();
}

void module_shutdown(void)
{
	struct bmon_subsys *ss;

	list_for_each_entry(ss, &subsys_list, s_list)
		module_foreach(ss, __module_shutdown);
}

void module_register_subsys(struct bmon_subsys *ss)
{
	list_add_tail(&ss->s_list, &subsys_list);
}
