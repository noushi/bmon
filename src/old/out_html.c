/*
 * out_html.c                      HTML Output
 *
 * Copyright (c) 2001-2005 Thomas Graf <tgraf@suug.ch>
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
#include <bmon/node.h>
#include <bmon/output.h>
#include <bmon/graph.h>
#include <bmon/utils.h>
#include <bmon/attr.h>

static const char *c_path;
static int c_graph_height = 6;
static int c_update_interval = 1;
static int c_rowspan = 1;
static int c_hide_item = 0;
static char *c_title = "Traffic Rate Estimation and Monitoring";

static FILE * open_file(const char *p)
{
	FILE *fd;

	if (!(fd = fopen(p, "w")))
		quit("fopen(%s) failed: %s\n", p, strerror(errno));

	return fd;
}

static void write_css(const char *path)
{
    char ofile[FILENAME_MAX];
    FILE *f;

	snprintf(ofile, sizeof(ofile), "%s/layout.css", path);
	
	/* do not overwrite an existing .css */
	if (!access(ofile, R_OK))
		return;

	f = open_file(ofile);
	fprintf(f, 
		"/*\n" \
		" * Generated by %s\n" \
		" */\n" \
		"\n" \
		"body {\n" \
		"	background: #f7f7f7;\n" \
		"	margin-top: 20px;\n" \
		"}\n" \
		"\n" \
		"p, b, td {\n" \
		"	color: #000000;\n" \
		"	font-family: Verdana, Arial, Helvetica, sans-serif;\n" \
		"	font-size: 10pt;\n" \
		"}\n" \
		"\n" \
		"a:link,a:visited,a:hover {\n" \
		"  color: #4E7AAF;\n" \
		"  text-decoration: underline;\n" \
		"}\n" \
		"\n" \
		"p.banner {\n" \
		"	margin-top: 8px;\n" \
		"	margin-bottom: 8px;\n" \
		"	text-align: center;\n" \
		"	font-size: 18pt;\n" \
		"	font-weight: bold;\n" \
		"}\n" \
		"\n" \
		"p.struct nodeitle {\n" \
		"	text-align: center;\n" \
		"	margin: 0pt;\n" \
		"	margin-bottom: 10px;\n" \
		"	font-weight: bold;\n" \
		"	font-size: 18pt;\n" \
		"}\n" \
		"\n" \
		"p.title {\n" \
		"	margin: 0pt;\n" \
		"	margin-top: 5px;\n" \
		"	text-align: center;\n" \
		"	font-weight: bold;\n" \
		"	font-size: 14pt;\n" \
		"}\n" \
		"\n" \
		"a.a_node_link:link,a.a_node_link:visited,a.a_node_link:hover {\n" \
		"	color: #000;\n" \
		"	font-weight: bold;\n" \
		"	text-decoration: none;\n" \
		"}\n" \
		"\n" \
		"a.a_node_link:hover {\n" \
		"	font-weight: bold;\n" \
		"	text-decoration: underline;\n" \
		"}\n" \
		"\n" \
		"a.a_intf_link:link,a.a_intf_link:visited,a.a_intf_link:hover {\n" \
		"	color: #000;\n" \
		"	text-decoration: none;\n" \
		"}\n" \
		"\n" \
		"a.a_intf_link:hover {\n" \
		"	text-decoration: underline;\n" \
		"}\n" \
		"\n" \
		"p.selection {\n" \
		"	text-align: center;\n" \
		"	margin: 0pt;\n" \
		"}\n" \
		"\n" \
		"table.intf_list {\n" \
		"	border: 1px solid #cdcdcd;\n" \
		"	width: 100%%;\n" \
		"	border-spacing: 1px;\n" \
		"	background: #f8f8f8;\n" \
		"	margin-bottom: 10px;\n" \
		"}\n" \
		"\n" \
		"tr.intf_list_hdr {\n" \
		"	font-weight: bold;\n" \
		"}\n" \
		"\n" \
		"th.intf_list_hdr_name {\n" \
		"	text-align: left;\n" \
		"	padding-left: 5px;\n" \
		"}\n" \
		"\n" \
		"td.intf_list_nr {\n" \
		"	border-top: 1px dashed #cdcdcd;\n" \
		"	text-align: center;\n" \
		"}\n" \
		"\n" \
		"td.intf_list_name {\n" \
		"	border-top: 1px dashed #cdcdcd;\n" \
		"	border-left: 1px dashed #cdcdcd;\n" \
		"	text-align: left;\n" \
		"	padding-left: 5px;\n" \
		"}\n" \
		"\n" \
		"td.intf_list_rx {\n" \
		"	border-top: 1px dashed #cdcdcd;\n" \
		"	border-left: 1px dashed #cdcdcd;\n" \
		"	text-align: center;\n" \
		"}\n" \
		"\n" \
		"td.intf_list_rxp {\n" \
		"	border-top: 1px dashed #cdcdcd;\n" \
		"	border-left: 1px dashed #cdcdcd;\n" \
		"	text-align: center;\n" \
		"}\n" \
		"\n" \
		"td.intf_list_tx {\n" \
		"	border-top: 1px dashed #cdcdcd;\n" \
		"	border-left: 1px dashed #cdcdcd;\n" \
		"	text-align: center;\n" \
		"}\n" \
		"\n" \
		"td.intf_list_txp {\n" \
		"	border-top: 1px dashed #cdcdcd;\n" \
		"	border-left: 1px dashed #cdcdcd;\n" \
		"	text-align: center;\n" \
		"}\n" \
		"	\n" \
		"table.details {\n" \
		"	border: 1px solid #cdcdcd;\n" \
		"	width: 100%%;\n" \
		"	border-spacing: 1px;\n" \
		"	background: #f8f8f8;\n" \
		"	margin-top: 10px;\n" \
		"}\n" \
		"\n" \
		"tr.details_hdr {\n" \
		"	font-weight: bold;\n" \
		"}\n" \
		"\n" \
		"th.details_hdr_name {\n" \
		"	text-align: left;\n" \
		"	padding-left: 20px;\n" \
		"}\n" \
		"\n" \
		"td.details_name {\n" \
		"	border-top: 1px dashed #cdcdcd;\n" \
		"	padding-left: 20px;\n" \
		"}\n" \
		"\n" \
		"td.details_rx {\n" \
		"	border-top: 1px dashed #cdcdcd;\n" \
		"	border-left: 1px dashed #cdcdcd;\n" \
		"	text-align: center;\n" \
		"}\n" \
		"\n" \
		"td.details_tx {\n" \
		"	border-top: 1px dashed #cdcdcd;\n" \
		"	border-left: 1px dashed #cdcdcd;\n" \
		"	text-align: center;\n" \
		"}\n" \
		"\n" \
		"table.overall {\n" \
		"	width: 750px;\n" \
		"	border-spacing: 0px;\n" \
		"}\n" \
		"\n" \
		"td.header {\n" \
		"	background: #b8c4db;\n" \
		"	text-align: center;\n" \
		"	border: 1px solid #000;\n" \
		"}\n" \
		"\n" \
		"td.left_col {\n" \
		"	background: #eeeeee;\n" \
		"	padding-left: 10px;\n" \
		"	border-left: 1px solid #cdcdcd;\n" \
		"	border-right: 1px solid #cdcdcd;\n" \
		"	border-bottom: 1px solid #cdcdcd;\n" \
		"	vertical-align: top;\n" \
		"	width: 130px;\n" \
		"}\n" \
		"\n" \
		"td.right_col {\n" \
		"	background: #fff;\n" \
		"	padding: 20px;\n" \
		"	padding-top: 10px;\n" \
		"	border-bottom: 1px solid #cdcdcd;\n" \
		"	border-right: 1px solid #cdcdcd;\n" \
		"	vertical-align: top;\n" \
		"}\n" \
		"\n" \
		"td.fg {\n" \
		"	background: #6a94b7;\n" \
		"}\n" \
		"\n" \
		"td.noise {\n" \
		"	background: #9fc5db;\n" \
		"}\n" \
		"\n" \
		"td.bg {\n" \
		"	background: #eeeeee;\n" \
		"}\n" \
		"\n" \
		"td.unknown {\n" \
		"	background: #f69191;\n" \
		"}\n" \
		"\n" \
		"table.graph {\n" \
		"	border: 1px solid #cdcdcd;\n" \
		"	width: 100%%;\n" \
		"	border-spacing: 1px;\n" \
		"	background: #f8f8f8;\n" \
		"	margin-top: 10px;\n" \
		"	table-layout: fixed;\n" \
		"	empty-cells: show;\n" \
		"	padding-right: 5px;\n" \
		"	padding-bottom: 5px;\n" \
		"}\n" \
		"\n" \
		"th.graph_hdr {\n" \
		"}\n" \
		"\n" \
		"th.scale_hdr {\n" \
		"	text-align: right;\n" \
		"	width: 60px;\n" \
		"}\n" \
		"\n" \
		"td.scale {\n" \
		"	text-align: right;\n" \
		"	vertical-align: top;\n" \
		"}\n" \
		"\n" \
		"tr.graph_row {\n" \
		"	line-height: 7px;\n" \
		"}\n" \
		"\n" \
		"ul.node_list {\n" \
		"	padding-left: 15px;\n" \
		"}\n" \
		"\n" \
		"ul.node_intf_list {\n" \
		"	padding-left: 8px;\n" \
		"}\n" \
		"\n" \
		"ul.sub_intf {\n" \
		"	padding-left: 8px;\n" \
		"}\n" \
		"\n" \
		"tr.legend {\n" \
		"	line-height: 10px;\n" \
		"}\n" \
		"\n" \
		"td.legend {\n" \
		"	font-size: 8pt;\n" \
		"}\n", PACKAGE_STRING);
	fclose(f);
}

