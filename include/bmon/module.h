/*
 * module.h               Module API
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

#ifndef __BMON_MODULE_H_
#define __BMON_MODULE_H_

#include <bmon/bmon.h>
#include <bmon/conf.h>

typedef enum mod_type_e {
	BMON_PRIMARY_MODULE,
	BMON_SECONDARY_MODULE,
} mod_type_t;

#define BMON_MODULE_ENABLED		1
#define BMON_MODULE_NO_DEFAULT		2

struct bmon_module
{
	char *			m_name;
	mod_type_t		m_type;

	void		      (*m_init)(void);
	int		      (*m_probe)(void);
	void		      (*m_shutdown)(void);

	void		      (*m_parse_opt)(const char *, const char *);
	void		      (*m_pre)(void);
	void		      (*m_do)(void);
	void		      (*m_post)(void);

	int			m_flags;
	struct list_head	m_list;
};

struct bmon_subsys
{
	char *			s_name;
	struct bmon_module *	s_primary;
	struct list_head	s_primary_list;
	struct list_head	s_secondary_list;

	void		      (*s_find_primary)(void);

	struct list_head	s_list;
};

extern void module_foreach(struct bmon_subsys *, void (*cb)(struct bmon_module *));

extern struct bmon_module *module_lookup(const char *, struct list_head *);
extern void module_register(struct bmon_subsys *, struct bmon_module *);
extern void module_set(struct bmon_subsys *, mod_type_t, const char *);

extern void		module_init(void);
extern void		module_shutdown(void);
extern void		module_register_subsys(struct bmon_subsys *);

#endif
