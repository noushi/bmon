/*
 * in_kstat.c                  libkstat (SunOS)
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
#include <bmon/element.h>
#include <bmon/group.h>
#include <bmon/attr.h>
#include <bmon/utils.h>

#if defined HAVE_KSTAT && defined SYS_SUNOS
#include <kstat.h>

static struct element_group *grp;
static int grp_instance = 0;
static const char *c_group = DEFAULT_GROUP;

static void kstat_attr_update(kstat_t *kst, struct element *e, int attr,
			      char *rx, char *tx)
{
	uint64_t c_rx = 0, c_tx = 0;
	kstat_named_t *k;
	int flags = 0;
	char buf[32];

	if (rx) {
		snprintf(buf, sizeof(buf), "%s64", rx);

		if ((k = (kstat_named_t *) kstat_data_lookup(kst, buf))) {
			c_rx = k->value.ui64;
			flags |= UPDATE_FLAG_64BIT;
		} else if ((k = (kstat_named_t *) kstat_data_lookup(kst, rx)))
			c_rx = k->value.ui32;

		flags |= UPDATE_FLAG_RX;
	}

	if (tx) {
		snprintf(buf, sizeof(buf), "%s64", tx);

		if ((k = (kstat_named_t *) kstat_data_lookup(kst, buf))) {
			c_tx = k->value.ui64;
			flags |= UPDATE_FLAG_64BIT;
		} else if ((k = (kstat_named_t *) kstat_data_lookup(kst, tx)))
			c_tx = k->value.ui32;

		flags |= UPDATE_FLAG_TX;
	}

	attr_update(e, attr, c_rx, c_tx, flags);
}

static void kstat_read_interface(kstat_t *kst)
{
	struct element *e;

	/* group is cached */
	if (!grp || grp_instance != kst->ks_instance) {
		char group_name[64];

		snprintf(group_name, sizeof(group_name), "Instance %d",
			 kst->ks_instance);

		if (!(grp = group_lookup(group_name, GROUP_CREATE)))
			return;
	}

	if (!(e = element_lookup(grp, kst->ks_name, 0, NULL)))
		return;

	if (e->e_flags & ELEMENT_FLAG_CREATED) {
		element_set_key_attr(e, ATTR_BYTES, ATTR_PACKETS);
		element_set_usage_attr(e, ATTR_BYTES);

		e->e_flags &= ~ELEMENT_FLAG_CREATED;
	}

	kstat_attr_update(kst, e, ATTR_BYTES, "rbytes", "obytes");
	kstat_attr_update(kst, e, ATTR_PACKETS, "ipackets", "opackets");
	kstat_attr_update(kst, e, ATTR_ERRORS,  "ierrors", "oerrors");
	kstat_attr_update(kst, e, ATTR_MULTICAST, "multircv", "multixmt");
	//kstat_attr_update(kst, e, ATTR_BROADCAST, 0, 0, brdcstrcv, brdcstxmt);
	kstat_attr_update(kst, e, ATTR_COLLISIONS, NULL, "colissions");

	element_notify_update(e, NULL);
	element_lifesign(e, 1);
}

static void kstat_do_read(void)
{
	kstat_ctl_t *kc;
	kstat_t * kst;
	
	if (!(kc = kstat_open()))
		quit("kstat_open() failed");
	
	if ((kst = kstat_lookup(kc, NULL, -1, NULL))) {
		for (; kst; kst = kst->ks_next) {
			if (strcmp(kst->ks_class, "net"))
				continue;

			if (strcmp(kst->ks_module, "dls"))
				continue;
			
			if (kstat_read(kc, kst, NULL) < 0)
				continue;

			kstat_read_interface(kst);
		}
	}
	
	kstat_close(kc);
}

static void print_help(void)
{
	printf(
	"kstat - kstat statistic collector for SunOS\n" \
	"\n" \
	"  SunOS statistic collector using libkstat, kindly provides both,\n" \
	"  32bit and 64bit counters.\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n" \
	"\n" \
	"  Options:\n" \
	"    group=NAME     Alternative group name (default: Interfaces)\n");
}

static void kstat_set_opts(struct list_head *list)
{
	tv_t *tv;

	list_for_each_entry(tv, list, tv_list) {
		if (!strcasecmp(tv->tv_type, "help")) {
			print_help();
			exit(0);
		} else if (!strcasecmp(tv->tv_type, "group") && tv->tv_value)
			c_group = tv->tv_value;
	}
}

static void kstat_do_init(void)
{
	//if (!(grp = group_lookup(c_group, GROUP_CREATE)))
	//	BUG();
}

static int kstat_probe(void)
{
	kstat_ctl_t *kc = kstat_open();

	if (kc) {
		kstat_t * kst = kstat_lookup(kc, NULL, -1, NULL);
		kstat_close(kc);

		if (kst)
			return 1;
	}

	return 0;
}

static struct input_module kstat_ops = {
	.im_name	= "kstat",
	.im_read	= kstat_do_read,
	.im_set_opts	= kstat_set_opts,
	.im_probe	= kstat_probe,
	.im_init	= kstat_do_init,
};

static void __init kstat_init(void)
{
	input_register(&kstat_ops);
}

#endif