static void list_add_child(struct item *item, void *arg)
{
	FILE *fd = arg;

	fprintf(fd, "<li><a class=\"a_intf_link\" href=\"%s.%s.%s.bytes.s.html\">%s</a>\n",
	    item->i_node->n_name, item->i_group->g_name, item->i_name, item->i_name);

	if (item->i_childs) {
		fprintf(fd, "<ul class=\"sub_intf\">");
		foreach_child(item->i_group, item, list_add_child, arg);
		fprintf(fd, "</ul>\n");
	}

	fprintf(fd, "</li>\n");
}

static void list_add_item(struct item *item, void *arg)
{
	FILE *fd = arg;

	if (item->i_parent)
		return;

	fprintf(fd, "<li><a class=\"a_intf_link\" href=\"%s.%s.%s.bytes.s.html\">%s</a>\n",
	    item->i_node->n_name, item->i_group->g_name, item->i_name, item->i_name);

	if (item->i_childs) {
		fprintf(fd, "<ul class=\"sub_intf\">");
		foreach_child(item->i_group, item, list_add_child, arg);
		fprintf(fd, "</ul>\n");
	}

	fprintf(fd, "</li>\n");
}

static void list_add_group(struct item_group *group, void *arg)
{
	foreach_item(group, list_add_item, arg);
}

