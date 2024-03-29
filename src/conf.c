/*
 * conf.c        Config Crap
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

/*
 * TODO
 *  - cache lifecycles
 */

#include <bmon/bmon.h>
#include <bmon/conf.h>
#include <bmon/unit.h>
#include <bmon/attr.h>
#include <bmon/element.h>
#include <bmon/element_cfg.h>
#include <bmon/history.h>
#include <bmon/utils.h>

cfg_t *cfg;

static cfg_opt_t element_opts[] = {
	CFG_STR("description", NULL, CFGF_NONE),
	CFG_BOOL("show", cfg_true, CFGF_NONE),
	CFG_INT("rxmax", 0, CFGF_NONE),
	CFG_INT("txmax", 0, CFGF_NONE),
	CFG_INT("max", 0, CFGF_NONE),
	CFG_END()
};

static cfg_opt_t history_opts[] = {
	CFG_FLOAT("interval", 1.0f, CFGF_NONE),
	CFG_INT("size", 60, CFGF_NONE),
	CFG_STR("type", "64bit", CFGF_NONE),
	CFG_END()
};

static cfg_opt_t attr_opts[] = {
	CFG_STR("description", "", CFGF_NONE),
	CFG_STR("unit", "", CFGF_NONE),
	CFG_STR("type", "counter", CFGF_NONE),
	CFG_BOOL("history", cfg_false, CFGF_NONE),
	CFG_END()
};

static cfg_opt_t variant_opts[] = {
	CFG_FLOAT_LIST("div", "{}", CFGF_NONE),
	CFG_STR_LIST("txt", "", CFGF_NONE),
	CFG_END()
};

static cfg_opt_t unit_opts[] = {
	CFG_SEC("variant", variant_opts, CFGF_MULTI | CFGF_TITLE),
	CFG_END()
};

static cfg_opt_t global_opts[] = {
	CFG_FLOAT("read_interval", 1.0f, CFGF_NONE),
	CFG_FLOAT("rate_interval", 1.0f, CFGF_NONE),
	CFG_FLOAT("lifetime", 30.0f, CFGF_NONE),
	CFG_FLOAT("history_variance", 0.1f, CFGF_NONE),
	CFG_FLOAT("variance", 0.1f, CFGF_NONE),
	CFG_BOOL("show_all", cfg_false, CFGF_NONE),
	CFG_INT("unit_exp", -1, CFGF_NONE),
	CFG_INT("sleep_time", 20000UL, CFGF_NONE),
	CFG_BOOL("use_si", 0, CFGF_NONE),
	CFG_BOOL("daemon", 0, CFGF_NONE),
	CFG_STR("uid", NULL, CFGF_NONE),
	CFG_STR("gid", NULL, CFGF_NONE),
	CFG_STR("pidfile", "/var/run/bmon.pid", CFGF_NONE),
	CFG_INT("signal_driven", 0, CFGF_NONE),
	CFG_STR("policy", "", CFGF_NONE),
	CFG_SEC("unit", unit_opts, CFGF_MULTI | CFGF_TITLE),
	CFG_SEC("attr", attr_opts, CFGF_MULTI | CFGF_TITLE),
	CFG_SEC("history", history_opts, CFGF_MULTI | CFGF_TITLE),
	CFG_SEC("element", element_opts, CFGF_MULTI | CFGF_TITLE),
};

float			cfg_read_interval;
float			cfg_rate_interval;
float			cfg_rate_variance;
float			cfg_history_variance;
int			cfg_show_all;
int			cfg_unit_exp		= DYNAMIC_EXP;

static char *		configfile		= NULL;

#if defined HAVE_CURSES
#if defined HAVE_USE_DEFAULT_COLORS
struct layout cfg_layout[] =
{
	{-1, -1, 0},                           /* dummy, not used */
	{-1, -1, 0},                           /* default */
	{-1, -1, A_REVERSE},                   /* statusbar */
	{-1, -1, 0},                           /* header */
	{-1, -1, 0},                           /* list */
	{-1, -1, A_REVERSE},                   /* selected */
};
#else
struct layout cfg_layout[] =
{
	{0, 0, 0},                              /* dummy, not used */
	{COLOR_BLACK, COLOR_WHITE, 0},          /* default */
	{COLOR_BLACK, COLOR_WHITE, A_REVERSE},  /* statusbar */
	{COLOR_BLACK, COLOR_WHITE, 0},          /* header */
	{COLOR_BLACK, COLOR_WHITE, 0},          /* list */
	{COLOR_BLACK, COLOR_WHITE, A_REVERSE},  /* selected */
};
#endif
#endif

tv_t * parse_tv(char *data)
{
	char *value;
	tv_t *tv = xcalloc(1, sizeof(tv_t));

	init_list_head(&tv->tv_list);

	value = strchr(data, '=');

	if (value) {
		*value = '\0';
		++value;
		tv->tv_value = strdup(value);
	}

	tv->tv_type = strdup(data);
	return tv;
}

