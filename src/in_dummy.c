/*
 * in_dummy.c                Dummy Input Method
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
#include <bmon/group.h>
#include <bmon/element.h>
#include <bmon/attr.h>
#include <bmon/utils.h>

#define MAXDEVS 32

static uint64_t c_rx_b_inc = 1000000000;
static uint64_t c_tx_b_inc = 80000000;
static uint64_t c_rx_p_inc = 1000;
static uint64_t c_tx_p_inc = 800;
static int c_numdev = 5;
static int c_randomize = 0;
static int c_mtu = 1540;
static int c_maxpps = 100000;
static int c_numgroups = 2;

static uint64_t *cnts;

/* cnts[ngroups][ndevs][2][2] */

static inline int cnt_size(void)
{
	return c_numgroups * c_numdev * 2 * 2;
}

static inline uint64_t *cnt(int group, int dev, int unit,
			   int direction)
{
	return cnts + (group * c_numdev * 2 * 2) +
		      (dev * 2 * 2) +
		      (unit * 2) +
		      direction;
}

static void dummy_read(void)
{
	int gidx, n;

	for (gidx = 0; gidx < c_numgroups; gidx++) {
		char gname[32];
		struct element_group *group;

		snprintf(gname, sizeof(gname), "group%02d", gidx);
		group = group_lookup(gname, GROUP_CREATE);

		for (n = 0; n < c_numdev; n++) {
			char ifname[IFNAMSIZ];
			struct element *e;

			snprintf(ifname, sizeof(ifname), "dummy%d", n);

			if (!(e = element_lookup(group, ifname, 0, NULL)))
				return;

			if (e->e_flags & ELEMENT_FLAG_UPDATED)
				continue;

			/* FIXME: DONT DO THIS ON EVERY READ */
			element_set_key_attr(e, ATTR_BYTES, ATTR_PACKETS);
			element_set_usage_attr(e, ATTR_BYTES);

			if (c_randomize) {
				uint64_t rx = rand() % c_maxpps;
				uint64_t tx = rand() % c_maxpps;

				*cnt(gidx, n, 0, 0) += rx;
				*cnt(gidx, n, 0, 1) += tx;
				*cnt(gidx, n, 1, 0) += rx * (rand() % c_mtu);
				*cnt(gidx, n, 1, 1) += tx * (rand() % c_mtu);
			} else {
				*cnt(gidx, n, 0, 0) += c_rx_p_inc;
				*cnt(gidx, n, 0, 1) += c_tx_p_inc;
				*cnt(gidx, n, 1, 0) += c_rx_b_inc;
				*cnt(gidx, n, 1, 1) += c_tx_b_inc;
			}

			attr_update(e, ATTR_PACKETS,
				    *cnt(gidx, n, 0, 0),
				    *cnt(gidx, n, 0, 1),
				    UPDATE_FLAG_RX | UPDATE_FLAG_TX |
				    UPDATE_FLAG_64BIT);
			attr_update(e, ATTR_BYTES,
				    *cnt(gidx, n, 1, 0),
				    *cnt(gidx, n, 1, 1),
				    UPDATE_FLAG_RX | UPDATE_FLAG_TX |
				    UPDATE_FLAG_64BIT);

			element_notify_update(e, NULL);
			element_lifesign(e, 1);
		}
	}
}

static void print_help(void)
{
	printf(
	"dummy - Statistic generator module (dummy)\n" \
	"\n" \
	"  Basic statistic generator for testing purposes. Can produce a\n" \
	"  constant or random statistic flow with configurable parameters.\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n" \
	"\n" \
	"  Options:\n" \
	"    rxb=NUM        RX bytes increment amount (default: 10^9)\n" \
	"    txb=NUM        TX bytes increment amount (default: 8*10^7)\n" \
	"    rxp=NUM        RX packets increment amount (default: 1K)\n" \
	"    txp=NUM        TX packets increment amount (default: 800)\n" \
	"    num=NUM        Number of devices (default: 5)\n" \
	"    numgroups=NUM  Number of groups (default: 2)\n" \
	"    randomize      Randomize counters (default: off)\n" \
	"    seed=NUM       Seed for randomizer (default: time(0))\n" \
	"    mtu=NUM        Maximal Transmission Unit (default: 1540)\n" \
	"    maxpps=NUM     Upper limit for packets per second (default: 100K)\n" \
	"\n" \
	"  Randomizer:\n" \
	"    RX-packets := Rand() %% maxpps\n" \
	"    TX-packets := Rand() %% maxpps\n" \
	"    RX-bytes   := RX-packets * (Rand() %% mtu)\n" \
	"    TX-bytes   := TX-packets * (Rand() %% mtu)\n");
}

static void dummy_parse_opt(const char *value, const char *type)
{
	if (!strcasecmp(type, "rxb") && value)
		c_rx_b_inc = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "txb") && value)
		c_tx_b_inc = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "rxp") && value)
		c_rx_p_inc = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "txp") && value)
		c_tx_p_inc = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "num") && value)
		c_numdev = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "randomize")) {
		c_randomize = 1;
		srand(time(0));
	} else if (!strcasecmp(type, "seed") && value)
		srand(strtol(value, NULL, 0));
	else if (!strcasecmp(type, "mtu") && value)
		c_mtu = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "maxpps") && value)
		c_maxpps = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "numgroups") && value)
		c_numgroups = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "help")) {
		print_help();
		exit(0);
	}
}

static int dummy_probe(void)
{
	int i;

	if (c_numdev >= MAXDEVS) {
		fprintf(stderr, "numdev must be in rage 0..%d\n", MAXDEVS);
		return 0;
	}

	cnts = xcalloc(cnt_size(), sizeof(uint64_t));

	for (i = 0; i < c_numgroups; i++) {
		char groupname[32];
		snprintf(groupname, sizeof(groupname), "group%02d", i);

		group_new_derived_hdr(groupname, groupname, DEFAULT_GROUP);
	}

	return 1;
}

static struct input_module dummy_ops = {
	.im_name	= "dummy",
	.im_read	= dummy_read,
	.im_parse_opt	= dummy_parse_opt,
	.im_probe	= dummy_probe,
	.im_no_default	= 1,
};

static void __init dummy_init(void)
{
	input_register(&dummy_ops);
}