static void list_add(struct node *node, void *arg)
{
	FILE *fd = (FILE *) arg;

	fprintf(fd, "<li><a class=\"a_node_link\" href=\"%s.html\">%s%s</a>\n",
		node->n_name, node->n_name, !c_hide_item ? ":" : "");

	if (!c_hide_item) {
		fprintf(fd, "<ul class=\"node_intf_list\">\n");
		foreach_group(node, list_add_group, fd);
		fprintf(fd, "</ul>\n");
	}

	fprintf(fd, "</li>\n");
}

static void write_header(FILE *fd, const char *title)
{
	fprintf(fd,
		"<!DOCTYPE html PUBLIC \"-//W3c/DTD XHTML 1.0 Strict//EN\" \n"        \
		"    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"        \
		"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\"\n"          \
		"       xml:lang=\"en\">\n"                                           \
		"<head>\n"                                                            \
		"<title>%s</title>\n"                                                 \
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"layout.css\" />\n" \
		"</head>\n"                                                           \
		"<body>\n"                                                            \
		"<table class=\"overall\" align=\"center\">\n"                        \
		"<tr>\n"                                                              \
		"<td colspan=\"2\" class=\"header\">\n"                               \
		"<p class=\"banner\">%s</p>\n"                                      \
		"</td>\n"                                                             \
		"</tr><tr>\n"                                                         \
		"<td class=\"left_col\">\n" \
		"<ul class=\"node_list\">\n",
		title, c_title);

	foreach_node(list_add, fd);

	fprintf(fd,
		"</ul></td>\n"                                                         \
		"<td class=\"right_col\">\n");
}

