/*
 * output.h               Output API
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

#ifndef __BMON_OUTPUT_H_
#define __BMON_OUTPUT_H_

#include <bmon/bmon.h>
#include <bmon/conf.h>

struct output_module
{
	char *                 om_name;
	void		      (*om_init)(void);
	void		      (*om_parse_opt)(const char *, const char *);
	int		      (*om_probe)(void);
	void		      (*om_pre)(void);
	void		      (*om_draw)(void);
	void		      (*om_post)(void);
	void		      (*om_shutdown)(void);
	int			om_enable;

	struct list_head	om_list;
};

extern void		output_register(struct output_module *);
extern void		output_register_secondary(struct output_module *);
extern void		output_set(const char *);
extern void		output_set_secondary(const char *);
extern void		output_init(void);
extern void		output_pre(void);
extern void		output_draw(void);
extern void		output_post(void);
extern void		output_shutdown(void);

#endif