module_conf_t * parse_module(char *data)
{
	char *name = data, *opts = data, *next;
	module_conf_t *m;

	if (!*name)
		quit("No module name given");

	m = xcalloc(1, sizeof(module_conf_t));

	init_list_head(&m->m_attrs);

	opts = strchr(data, ':');

	if (opts) {
		*opts = '\0';
		opts++;

		do {
			tv_t *tv;
			next = strchr(opts, ';');

			if (next) {
				*next = '\0';
				++next;
			}

			tv = parse_tv(opts);
			list_add_tail(&tv->tv_list, &m->m_attrs);

			opts = next;
		} while(next);
	}

	m->m_name = strdup(name);
	return m;
}


int parse_module_param(const char *data, struct list_head *list)
{
	char *buf = strdup(data);
	char *next;
	char *current = buf;
	module_conf_t *m;
	int n = 0;
	
	do {
		next = strchr(current, ',');

		if (next) {
			*next = '\0';
			++next;
		}

		m = parse_module(current);
		if (m) {
			list_add_tail(&m->m_list, list);
			n++;
		}

		current = next;
	} while (next);

	free(buf);

	return n;
}

static void configfile_read_history(void)
{
	int i, nhistory;

	nhistory = cfg_size(cfg, "history");

	for (i = 0; i < nhistory; i++) {
		struct history_def *def;
		cfg_t *history;
		const char *name, *type;
		float interval;
		int size;

		if (!(history = cfg_getnsec(cfg, "history", i)))
			BUG();

		if (!(name = cfg_title(history)))
			BUG();

		interval = cfg_getfloat(history, "interval");
		size = cfg_getint(history, "size");
		type = cfg_getstr(history, "type");

		if (interval == 0.0f)
			interval = cfg_getfloat(cfg, "read_interval");

		def = history_def_alloc(name);
		def->hd_interval = interval;
		def->hd_size = size;

		if (!strcasecmp(type, "8bit"))
			def->hd_type = HISTORY_TYPE_8;
		else if (!strcasecmp(type, "16bit"))
			def->hd_type = HISTORY_TYPE_16;
		else if (!strcasecmp(type, "32bit"))
			def->hd_type = HISTORY_TYPE_32;
		else if (!strcasecmp(type, "64bit"))
			def->hd_type = HISTORY_TYPE_64;
		else
			quit("Invalid type \'%s\', must be \"(8|16|32|64)bit\""
			     " in history definition #%d\n", type, i+1);
	}
}

static void configfile_read_element_cfg(void)
{
	int i, nelement;

	nelement = cfg_size(cfg, "element");

	for (i = 0; i < nelement; i++) {
		struct element_cfg *ec;
		cfg_t *element;
		const char *name, *description;
		long max;

		if (!(element = cfg_getnsec(cfg, "element", i)))
			BUG();

		if (!(name = cfg_title(element)))
			BUG();

		ec = element_cfg_alloc(name);

		if ((description = cfg_getstr(element, "description")))
			ec->ec_description = strdup(description);

		if ((max = cfg_getint(element, "max")))
			ec->ec_rxmax = ec->ec_txmax = max;

		if ((max = cfg_getint(element, "rxmax")))
			ec->ec_rxmax = max;

		if ((max = cfg_getint(element, "txmax")))
			ec->ec_txmax = max;

		if (cfg_getbool(element, "show"))
			ec->ec_flags |= ELEMENT_CFG_SHOW;
		else
			ec->ec_flags |= ELEMENT_CFG_HIDE;
	}
}

static void add_div(struct unit *unit, int type, cfg_t *variant)
{
	int ndiv, n, ntxt;

	if (!(ndiv = cfg_size(variant, "div")))
		return;

	ntxt = cfg_size(variant, "txt");
	if (ntxt != ndiv)
		quit("Number of elements for div and txt not equal\n");

	if (!list_empty(&unit->u_div[type])) {
		struct fraction *f, *n;

		list_for_each_entry_safe(f, n, &unit->u_div[type], f_list)
			fraction_free(f);
	}

	for (n = 0; n < ndiv; n++) {
		char *txt;
		float div;

		div = cfg_getnfloat(variant, "div", n);
		txt = cfg_getnstr(variant, "txt", n);

		unit_add_div(unit, type, txt, div);
	}
}

static void configfile_read_units(void)
{
	int i, nunits;
	struct unit *u;

	nunits = cfg_size(cfg, "unit");

	for (i = 0; i < nunits; i++) {
		int nvariants, n;
		cfg_t *unit;
		const char *name;

		if (!(unit = cfg_getnsec(cfg, "unit", i)))
			BUG();

		if (!(name = cfg_title(unit)))
			BUG();

		if (!(nvariants = cfg_size(unit, "variant")))
			continue;

		if (!(u = unit_add(name)))
			continue;

		for (n = 0; n < nvariants; n++) {
			cfg_t *variant;
			const char *vtitle;

			if (!(variant = cfg_getnsec(unit, "variant", n)))
				BUG();

			if (!(vtitle = cfg_title(variant)))
				BUG();

			if (!strcasecmp(vtitle, "default"))
				add_div(u, UNIT_DEFAULT, variant);
			else if (!strcasecmp(vtitle, "si"))
				add_div(u, UNIT_SI, variant);
			else
				quit("Unknown unit variant \'%s\'\n", vtitle);
		}
	}
}