static void write_footer(FILE *fd)
{
	time_t now = time(0);
	
	fprintf(fd,
		"</td>\n" \
		"</tr>\n" \
		"<tr>\n"                                                              \
		"<td colspan=\"2\" class=\"footer\">\n"                               \
		"<p class=\"p_footer\">Updated: %s</p>\n"                             \
		"</td>\n"                                                             \
		"</tr>\n"                                                         \
		"</table>\n" \
		"</body>\n" \
		"</html>\n",
		asctime(localtime(&now)));
}

static void write_interface_entry(FILE *fd, struct item *intf, struct node *node)
{
	struct stat_attr *bytes, *packets;
	int i, rxprec, txprec;
	double rx, tx;
	char *rx_u, *tx_u;
	
	bytes = lookup_attr(intf, intf->i_major_attr);
	packets = lookup_attr(intf, intf->i_minor_attr);

	if (!bytes || !packets)
		return;

	rx = cancel_down(attr_get_rx_rate(bytes), bytes->a_unit, &rx_u, &rxprec);
	tx = cancel_down(attr_get_tx_rate(bytes), bytes->a_unit, &tx_u, &txprec);

	fprintf(fd,
		"<tr class=\"intf_list_row\">\n" \
		"<td class=\"intf_list_name\">");

	for (i = 0; i < intf->i_level; i++)
		fprintf(fd, "&nbsp;&nbsp;");

	fprintf(fd,
		"<a class=\"a_intf\" href=\"%s.%s.%s.bytes.s.html\">%s</a></td>\n" \
		"<td class=\"intf_list_rx\">%.*f %s</td>\n" \
		"<td class=\"intf_list_rxp\">%u</td>\n" \
		"<td class=\"intf_list_tx\">%.*f %s</td>\n" \
		"<td class=\"intf_list_txp\">%u</td>\n" \
		"</tr>\n",
		node->n_name, intf->i_group->g_name, intf->i_name, intf->i_name,
		rxprec, rx, rx_u, attr_get_rx_rate(packets),
		txprec, tx, tx_u, attr_get_tx_rate(packets));
}

static void handle_child(struct item *item, void *arg)
{
	write_interface_entry(arg, item, item->i_node);
	foreach_child(item->i_group, item, handle_child, arg);
}

static void add_to_interace_list(FILE *fd, struct item *item)
{
	if (item->i_parent)
		return;

	write_interface_entry(fd, item, item->i_node);
	foreach_child(item->i_group, item, handle_child, fd);
}

static void add_group_to_list(struct item_group *group, void *arg)
{
	struct item *item;
	FILE *fd = arg;

	for (item = group->g_items; item; item = item->i_next)
		if (item->i_name[0])
			add_to_interace_list(fd, item);

}

static void write_interface_list(FILE *fd, struct node *node)
{

	fprintf(fd,
		"<p class=\"struct nodeitle\">%s</p>\n" \
		"<table class=\"intf_list\">\n" \
		"<tr class=\"intf_list_hdr\">\n" \
		"<th class=\"intf_list_hdr_name\">Name</th>\n" \
		"<th class=\"intf_list_hdr_rx\">RX</th>\n" \
		"<th class=\"intf_list_hdr_rxp\">#</th>\n" \
		"<th class=\"intf_list_hdr_tx\">TX</th>\n" \
		"<th class=\"intf_list_hdr_txp\">#</th>\n" \
		"</tr>", 
		node->n_name);

	foreach_group(node, add_group_to_list, fd);

	fprintf(fd,
		"</table>\n");
}

static void write_graph(FILE *fd, struct node *node, struct item *intf,
			struct stat_attr_hist *h, struct hist_elem *e, const char *x_unit)
{
	int i, w, rem;
	struct graph *g = create_graph(e, c_graph_height, h->a_unit);

	fprintf(fd,
		"<p class=\"selection\">");

	for (i = 0; i < ATTR_HASH_MAX; i++) {
		struct stat_attr *a;
		for (a = intf->i_attrs[i]; a; a = a->a_next) {
			if (!(a->a_flags & ATTR_FLAG_HISTORY))
				continue;
			fprintf(fd,
				"[<a class=\"a_selection\" href=\"%s.%s.%s.%s.s.html\">%s</a>] ",
				node->n_name, intf->i_group->g_name, intf->i_name,
				a->sa_attr->a_name, a->sa_attr->a_desc);
		}
	}
	
	fprintf(fd,
		"</p><p class=\"selection\">[");

