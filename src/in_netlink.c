/*
 * in_netlink.c            rtnetlink input (Linux)
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
#include <bmon/attr.h>
#include <bmon/conf.h>
#include <bmon/input.h>
#include <bmon/utils.h>

static int c_notc = 0;
static const char *c_group = DEFAULT_GROUP;
static struct element_group *grp;

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/utils.h>
#include <netlink/route/link.h>
#include <netlink/route/tc.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/class.h>
#include <netlink/route/classifier.h>
#include <net/if.h>

struct attr_map {
	const char *	name;
	const char *	description;
	const char *	unit;
	int		attrid,
			type,
			rxid,
			txid;
};

#define A(NAME, TYPE, UNIT, DESC, RX, TX) \
	{ .name = NAME, .type = ATTR_TYPE_ ##TYPE, .unit = UNIT, \
	  .description = DESC, .rxid = RX, .txid = TX }

struct attr_map link_attrs[] = {
	A("bytes",	COUNTER, "byte", "Bytes",
			RTNL_LINK_RX_BYTES, RTNL_LINK_TX_BYTES),
	A("packets",	COUNTER, "number", "Packtes",
			RTNL_LINK_RX_PACKETS, RTNL_LINK_TX_PACKETS),
	A("errors",	COUNTER, "number", "Errors",
			RTNL_LINK_RX_ERRORS, RTNL_LINK_TX_ERRORS),
	A("drop",	COUNTER, "number", "Dropped",
			RTNL_LINK_RX_DROPPED, RTNL_LINK_TX_DROPPED),
	A("compressed",	COUNTER, "number", "Compressed",
			RTNL_LINK_RX_COMPRESSED, RTNL_LINK_TX_COMPRESSED),
	A("fifoerr",	COUNTER, "number", "FIFO Error",
			RTNL_LINK_RX_FIFO_ERR, RTNL_LINK_TX_FIFO_ERR),
	A("lenerr",	COUNTER, "number", "Length Error",
			RTNL_LINK_RX_LEN_ERR, -1),
	A("overerr",	COUNTER, "number", "Over Error",
			RTNL_LINK_RX_OVER_ERR, -1),
	A("crcerr",	COUNTER, "number", "CRC Error",
			RTNL_LINK_RX_CRC_ERR, -1),
	A("frameerr",	COUNTER, "number", "Frame Error",
			RTNL_LINK_RX_FRAME_ERR, -1),
	A("misserr",	COUNTER, "number", "Missed Error",
			RTNL_LINK_RX_MISSED_ERR, -1),
	A("aborterr",	COUNTER, "number", "Abort Error",
			-1, RTNL_LINK_TX_ABORT_ERR),
	A("carrerr",	COUNTER, "number", "Carrier Error",
			-1, RTNL_LINK_TX_CARRIER_ERR),
	A("hbeaterr",	COUNTER, "number", "Heartbeat Error",
			-1, RTNL_LINK_TX_HBEAT_ERR),
	A("winerr",	COUNTER, "number", "Window Error",
			-1, RTNL_LINK_TX_WIN_ERR),
	A("coll",	COUNTER, "number", "Collisions",
			-1, RTNL_LINK_COLLISIONS),
	A("mcast",	COUNTER, "number", "Multicast",
			-1, RTNL_LINK_MULTICAST),
	A("ip6pkts",	COUNTER, "number", "Ip6Pkts",
			RTNL_LINK_IP6_INPKTS, RTNL_LINK_IP6_OUTPKTS),
	A("ip6discards",COUNTER, "number", "Ip6Discards",
			RTNL_LINK_IP6_INDISCARDS, RTNL_LINK_IP6_OUTDISCARDS),
	A("ip6octets",	COUNTER, "byte", "Ip6Octets",
			RTNL_LINK_IP6_INOCTETS, RTNL_LINK_IP6_OUTOCTETS),
	A("ip6bcastp",	COUNTER, "number", "Ip6 Broadcast Packets",
			RTNL_LINK_IP6_INBCASTPKTS, RTNL_LINK_IP6_OUTBCASTPKTS),
	A("ip6bcast",	COUNTER, "byte", "Ip6 Broadcast",
			RTNL_LINK_IP6_INBCASTOCTETS, RTNL_LINK_IP6_OUTBCASTOCTETS),
	A("ip6mcastp",	COUNTER, "number", "Ip6 Multicast Packets",
			RTNL_LINK_IP6_INMCASTPKTS, RTNL_LINK_IP6_OUTMCASTPKTS),
	A("ip6mcast",	COUNTER, "byte", "Ip6 Multicast",
			RTNL_LINK_IP6_INMCASTOCTETS, RTNL_LINK_IP6_OUTMCASTOCTETS),
	A("ip6noroute",	COUNTER, "number", "Ip6 No Route",
			RTNL_LINK_IP6_INNOROUTES, RTNL_LINK_IP6_OUTNOROUTES),
	A("ip6forward",	COUNTER, "number", "Ip6 Forwarded",
			-1, RTNL_LINK_IP6_OUTFORWDATAGRAMS),
	A("ip6delivers",COUNTER, "number", "Ip6 Delivers",
			RTNL_LINK_IP6_INDELIVERS, -1),
	A("icmp6",	COUNTER, "number", "ICMPv6",
			RTNL_LINK_ICMP6_INMSGS, RTNL_LINK_ICMP6_OUTMSGS),
	A("icmp6err",	COUNTER, "number", "ICMPv6 Errors",
			RTNL_LINK_ICMP6_INERRORS, RTNL_LINK_ICMP6_OUTERRORS),
	A("ip6inhdrerr",COUNTER, "number", "Ip6 Header Error",
			RTNL_LINK_IP6_INHDRERRORS, -1),
	A("ip6toobigerr",COUNTER, "number", "Ip6 Too Big Error",
			RTNL_LINK_IP6_INTOOBIGERRORS, -1),
	A("ip6trunc",	COUNTER, "number", "Ip6 Truncated Packets",
			RTNL_LINK_IP6_INTRUNCATEDPKTS, -1),
	A("ip6unkproto",COUNTER, "number", "Ip6 Unknown Protocol Error",
			RTNL_LINK_IP6_INUNKNOWNPROTOS, -1),
	A("ip6addrerr",	COUNTER, "number", "Ip6 Address Error",
			RTNL_LINK_IP6_INADDRERRORS, -1),
	A("ip6reasmtimeo",	COUNTER, "number", "Ip6 Reassembly Timeouts",
			RTNL_LINK_IP6_REASMTIMEOUT, -1),
	A("ip6fragok",	COUNTER, "number", "Ip6 Reasm/Frag OK",
			RTNL_LINK_IP6_REASMOKS, RTNL_LINK_IP6_FRAGOKS),
	A("ip6fragfail",COUNTER, "number", "Ip6 Reasm/Frag Failures",
			RTNL_LINK_IP6_REASMFAILS, RTNL_LINK_IP6_FRAGFAILS),
	A("ip6fragcreate",COUNTER, "number", "Ip6 Reasm/Frag Requests",
			RTNL_LINK_IP6_REASMREQDS, RTNL_LINK_IP6_FRAGCREATES),
};

struct attr_map tc_attrs[] = {
	A("bytes",	COUNTER, "byte", "Bytes", -1, RTNL_TC_BYTES),
	A("packets",	COUNTER, "number", "Packets", -1, RTNL_TC_PACKETS),
	A("overlimits",	COUNTER, "number", "Overlimits",
			-1, RTNL_TC_OVERLIMITS),
	A("drop",	COUNTER, "number", "Dropped", -1, RTNL_TC_DROPS),
	A("bps",	RATE, "byte", "Byte Rate/s", -1, RTNL_TC_RATE_BPS),
	A("pps",	RATE, "number", "Packet Rate/s", -1, RTNL_TC_RATE_PPS),
	A("qlen",	RATE, "number", "Queue Length", -1, RTNL_TC_QLEN),
	A("backlog",	RATE, "number", "Backlog", -1, RTNL_TC_BACKLOG),
	A("requeues",	COUNTER, "number", "Requeues", -1, RTNL_TC_REQUEUES),
};

#undef A

struct rdata {
	struct element *	parent;
	int 			level;
};

static struct nl_sock *sock;
static struct nl_cache *link_cache, *qdisc_cache, *class_cache;

static void update_tc_attrs(struct element *e, struct rtnl_tc *tc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(tc_attrs); i++) {
		uint64_t c_tx = rtnl_tc_get_stat(tc, tc_attrs[i].txid);
		attr_update(e, tc_attrs[i].attrid, 0, c_tx, UPDATE_FLAG_TX);
	}
}

static void update_tc_infos(struct element *e, struct rtnl_tc *tc)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%u", rtnl_tc_get_mtu(tc));
	element_update_info(e, "MTU", buf);

	snprintf(buf, sizeof(buf), "%u", rtnl_tc_get_mpu(tc));
	element_update_info(e, "MPU", buf);

	snprintf(buf, sizeof(buf), "%u", rtnl_tc_get_overhead(tc));
	element_update_info(e, "Overhead", buf);

	snprintf(buf, sizeof(buf), "%#x", rtnl_tc_get_handle(tc));
	element_update_info(e, "Id", buf);

	snprintf(buf, sizeof(buf), "%#x", rtnl_tc_get_parent(tc));
	element_update_info(e, "Parent", buf);

	//extern uint32_t		rtnl_tc_get_linktype(struct rtnl_tc *);
}

static void handle_qdisc(struct element *, struct rtnl_qdisc *, int);

static void handle_filter(struct nl_object *obj, void *arg)
{
	struct rtnl_cls *cls = (struct rtnl_cls *) obj;
	struct rdata *rdata = arg;
	uint32_t id = rtnl_tc_get_handle((struct rtnl_tc *) cls);
	char *kind = rtnl_tc_get_kind((struct rtnl_tc *) cls);
	char name[IFNAME_MAX];
	struct element *e;

	if (id)
		rtnl_tc_handle2str(id, name, sizeof(name));
	else
		snprintf(name, sizeof(name), "cls:%s", kind);

	if (!(e = element_lookup(grp, name, id, rdata->parent)))
		return;

	if (e->e_flags & ELEMENT_FLAG_CREATED) {
		e->e_level = rdata->level;

		element_set_key_attr(e, ATTR_BYTES, ATTR_PACKETS);
		element_set_usage_attr(e, ATTR_BYTES);

		update_tc_infos(e, (struct rtnl_tc *) cls);

		e->e_flags &= ~ELEMENT_FLAG_CREATED;
	}

	update_tc_attrs(e, (struct rtnl_tc *) cls);

	element_notify_update(e, NULL);
	element_lifesign(e, 1);
}

static void handle_class(struct nl_object *obj, void *arg)
{
	struct element *e;
	struct rdata *rdata = arg;
	struct rtnl_qdisc *leaf;
	struct rtnl_class *class = (struct rtnl_class *) obj;
	uint32_t parent = rtnl_tc_get_handle((struct rtnl_tc *) class);
	uint32_t id = rtnl_tc_get_handle((struct rtnl_tc *) class);
	char *kind = rtnl_tc_get_kind((struct rtnl_tc *) class);
	char name[IFNAME_MAX];
	struct rdata ndata = {
		.level = rdata->level + 1,
		.parent = rdata->parent,
	};

	if (id)
		rtnl_tc_handle2str(id, name, sizeof(name));
	else
		snprintf(name, sizeof(name), "c:%s", kind);

	if (!(e = element_lookup(grp, name, id, rdata->parent)))
		return;

	if (e->e_flags & ELEMENT_FLAG_CREATED) {
		e->e_level = rdata->level;

		element_set_key_attr(e, ATTR_BYTES, ATTR_PACKETS);
		element_set_usage_attr(e, ATTR_BYTES);

		update_tc_infos(e, (struct rtnl_tc *) class);

		e->e_flags &= ~ELEMENT_FLAG_CREATED;
	}

	update_tc_attrs(e, (struct rtnl_tc *) class);

	element_notify_update(e, NULL);
	element_lifesign(e, 1);

	if ((leaf = rtnl_class_leaf_qdisc(class, qdisc_cache)))
		handle_qdisc(e, leaf, rdata->level + 1);

	rtnl_class_foreach_child(class, class_cache, &handle_class, &ndata);
	/* FIXME: classifier */
}

