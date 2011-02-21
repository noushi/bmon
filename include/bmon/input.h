/*
 * input.h             Input API
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

#ifndef __BMON_INPUT_H_
#define __BMON_INPUT_H_

#include <bmon/bmon.h>
#include <bmon/conf.h>

struct input_module
{
	char *			im_name;
	void		      (*im_read)(void);
	void		      (*im_parse_opt)(const char *, const char *);
	int		      (*im_probe)(void);
	void		      (*im_init)(void);
	void		      (*im_shutdown)(void);
	int			im_no_default;
	int			im_enable;
	struct list_head	im_list;
};

extern void input_init(void);
extern void input_shutdown(void);

extern void input_set(const char *);
extern void input_set_secondary(const char *);

extern void input_register(struct input_module *);
extern void input_register_secondary(struct input_module *);

extern void input_read(void);

struct reader_timing
{
	timestamp_t rt_last_read;	/* timestamp taken before read */
	timestamp_t rt_next_read;	/* estimated next read */

	struct {
		float v_error;
		float v_max;
		float v_min;
		float v_total;
	} rt_variance;
};

extern struct reader_timing rtiming;

#endif
