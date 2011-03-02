/*
 * out_rrd.c		RRD Output
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
#include <bmon/input.h>
#include <bmon/element.h>
#include <bmon/group.h>
#include <bmon/attr.h>
#include <bmon/utils.h>
#include <inttypes.h>

#define MAX_RRA 128

static const char *c_path = "./";
static char *c_step = "1";
static const char *c_heartbeat = "2";
static char *c_rra[MAX_RRA];
static int c_rra_index = 0;
static int c_update_interval = 1;
static int c_always_unique = 0;

static void create_rrd(char *file, struct element *e)
{
	char *argv[256];
	char nows[32];
	int i = 0, m, ul;
	struct attr *a;

	memset(argv, 0, sizeof(argv));

	snprintf(nows, sizeof(nows), "%lld", (unsigned long long) (time(0) - 1));

	argv[i++] = "create";
	argv[i++] = file;
	argv[i++] = "--start";
	argv[i++] = nows;
	argv[i++] = "--step";
	argv[i++] = c_step;

	list_for_each_entry(a, &e->e_attr_sorted, a_sort_list) {
		char ds[128];

		if (i >= (253 - c_rra_index))
			goto overflow;

		snprintf(ds, sizeof(ds), "DS:%s_rx:%s:%s:U:U",
			a->a_def->ad_name,
			a->a_def->ad_type == ATTR_TYPE_COUNTER ?
				"COUNTER" : "GAUGE",
			c_heartbeat);

		argv[i++] = strdup(ds);

		if (i >= (253 - c_rra_index))
			goto overflow;

		snprintf(ds, sizeof(ds), "DS:%s_tx:%s:%s:U:U",
			a->a_def->ad_name,
			a->a_def->ad_type == ATTR_TYPE_COUNTER ?
				"COUNTER" : "GAUGE",
			c_heartbeat);

		argv[i++] = strdup(ds);
	}
	
	ul = i;

	if (i + c_rra_index >= 255)
		goto overflow;

	for (m = 0; m < c_rra_index; m++)
		argv[i++] = c_rra[m];

	optind = 0; /* What a nice undocumented precondition */
	opterr = 0;

	if (rrd_create(i, argv) < 0)
		fprintf(stderr, "rrd_create failed: %s\n", rrd_get_error());

	for (i = 6; i < ul && argv[i]; i++)
		free(argv[i]);

	return;

overflow:
	quit("Argument overflow, blame the RRD API\n");
}

static void update_rrd(char *file, struct element *e)
{
	char *argv[6];
	char template[256];
	char data[1024];
	int m, i = 0;
	struct attr *a;

	memset(argv, 0, sizeof(argv));

	argv[i++] = "update";
	argv[i++] = file;
	argv[i++] = "--template";

	memset(template, 0, sizeof(template));

	list_for_each_entry(a, &e->e_attr_sorted, a_sort_list) {
		if (template[0])
			strncat(template, ":",
			    sizeof(template) - strlen(template) - 1);
		strncat(template, a->a_def->ad_name,
		    sizeof(template) - strlen(template) - 1);
		strncat(template, "_rx",
		    sizeof(template) - strlen(template) - 1);
		strncat(template, ":",
		    sizeof(template) - strlen(template) - 1);
		strncat(template, a->a_def->ad_name,
		    sizeof(template) - strlen(template) - 1);
		strncat(template, "_tx",
		    sizeof(template) - strlen(template) - 1);
	}

	argv[i++] = template;

	snprintf(data, sizeof(data), "%" PRId64,
	    rtiming.rt_last_read.tv_sec);

	list_for_each_entry(a, &e->e_attr_sorted, a_sort_list) {
		char valuepair[64];

		snprintf(valuepair, sizeof(valuepair),
		    "%s%" PRId64 ":%" PRId64,
		    data[0] ? ":" : "", 
		    	a->a_rx_rate.r_total,
		    	a->a_tx_rate.r_total);

		strncat(data, valuepair,
		    sizeof(data) - strlen(data) - 1);
	}

	argv[4] = data;

	/*for (m = 0; m < 5; m++)
		printf("%s ", argv[m]);
	printf("\n");*/

	optind = 0; /* What a nice undocumented precondition */
	opterr = 0;

	rrd_update(5, argv);
}

static void rrd_draw_element(struct element_group *g, struct element *e,
			     void *arg)
{
	char file[FILENAME_MAX];

	if (c_always_unique)
		snprintf(file, sizeof(file), "%s/%d_%s.rrd",
			 c_path, e->e_id, e->e_name);
	else
		snprintf(file, sizeof(file), "%s/%s.rrd",
			 c_path, e->e_name);

	if (access(file, W_OK) != 0)
		create_rrd(file, e);

	if (access(file, W_OK) == 0)
		update_rrd(file, e);
}

void rrd_draw(void)
{
	static int remaining = 1;

	if (--remaining)
		return;
	else
		remaining = c_update_interval;

	group_foreach_recursive(rrd_draw_element, NULL);
}

static void rrd_do_init(void)
{
	if (!c_rra_index)
		c_rra_index = 1;
}

static void print_module_help(void)
{
	printf(
	"RRD - RRD Output\n" \
	"\n" \
	"  Writes updates to RRD databases. Databases are created if needed\n" \
	"  and non-existent\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n" \
	"\n" \
	"  Options:\n" \
	"    path=PATH        Output directory\n" \
	"    step=SECS        Interval RRD expects updates (default: 1 second)\n" \
	"    heartbeat=SECS   Maximum interval until RRD throws away an\n" \
	"                     update (default: 2 seconds)\n" \
	"    interval=NUM     Update interval in read interval cycles\n" \
	"    rra=RRA_DEF      RRA definition (default: RRA:AVERAGE:0.5:1:86400)\n");
}

static void rrd_parse_opt(const char *type, const char *value)
{
	if (!strcasecmp(type, "path") && value)
		c_path = value;
	else if (!strcasecmp(type, "step") && value)
		c_step = (char *) value;
	else if (!strcasecmp(type, "heartbeat") && value)
		c_heartbeat = value;
	else if (!strcasecmp(type, "always_unique"))
		c_always_unique = 1;
	else if (!strcasecmp(type, "interval") && value)
		c_update_interval = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "rra") && value) {
		if (c_rra_index < MAX_RRA)
			c_rra[c_rra_index++] = (char *) value;
	} else if (!strcasecmp(type, "help")) {
		print_module_help();
		exit(0);
	}
}

static int rrd_probe(void)
{
	return 1;
}

static struct bmon_module rrd_ops = {
	.m_name		= "rrd",
	.m_type		= BMON_SECONDARY_MODULE,
	.m_do		= rrd_draw,
	.m_parse_opt	= rrd_parse_opt,
	.m_probe	= rrd_probe,
	.m_init		= rrd_do_init,
};

static void __init save_init(void)
{
	c_rra[0] = "RRA:AVERAGE:0.5:1:86400";
	output_register(&rrd_ops);
}