static void handle_qdisc(struct element *e, struct rtnl_qdisc *q, int level)
{
	struct element *qe;
	uint32_t id = rtnl_tc_get_handle((struct rtnl_tc *) q);
	char *kind = rtnl_tc_get_kind((struct rtnl_tc *) q);
	char name[IFNAME_MAX];
	struct rdata rdata = {
		.level = level + 1,
	};

	if (id)
		rtnl_tc_handle2str(id, name, sizeof(name));
	else
		snprintf(name, sizeof(name), "q:%s", kind);

	if (!(qe = element_lookup(grp, name, id, e)))
		return;
	
	rdata.parent = qe;

	if (qe->e_flags & ELEMENT_FLAG_CREATED) {
		qe->e_level = level;
		element_set_key_attr(qe, ATTR_BYTES, ATTR_PACKETS);
		element_set_usage_attr(qe, ATTR_BYTES);

		update_tc_infos(qe, (struct rtnl_tc *) q);

		qe->e_flags &= ~ELEMENT_FLAG_CREATED;
	}

	update_tc_attrs(qe, (struct rtnl_tc *) q);

	element_notify_update(qe, NULL);
	element_lifesign(qe, 1);

	rtnl_qdisc_foreach_child(q, class_cache, &handle_class, &rdata);

	//rtnl_qdisc_foreach_filter_nocache(nl_h, qdisc, &handle_filter, &xn);
}