	if (get_read_interval() != 1.0f)
		fprintf(fd, 
			"<a class=\"a_selection\" href=\"%s.%s.%s.%s.r.html\">Read interval</a> ] [",
				node->n_name, intf->i_group->g_name, intf->i_name,
				h->sa_attr->a_name);

	fprintf(fd, 
		"<a class=\"a_selection\" href=\"%s.%s.%s.%s.s.html\">Seconds</a>] [" \
		"<a class=\"a_selection\" href=\"%s.%s.%s.%s.m.html\">Minutes</a>] [" \
		"<a class=\"a_selection\" href=\"%s.%s.%s.%s.h.html\">Hours</a>] [" \
		"<a class=\"a_selection\" href=\"%s.%s.%s.%s.d.html\">Days</a>]</p>\n",
		node->n_name, intf->i_group->g_name, intf->i_name, h->sa_attr->a_name,
		node->n_name, intf->i_group->g_name, intf->i_name, h->sa_attr->a_name,
		node->n_name, intf->i_group->g_name, intf->i_name, h->sa_attr->a_name,
		node->n_name, intf->i_group->g_name, intf->i_name, h->sa_attr->a_name);
	


	fprintf(fd, "<table class=\"graph\">\n");

	fprintf(fd, "<tr><th class=\"scale_hdr\">%s</th>" \
	    "<th class=\"graph_hdr\" colspan=\"61\">RX %s</th></tr>\n",
	    g->g_tx.t_y_unit, h->sa_attr->a_desc);

	for (rem = 1, w = (c_graph_height - 1); w >= 0; w--) {
		char *p;
		fprintf(fd, "<tr class=\"graph_row\">\n");

		if (--rem == 0) {
			fprintf(fd, "<td class=\"scale\" rowspan=\"%d\">%8.2f</td>",
			    c_rowspan, g->g_rx.t_y_scale[w]);
			rem = c_rowspan;
		}

		for (p = (char *) (g->g_rx.t_data + (w * (HISTORY_SIZE + 1))); *p; p++) {
			if (*p == get_fg_char())
				fprintf(fd, "<td class=\"fg\"></td>");
			else if (*p == get_noise_char())
				fprintf(fd, "<td class=\"noise\"></td>");
			else if (*p == get_unk_char())
				fprintf(fd, "<td class=\"unknown\"></td>");
			else
				fprintf(fd, "<td class=\"bg\"></td>");
		}
		fprintf(fd, "<td>&nbsp;</td></tr>");
	}

	fprintf(fd, "<tr><td>&nbsp;</td>" \
	    "<td colspan=\"5\">1</td>" \
	    "<td colspan=\"5\">5</td>" \
	    "<td colspan=\"5\">10</td>" \
	    "<td colspan=\"5\">15</td>" \
	    "<td colspan=\"5\">20</td>" \
	    "<td colspan=\"5\">25</td>" \
	    "<td colspan=\"5\">30</td>" \
	    "<td colspan=\"5\">35</td>" \
	    "<td colspan=\"5\">40</td>" \
	    "<td colspan=\"5\">45</td>" \
	    "<td colspan=\"5\">50</td>" \
	    "<td colspan=\"5\">55</td>" \
	    "<td>%s</td></tr>\n" \
	    "  <tr class=\"legend\">\n" \
	    "    <td colspan=\"7\"></td>\n" \
	    "    <td colspan=\"2\" class=\"fg\">&nbsp;</td>\n" \
	    "    <td colspan=\"15\" class=\"legend\">Consumed</td>\n" \
	    "    <td colspan=\"2\" class=\"noise\">&nbsp;</td>\n" \
	    "    <td colspan=\"15\" class=\"legend\">Noise</td>\n" \
	    "    <td colspan=\"2\" class=\"unknown\">&nbsp;</td>\n" \
	    "    <td colspan=\"15\" class=\"legend\">No input data</td>\n" \
	    "    <td colspan=\"4\"></td>\n" \
	    "  </tr>\n" \
	    "</table>\n", x_unit);
	
	fprintf(fd, "<table class=\"graph\">\n");
	fprintf(fd, "<tr><th class=\"scale_hdr\">%s</th>" \
	    "<th class=\"graph_hdr\" colspan=\"61\">TX %s</th></tr>\n",
	    g->g_tx.t_y_unit, h->sa_attr->a_desc);
	
