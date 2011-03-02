/*
 * in_proc.c		       /proc/net/dev Input (Linux)
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

static const char *c_path = "/proc/net/dev";
static const char *c_group = DEFAULT_GROUP;
static struct element_group *grp;

static void proc_read(void)
{
	struct element *e;
	FILE *fd;
	char buf[512], *p, *s;
	int w;
	
	if (!(fd = fopen(c_path, "r")))
		quit("Unable to open file %s: %s\n", c_path, strerror(errno));

	/* Ignore header */
	fgets(buf, sizeof(buf), fd);
	fgets(buf, sizeof(buf), fd);
	
	for (; fgets(buf, sizeof(buf), fd);) {
		uint64_t rx_errors, rx_drop, rx_fifo, rx_frame, rx_compressed;
		uint64_t rx_multicast, tx_errors, tx_drop, tx_fifo, tx_frame;
		uint64_t tx_compressed, tx_multicast, rx_bytes, tx_bytes;
		uint64_t rx_packets, tx_packets;
		
		if (buf[0] == '\r' || buf[0] == '\n')
			continue;

		if (!(p = strchr(buf, ':')))
			continue;
		*p = '\0';
		s = (p + 1);
		
		for (p = &buf[0]; *p == ' '; p++);

		w = sscanf(s, "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " "
			      "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " "
			      "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " "
			      "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
			      "\n",
			      &rx_bytes, &rx_packets, &rx_errors, &rx_drop,
			      &rx_fifo, &rx_frame, &rx_compressed,
			      &rx_multicast, &tx_bytes, &tx_packets,
			      &tx_errors, &tx_drop, &tx_fifo, &tx_frame,
			      &tx_compressed, &tx_multicast);
		
		if (w != 16)
			continue;

		if (!(e = element_lookup(grp, p, 0, NULL)))
			return;

		if (e->e_flags & ELEMENT_FLAG_CREATED) {

			element_set_key_attr(e, ATTR_BYTES, ATTR_PACKETS);
			element_set_usage_attr(e, ATTR_BYTES);

			e->e_flags &= ~ELEMENT_FLAG_CREATED;
		}

		attr_update(e, ATTR_BYTES, rx_bytes, tx_bytes,
			    UPDATE_FLAG_RX | UPDATE_FLAG_TX);

		attr_update(e, ATTR_PACKETS, rx_packets, tx_packets,
			    UPDATE_FLAG_RX | UPDATE_FLAG_TX);

		attr_update(e, ATTR_ERRORS, rx_errors, tx_errors,
			    UPDATE_FLAG_RX | UPDATE_FLAG_TX);

		attr_update(e, ATTR_DROP, rx_drop, tx_drop,
			    UPDATE_FLAG_RX | UPDATE_FLAG_TX);

		attr_update(e, ATTR_FIFO, rx_fifo, tx_fifo,
			    UPDATE_FLAG_RX | UPDATE_FLAG_TX);

		attr_update(e, ATTR_FRAME, rx_frame, tx_frame,
			    UPDATE_FLAG_RX | UPDATE_FLAG_TX);

		attr_update(e, ATTR_COMPRESSED, rx_compressed, tx_compressed,
			    UPDATE_FLAG_RX | UPDATE_FLAG_TX);

		attr_update(e, ATTR_MULTICAST, rx_multicast, tx_multicast,
			    UPDATE_FLAG_RX | UPDATE_FLAG_TX);

		element_notify_update(e, NULL);
		element_lifesign(e, 1);
	}
	
	fclose(fd);
}

static void print_help(void)
{
	printf(
	"proc - procfs statistic collector for Linux" \
	"\n" \
	"  Reads statistics from procfs (/proc/net/dev)\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n" \
	"\n" \
	"  Options:\n" \
	"    file=PATH	    Path to statistics file (default: /proc/net/dev)\n"
	"    group=NAME     Name of group\n");
}

static void proc_parse_opt(const char *type, const char *value)
{
	if (!strcasecmp(type, "file") && value)
		c_path = value;
	else if (!strcasecmp(type, "group") && value)
		c_group = value;
	else if (!strcasecmp(type, "help")) {
		print_help();
		exit(0);
	}
}

static void proc_do_init(void)
{
	if (!(grp = group_lookup(c_group, GROUP_CREATE)))
		BUG();
}

static int proc_probe(void)
{
	FILE *fd = fopen(c_path, "r");

	if (fd) {
		fclose(fd);
		return 1;
	}
	return 0;
}

static struct bmon_module proc_ops = {
	.m_name		= "proc",
	.m_type		= BMON_PRIMARY_MODULE,
	.m_do		= proc_read,
	.m_parse_opt	= proc_parse_opt,
	.m_probe	= proc_probe,
	.m_init		= proc_do_init,
};

static void __init proc_init(void)
{
	input_register(&proc_ops);
}