static void handle_tc(struct element *e, struct rtnl_link *link)
{
	struct rtnl_qdisc *qdisc;
	int ifindex = rtnl_link_get_ifindex(link);

	if (rtnl_class_alloc_cache(sock, ifindex, &class_cache) < 0)
		return;

	qdisc = rtnl_qdisc_get_by_parent(qdisc_cache, ifindex, TC_H_ROOT);
	if (qdisc) {
		handle_qdisc(e, qdisc, 1);
		rtnl_qdisc_put(qdisc);
	}

	qdisc = rtnl_qdisc_get_by_parent(qdisc_cache, ifindex, 0);
	if (qdisc) {
		handle_qdisc(e, qdisc, 1);
		rtnl_qdisc_put(qdisc);
	}

	qdisc = rtnl_qdisc_get_by_parent(qdisc_cache, ifindex, TC_H_INGRESS);
	if (qdisc) {
		handle_qdisc(e, qdisc, 1);
		rtnl_qdisc_put(qdisc);
	}

	nl_cache_free(class_cache);
}

static void update_link_infos(struct element *e, struct rtnl_link *link)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%lu", rtnl_link_get_mtu(link));
	element_update_info(e, "MTU", buf);

	snprintf(buf, sizeof(buf), "%#x", rtnl_link_get_weight(link));
	element_update_info(e, "Weight", buf);

	rtnl_link_flags2str(rtnl_link_get_flags(link), buf, sizeof(buf));
	element_update_info(e, "Flags", buf);

	rtnl_link_operstate2str(rtnl_link_get_operstate(link),
				buf, sizeof(buf));
	element_update_info(e, "Operstate", buf);

	snprintf(buf, sizeof(buf), "%u", rtnl_link_get_ifindex(link));
	element_update_info(e, "IfIndex", buf);

	nl_addr2str(rtnl_link_get_addr(link), buf, sizeof(buf));
	element_update_info(e, "Address", buf);

	nl_addr2str(rtnl_link_get_broadcast(link), buf, sizeof(buf));
	element_update_info(e, "Broadcast", buf);

	rtnl_link_mode2str(rtnl_link_get_linkmode(link),
			   buf, sizeof(buf));
	element_update_info(e, "Mode", buf);

	snprintf(buf, sizeof(buf), "%u", rtnl_link_get_txqlen(link));
	element_update_info(e, "TXQlen", buf);

	nl_af2str(rtnl_link_get_family(link), buf, sizeof(buf));
	element_update_info(e, "Family", buf);

	element_update_info(e, "Alias",
		rtnl_link_get_ifalias(link) ? : "");

	element_update_info(e, "Qdisc",
		rtnl_link_get_qdisc(link) ? : "");
}

