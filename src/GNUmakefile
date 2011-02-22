# Makefile                bmon Makefile
#
# Copyright (c) 2001-2011 Thomas Graf <tgraf@suug.ch>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
ifeq ($(shell [ ! -r ../Makefile.opts ] && echo 1),)
    include ../Makefile.opts
endif

CIN := utils.c unit.c conf.c input.c output.c group.c element.c attr.c
CIN += signal.c element_cfg.c history.c graph.c bmon.c

# Primary input modules
CIN += in_null.c in_dummy.c

# Linux
CIN += in_proc.c

ifeq ($(NL),Yes)
CIN += in_netlink.c
endif

# SunOS
CIN += in_kstat.c

#CIN  += in_sysfs.c in_netstat.c in_sysctl.c

# Primary output modules
CIN += out_null.c out_format.c out_ascii.c

ifeq ($(CURSES),Yes)
CIN  += out_curses.c
endif

ifeq ($(RRD),Yes)
CIN  += out_rrd.c
endif

OBJ  := $(CIN:%.c=%.o)
OUT  := bmon

export

.PHONY: all clean install $(OUT)

all: $(OUT)

$(OUT): ../Makefile.opts $(OBJ)
	@echo "  LD $(OUT)"; \
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(OUT) $(OBJ) $(LDFLAGS) $(BMON_LIB)

clean:
	@echo "  CLEAN src"; \
	$(RM) -f $(OBJ) $(OUT)

distclean:
	@echo "  DISTCLEAN src"; \
	$(RM) -f .deps/*

$(DEPS): ../Makefile.opts

include ../Makefile.rules

