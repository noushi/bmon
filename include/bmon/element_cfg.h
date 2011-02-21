/*
 * element_cfg.h	Element Configuration
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

#ifndef __BMON_ELEMENT_CFG_H_
#define __BMON_ELEMENT_CFG_H_

#include <bmon/bmon.h>
#include <bmon/conf.h>

#define ELEMENT_CFG_SHOW	(1 << 0)
#define ELEMENT_CFG_HIDE	(1 << 1)

struct element_cfg
{
	char *			ec_name;
	char *			ec_parent;
	char *			ec_description;
	uint64_t		ec_rxmax;
	uint64_t		ec_txmax;
	unsigned int		ec_flags;

	struct list_head	ec_list;
};

extern struct element_cfg *	element_cfg_alloc(const char *);
extern void			element_cfg_free(struct element_cfg *);
extern struct element_cfg *	element_cfg_lookup(const char *);

#endif