	for (rem = 1, w = (c_graph_height - 1); w >= 0; w--) {
		char *p;
		fprintf(fd, "<tr class=\"graph_row\">\n");

		if (--rem == 0) {
			fprintf(fd, "<td class=\"scale\" rowspan=\"%d\">%8.2f</td>",
			    c_rowspan, g->g_tx.t_y_scale[w]);
			rem = c_rowspan;
		}
		for (p = (char *) (g->g_tx.t_data + (w * (HISTORY_SIZE + 1))); *p; p++) {
			if (*p == get_fg_char())
				fprintf(fd, "<td class=\"fg\"></td>");
			else if (*p == get_noise_char())
				fprintf(fd, "<td class=\"noise\"></td>");
			else
				fprintf(fd, "<td class=\"bg\"></td>");
		}
		fprintf(fd, "<td>&nbsp;</td></tr>");
	}
	fprintf(fd, "<tr><td>&nbsp;</td>" \
	    "<td colspan=\"5\">1</td>" \
	    "<td colspan=\"5\">5</td>" \
	    "<td colspan=\"5\">10</td>" \
	    "<td colspan=\"5\">15</td>" \
	    "<td colspan=\"5\">20</td>" \
	    "<td colspan=\"5\">25</td>" \
	    "<td colspan=\"5\">30</td>" \
	    "<td colspan=\"5\">35</td>" \
	    "<td colspan=\"5\">40</td>" \
	    "<td colspan=\"5\">45</td>" \
	    "<td colspan=\"5\">50</td>" \
	    "<td colspan=\"5\">55</td>" \
	    "<td>%s</td></tr>\n" \
	    "  <tr class=\"legend\">\n" \
	    "    <td colspan=\"7\"></td>\n" \
	    "    <td colspan=\"2\" class=\"fg\">&nbsp;</td>\n" \
	    "    <td colspan=\"15\" class=\"legend\">Consumed</td>\n" \
	    "    <td colspan=\"2\" class=\"noise\">&nbsp;</td>\n" \
	    "    <td colspan=\"15\" class=\"legend\">Noise</td>\n" \
	    "    <td colspan=\"2\" class=\"unknown\">&nbsp;</td>\n" \
	    "    <td colspan=\"15\" class=\"legend\">No input data</td>\n" \
	    "    <td colspan=\"4\"></td>\n" \
	    "  </tr>\n" \
	    "</table>\n", x_unit);

	free_graph(g);
}

static void print_attr_detail(struct stat_attr *a, void *arg)
{
	double rx, tx;
	char *rx_u, *tx_u;
	int rxprec, txprec;
	FILE *fd = (FILE *) arg;

	rx = cancel_down(attr_get_rx(a), a->a_unit, &rx_u, &rxprec);
	tx = cancel_down(attr_get_tx(a), a->a_unit, &tx_u, &txprec);

	fprintf(fd,
		"<tr class=\"details\">\n" \
		"<td class=\"details_name\">%s</td>\n" \
		"<td class=\"details_rx\">%.*f %s</td>\n" \
		"<td class=\"details_tx\">%.*f %s</td>\n" \
		"</tr>\n",
		a->sa_attr->a_desc, rxprec, rx, rx_u, txprec, tx, tx_u);
}

static void write_details(FILE *fd, struct item *intf)
{
	
	fprintf(fd,
		"<table class=\"details\">\n" \
		"<tr class=\"details_hdr\">\n" \
		"<th class=\"details_hdr_name\">Details</th>\n" \
		"<th class=\"details_hdr_rx\">RX</th>\n" \
		"<th class=\"details_hdr_tx\">TX</th>\n" \
		"</tr>\n");
	
	foreach_attr(intf, print_attr_detail, (void *) fd);

	fprintf(fd, "</table>\n");
}

static void __write_per_item(struct item *intf, struct node *node,
			     struct stat_attr_hist *h,
			     struct hist_elem *e, char *x_unit)
{
	char outf[FILENAME_MAX];
	char title[256];
	FILE *fd;

	snprintf(outf, sizeof(outf), "%s/%s.%s.%s.%s.%s.html",
		c_path, node->n_name, intf->i_group->g_name, intf->i_name,
		h->sa_attr->a_name, x_unit);
	snprintf(title, sizeof(title), "%s on %s - %s/%s",
	    intf->i_name, node->n_name, h->sa_attr->a_desc, x_unit);
	fd = open_file(outf);
	write_header(fd, title);