static void do_link(struct nl_object *obj, void *arg)
{
	struct rtnl_link *link = (struct rtnl_link *) obj;
	struct element *e;
	int i;

	if (!cfg_show_all && !(rtnl_link_get_flags(link) & IFF_UP))
		return;

	if (!(e = element_lookup(grp, rtnl_link_get_name(link), 0, NULL)))
		return;

	if (e->e_flags & ELEMENT_FLAG_CREATED) {

		element_set_key_attr(e, ATTR_BYTES, ATTR_PACKETS);
		element_set_usage_attr(e, ATTR_BYTES);

		update_link_infos(e, link);

		e->e_flags &= ~ELEMENT_FLAG_CREATED;
	}

	for (i = 0; i < ARRAY_SIZE(link_attrs); i++) {
		struct attr_map *m = &link_attrs[i];
		uint64_t c_rx = 0, c_tx = 0;
		int flags = 0;

		if (m->rxid >= 0) {
			c_rx = rtnl_link_get_stat(link, m->rxid);
			flags |= UPDATE_FLAG_RX;
		}

		if (m->txid >= 0) {
			c_tx = rtnl_link_get_stat(link, m->txid);
			flags |= UPDATE_FLAG_TX;
		}

		attr_update(e, m->attrid, c_rx, c_tx, flags);
	}

	if (!c_notc)
		handle_tc(e, link);

	element_notify_update(e, NULL);
	element_lifesign(e, 1);
}