static void configfile_read_attrs(void)
{
	int i, nattrs, t;

	nattrs = cfg_size(cfg, "attr");

	for (i = 0; i < nattrs; i++) {
		struct unit *u;
		cfg_t *attr;
		const char *name, *description, *unit, *type;
		int flags = 0;

		if (!(attr = cfg_getnsec(cfg, "attr", i)))
			BUG();

		if (!(name = cfg_title(attr)))
			BUG();

		description = cfg_getstr(attr, "description");
		unit = cfg_getstr(attr, "unit");
		type = cfg_getstr(attr, "type");

		if (!unit)
			quit("Attribute '%s' is missing unit specification\n",
			     name);

		if (!type)
			quit("Attribute '%s' is missing type specification\n",
			     name);

		if (!(u = unit_lookup(unit)))
			quit("Unknown unit \'%s\' attribute '%s'\n",
				unit, name);

		if (!strcasecmp(type, "counter"))
			t = ATTR_TYPE_COUNTER;
		else if (!strcasecmp(type, "rate"))
			t = ATTR_TYPE_RATE;
		else if (!strcasecmp(type, "percent"))
			t = ATTR_TYPE_PERCENT;
		else
			quit("Unknown type \'%s\' in attribute '%s'\n",
				type, name);

		if (cfg_getbool(attr, "history"))
			flags |= ATTR_DEF_FLAG_HISTORY;

		attr_def_add(name, description, u, t, flags);
	}
}

static void conf_read(const char *path, int must)
{
	int err;

	DBG(1, "Reading configfile %s...\n", path);

	if (access(path, R_OK) != 0) {
		if (must)
			quit("Error: Unable to read configfile \"%s\": %s\n",
			     path, strerror(errno));
		else
			return;
	}

	err = cfg_parse(cfg, path);
	if (err == CFG_FILE_ERROR) {
		quit("Error while reading configfile \"%s\": %s\n",
		     path, strerror(errno));
	} else if (err == CFG_PARSE_ERROR) {
		quit("Error while reading configfile \"%s\": parse error\n",
		     path);
	}

	configfile_read_units();
	configfile_read_history();
	configfile_read_attrs();
	configfile_read_element_cfg();
}

void configfile_read(void)
{
	if (configfile)
		conf_read(configfile, 1);
	else {
		conf_read(SYSCONFDIR "/bmonrc", 0);
		
		if (getenv("HOME")) {
			char path[FILENAME_MAX+1];
			snprintf(path, sizeof(path), "%s/.bmonrc",
				 getenv("HOME"));
			conf_read(path, 0);
		}
	}
}

void conf_init(void)
{
	cfg_read_interval = cfg_getfloat(cfg, "read_interval");
	cfg_rate_interval = cfg_getfloat(cfg, "rate_interval");
	cfg_rate_variance = cfg_getfloat(cfg, "variance") * cfg_rate_interval;
	cfg_history_variance = cfg_getfloat(cfg, "history_variance");
	cfg_show_all = cfg_getbool(cfg, "show_all");
	cfg_unit_exp = cfg_getint(cfg, "unit_exp");

	element_parse_policy(cfg_getstr(cfg, "policy"));
}

void set_configfile(const char *file)
{
	static int set = 0;
	if (!set) {
		configfile = strdup(file);
		set = 1;
	}
}

void set_unit_exp(const char *name)
{
	if (tolower(*name) == 'b')
		cfg_setint(cfg, "unit_exp", 0);
	else if (tolower(*name) == 'k')
		cfg_setint(cfg, "unit_exp", 1);
	else if (tolower(*name) == 'm')
		cfg_setint(cfg, "unit_exp", 2);
	else if (tolower(*name) == 'g')
		cfg_setint(cfg, "unit_exp", 3);
	else if (tolower(*name) == 't')
		cfg_setint(cfg, "unit_exp", 4);
	else if (tolower(*name) == 'd')
		cfg_setint(cfg, "unit_exp", DYNAMIC_EXP);
	else
		quit("Unknown unit exponent '%s'\n", name);
}

unsigned int get_lifecycles(void)
{
	return (unsigned int)
		(cfg_getfloat(cfg, "lifetime") / cfg_getfloat(cfg, "read_interval"));
}

void conf_shutdown(void)
{
	cfg_free(cfg);
}

static void __init __conf_init(void)
{
	cfg = cfg_init(global_opts, CFGF_NOCASE);

	/* FIXME: Add validation functions */
	//cfg_set_validate_func(cfg, "bookmark", &cb_validate_bookmark);
}