	write_interface_list(fd, node);
	fprintf(fd, "<p class=\"title\">%s</p>\n", intf->i_name);
	write_graph(fd, node, intf, h, e, x_unit);
	write_details(fd, intf);
	
	write_footer(fd);
	fclose(fd);
}

static void write_attr_graph(struct stat_attr *a, void *arg)
{
	struct item *item = arg;
	struct node *node = item->i_node;
	struct stat_attr_hist *h = (struct stat_attr_hist *) a;

	if (!(a->a_flags & ATTR_FLAG_HISTORY))
		return;

	if (get_read_interval() != 1.0f)
		__write_per_item(item, node, h, &h->a_hist.h_read, "r");
	__write_per_item(item, node, h, &h->a_hist.h_sec, "s");
	__write_per_item(item, node, h, &h->a_hist.h_min, "m");
	__write_per_item(item, node, h, &h->a_hist.h_hour, "h");
	__write_per_item(item, node, h, &h->a_hist.h_day, "d");
}

static void write_per_item(struct item *item, void *arg)
{
	foreach_attr(item, &write_attr_graph, item);
}

static void write_per_group(struct item_group *group, void *arg)
{
	foreach_item(group, write_per_item, NULL);
}

static void write_per_node(struct node *node, void *arg)
{
	char outf[FILENAME_MAX];
	FILE *fd;

	snprintf(outf, sizeof(outf), "%s/%s.html", c_path, node->n_name);
	fd = open_file(outf);
	write_header(fd, node->n_name);
	write_interface_list(fd, node);
	write_footer(fd);
	fclose(fd);

	foreach_group(node, write_per_group, NULL);
}

void html_draw(void)
{
	static int rem = 1;
	char outf[FILENAME_MAX];
	FILE *fd;

	if (--rem)
		return;
	else
		rem = c_update_interval;

	umask(0133);
	write_css(c_path);
	
	foreach_node(write_per_node, NULL);

	snprintf(outf, sizeof(outf), "%s/index.html", c_path);
	fd = open_file(outf);
	write_header(fd, c_title);
	write_footer(fd);
	fclose(fd);
}

static void print_module_help(void)
{
	printf(
		"HTML - HTML Output\n" \
		"\n" \
		"  Lightweight HTML output with CSS configuration.\n" \
		"  Author: Thomas Graf <tgraf@suug.ch>\n" \
		"\n" \
		"  Options:\n" \
		"    path=PATH        Output directory\n" \
		"    interval=SEC     Update interval in seconds (default: 1)\n" \
		"    rowspan=NUM      Summarize NUM rows into into a single scale step\n" \
		"    height=NUM       Height of graphical statistics (default: 6)\n" \
		"    hideitems        Hide interfaces in node list\n" \
		"    title=STRING     Title of output\n");
}

static void html_set_opts(tv_t *attrs)
{
	while (attrs) {
		if (!strcasecmp(attrs->type, "path") && attrs->value)
			c_path = attrs->value;
		else if (!strcasecmp(attrs->type, "height") && attrs->value)
			c_graph_height = strtol(attrs->value, NULL, 0);
		else if (!strcasecmp(attrs->type, "interval") && attrs->value)
			c_update_interval = strtol(attrs->value, NULL, 0);
		else if (!strcasecmp(attrs->type, "rowspan") && attrs->value)
			c_rowspan = strtol(attrs->value, NULL, 0);
		else if (!strcasecmp(attrs->type, "title") && attrs->value)
			c_title = attrs->value;
		else if (!strcasecmp(attrs->type, "hideitems"))
			c_hide_item = 1;
		else if (!strcasecmp(attrs->type, "help")) {
			print_module_help();
			exit(0);
		}
		
		attrs = attrs->next;
	}
}

static int html_probe(void)
{
	if (NULL == c_path)
		quit("You must specify a path (-O html:path=DIR)\n");

	return 1;
}

static struct output_module html_ops = {
	.om_name = "html",
	.om_draw = html_draw,
	.om_set_opts = html_set_opts,
	.om_probe = html_probe,
};

static void __init html_init(void)
{
	register_secondary_output_module(&html_ops);
}
