/*
 * in_null.c               Null Input Method
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

static void null_read(void)
{
	DBG(2, "null: reading...\n");
}

static void print_help(void)
{
	printf(
	"null - Do not collect statistics at all\n" \
	"\n" \
	"  Will not collect any statistics at all, used to disable \n" \
	"  local statistics collection.\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n");
}

static void null_parse_opt(const char *type, const char *value)
{
	if (!strcasecmp(type, "help")) {
		print_help();
		exit(0);
	}
}

static int null_probe(void)
{
	return 1;
}

static struct input_module null_ops = {
	.im_name		= "null",
	.im_read		= null_read,
	.im_parse_opt		= null_parse_opt,
	.im_probe		= null_probe,
	.im_no_default		= 1,
};

static void __init null_init(void)
{
	input_register(&null_ops);
}