static void netlink_read(void)
{
	int err;

	if (link_cache == NULL) {
		err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache);
		if (err < 0)
			quit("Unable to allocate link cache: %s\n",
				nl_geterror(err));
	} else {
		err = nl_cache_resync(sock, link_cache, NULL, NULL);
		if (err < 0)
			quit("Unable to resync link cache: %s\n",
				nl_geterror(err));
	}

	if (qdisc_cache == NULL) {
		err = rtnl_qdisc_alloc_cache(sock, &qdisc_cache);
		if (err < 0)
			quit("Unable to allocate qdisc cache: %s\n",
				nl_geterror(err));
	} else {
		err = nl_cache_resync(sock, qdisc_cache, NULL, NULL);
		if (err < 0)
			quit("Unable to resync qdisc cache: %s\n",
				nl_geterror(err));
	}

	nl_cache_foreach(link_cache, do_link, NULL);
}

static void netlink_shutdown(void)
{
	nl_cache_free(link_cache);
	nl_cache_free(qdisc_cache);
	nl_socket_free(sock);
}

static void netlink_do_init(void)
{
	int err, i;

	if (!(sock = nl_socket_alloc()))
		quit("Unable to allocate netlink socket\n");

	if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0)
		quit("Unable to connect netlink socket: %s\n", nl_geterror(err));

	for (i = 0; i < ARRAY_SIZE(link_attrs); i++) {
		struct attr_map *m = &link_attrs[i];
		struct unit *u;

		if (!(u = unit_lookup(m->unit)))
			continue;

		m->attrid = attr_def_add(m->name, m->description, u,
					 m->type, 0);
	}

	for (i = 0; i < ARRAY_SIZE(tc_attrs); i++) {
		struct attr_map *m = &tc_attrs[i];
		struct unit *u;

		if (!(u = unit_lookup(m->unit)))
			continue;

		m->attrid = attr_def_add(m->name, m->description, u,
					 m->type, 0);
	}

	if (!(grp = group_lookup(c_group, GROUP_CREATE)))
		BUG();
}

static int netlink_probe(void)
{
	struct nl_sock *sock = nl_socket_alloc();
	
	if (nl_connect(sock, NETLINK_ROUTE) < 0)
		return 0;
	
	if (rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache) < 0) {
		nl_socket_free(sock);
		return 0;
	}

	nl_cache_free(link_cache);
	link_cache = NULL;
	nl_socket_free(sock);
	
	return 1;
}

static void print_help(void)
{
	printf(
	"netlink - Netlink statistic collector for Linux\n" \
	"\n" \
	"  Powerful statistic collector for Linux using netlink sockets\n" \
	"  to collect link and traffic control statistics.\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n" \
	"\n" \
	"  Options:\n" \
	"    notc           Do not collect traffic control statistics\n" \
	"    group=NAME     Alternative group name (default: Interfaces)\n");
}

static void netlink_parse_opt(const char *type, const char *value)
{
	if (!strcasecmp(type, "notc"))
		c_notc = 1;
	else if (!strcasecmp(type, "group") && value)
		c_group = value;
	else if (!strcasecmp(type, "help")) {
		print_help();
		exit(0);
	}
}

static struct input_module netlink_ops = {
	.im_name	= "netlink",
	.im_read	= netlink_read,
	.im_shutdown	= netlink_shutdown,
	.im_parse_opt	= netlink_parse_opt,
	.im_probe	= netlink_probe,
	.im_init	= netlink_do_init,
};

static void __init netlink_init(void)
{
	input_register(&netlink_ops);
}
