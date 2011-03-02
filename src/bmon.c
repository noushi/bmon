/*
 * src/bmon.c		   Bandwidth Monitor
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
#include <bmon/conf.h>
#include <bmon/utils.h>
#include <bmon/input.h>
#include <bmon/output.h>
#include <bmon/module.h>
#include <bmon/group.h>
#include <bmon/signal.h>

int start_time;
int do_quit = 0;
int is_daemon = 0;

static char *usage_text =
"Usage: bmon [OPTION]...\n" \
"\n" \
"Options:\n" \
"Startup:\n" \
"   -i, --input=MODPARM             Primary input module\n" \
"   -o, --output=MODPARM            Primary ouptut module\n" \
"   -I, --secondary-input=MODPARM   Secondary input modules\n" \
"   -O, --secondary-output=MODPARM  Secondary output modules\n" \
"   -f, --configfile=PATH           Alternative path to configuration file\n" \
"   -w, --wait-for-signal           Signal driven output intervals\n" \
"   -S, --send-signal=PID           Send SIGUSR1 to a running bmon instance\n" \
"   -d, --daemon                    Run as a daemon\n" \
"   -P, --pidfile=PATH              Path to the pidfile\n" \
"   -u, --uid=UID                   Drop privileges and change UID\n" \
"   -g, --gid=GID                   Drop privileges and change GID\n" \
"   -h, --help                      show this help text\n" \
"   -V, --version                   show version\n" \
"\n" \
"Input:\n" \
"   -p, --policy=POLICY             Interface acceptance policy\n" \
"   -a, --show-all                  Accept interfaces even if they are down\n" \
"   -r, --read-interval=FLOAT       Read interval in seconds\n" \
"   -L, --lifetime=LIFETIME         Lifetime of a item in seconds\n" \
"   -s, --sleep-interval=FLOAT      Sleep time in seconds\n" \
"\n" \
"Output:\n" \
"   -c, --use-si                    Use SI units\n" \
"\n" \
"Rate Estimation:\n" \
"   -R, --rate-interval=FLOAT       Rate interval in seconds\n" \
"\n" \
"Module configuration:\n" \
"   modparm := MODULE:optlist,MODULE:optlist,...\n" \
"   optlist := option;option;...\n" \
"   option  := TYPE[=VALUE]\n" \
"\n" \
"   Examples:\n" \
"       -o curses:ngraph=2\n" \
"       -o list            # Shows a list of available modules\n" \
"       -o curses:help     # Shows a help text for html module\n" \
"\n" \
"Interface selection:\n" \
"   policy  := [!]simple_regexp,[!]simple_regexp,...\n" \
"\n" \
"   Example: -p 'eth*,lo*,!eth1'\n" \
"\n" \
"Please see the bmon(1) man pages for full documentation.\n";

static char *uid, *gid;

static void do_shutdown(void)
{
	static int done;
	
	if (!done) {
		done = 1;
		module_shutdown();
		conf_shutdown();
	}
}

RETSIGTYPE sig_int(int unused)
{
	if (do_quit)
		exit(-1);
	do_quit = 1;
}

void sig_exit(void)
{
	do_shutdown();
}

void quit(const char *fmt, ...)
{
	static int done;
	va_list args;

	va_start(args, fmt);
	if (is_daemon)
		vsyslog(LOG_ERR, fmt, args);
	else
		vfprintf(stderr, fmt, args);
	va_end(args);

	if (!done) {
		done = 1;
		do_shutdown();
	}

	exit(1);
}

inline void xwarn(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

static inline void print_version(void)
{
	printf("bmon %s\n", PACKAGE_VERSION);
	printf("Copyright (C) 2001-2011 by Thomas Graf <tgraf@suug.ch>\n");
	printf("bmon comes with ABSOLUTELY NO WARRANTY. This is free " \
	       "software, and you\nare welcome to redistribute it under " \
	       "certain conditions. See the source\ncode for details.\n");
}

static void parse_args_pre(int argc, char *argv[])
{
	for (;;)
	{
		char *gostr = "+:hvVf:";

#ifdef HAVE_GETOPT_LONG
		struct option long_opts[] = {
			{"help", 0, 0, 'h'},
			{"version", 0, 0, 'v'},
			{"configfile", 1, 0, 'f'},
			{0, 0, 0, 0},
		};
		int c = getopt_long(argc, argv, gostr, long_opts, NULL);
#else
		int c = getopt(argc, argv, gostr;
#endif

		if (c == -1)
			break;
		
		switch (c)
		{
			case 'f':
				set_configfile(optarg);
				break;

			case 'h':
				print_version();
				printf("\n%s", usage_text);
				exit(1);
			case 'V':
			case 'v':
				print_version();
				exit(0);
		}
	}
}

static void parse_args_post(int argc, char *argv[])
{
	optind = 1;

	for (;;)
	{
		char *gostr = "i:I:o:O:p:r:s:S:P:wadcN:" \
			      "u:g:H:R:L:t:";

#ifdef HAVE_GETOPT_LONG
		struct option long_opts[] = {
			{"input", 1, 0, 'i'},
			{"secondary-input", 1, 0, 'I'},
			{"output", 1, 0, 'o'},
			{"secondary-output", 1, 0, 'O'},
			{"policy", 1, 0, 'p'},
			{"read-interval", 1, 0, 'r'},
			{"sleep-interval", 1, 0, 's'},
			{"rate-interval", 1, 0, 'R'},
			{"send-signal", 1, 0, 'S'},
			{"pidfile", 1, 0, 'P'},
			{"wait-for-signal", 0, 0, 'w'},
			{"show-all", 0, 0, 'a'},
			{"daemon", 0, 0, 'd'},
			{"attr-policy", 1, 0, 'A'},
			{"use-si", 0, 0, 'c'},
			{"num-graphs", 1, 0, 'N'},
			{"uid", 1, 0, 'u'},
			{"gid", 1, 0, 'g'},
			{"lifetime", 1, 0, 'L'},
			{0, 0, 0, 0},
		};
		int c = getopt_long(argc, argv, gostr, long_opts, NULL);
#else
		int c = getopt(argc, argv, gostr;
#endif

		if (c == -1)
			break;
		
		switch (c)
		{
			case 'i':
				input_set(optarg);
				break;

			case 'I':
				input_set_secondary(optarg);
				break;

			case 'o':
				output_set(optarg);
				break;

			case 'O':
				output_set_secondary(optarg);
				break;

			case 'r':
				cfg_setfloat(cfg, "read_interval", strtod(optarg, NULL));
				break;

			case 's':
				cfg_setint(cfg, "sleep_time", strtoul(optarg, NULL, 0));
				break;

			case 'S':
				signal_send(optarg);
				exit(0);

			case 'w':
				cfg_setint(cfg, "signal_driven", 1);
				break;

			case 'R':
				cfg_setfloat(cfg, "rate_interval", strtod(optarg, NULL));
				break;
			case 'a':
				cfg_setint(cfg, "show_all", 1);
				break;

			case 'p':
				cfg_setstr(cfg, "policy", optarg);
				break;

			case 'c':
				cfg_setbool(cfg, "use_si", cfg_true);
				break;

			case 'L':
				cfg_setint(cfg, "lifetime", strtoul(optarg, NULL, 0));
				break;

			case 'd':
				cfg_setbool(cfg, "daemon", cfg_true);
				break;

			case 'P':
				cfg_setstr(cfg, "pidfile", optarg);
				break;

			case 'u':
				cfg_setstr(cfg, "uid", optarg);
				break;

			case 'g':
				cfg_setstr(cfg, "gid", optarg);
				break;

		}
	}
}

#if 0
static void calc_variance(timestamp_t *c, timestamp_t *ri)
{
	float v = (ts_to_float(c) / ts_to_float(ri)) * 100.0f;

	rtiming.rt_variance.v_error = v;
	rtiming.rt_variance.v_total += v;

	if (v > rtiming.rt_variance.v_max)
		rtiming.rt_variance.v_max = v;

	if (v < rtiming.rt_variance.v_min)
		rtiming.rt_variance.v_min = v;
}
#endif

static void drop_privs(void)
{
	struct passwd *uentry = NULL;
	struct group *gentry = NULL;
	char *gid = cfg_getstr(cfg, "gid");
	char *uid = cfg_getstr(cfg, "uid");

	if (gid)
		gentry = getgrnam(gid);

	if (uid)
		uentry = getpwnam(uid);

	if (gentry)
		if (setgid(gentry->gr_gid) < 0)
			quit("Unable to set group id %d: %s\n",
			    gentry->gr_gid, strerror(errno));

	if (uentry)
		if (setuid(uentry->pw_uid) < 0)
			quit("Unable to set user id %d: %s\n",
			    uentry->pw_uid, strerror(errno));
}

static void daemonize(void)
{
#ifdef HAVE_DAEMON 
	if (daemon(0, 0) < 0)
		quit("Can't daemon(): %s\n", strerror(errno));
#else
	int devnull;
	pid_t pid;

	devnull = open("/dev/null", O_RDWR);

	if (devnull < 0)
		quit("Unable to open /dev/null: %s\n", strerror(errno));
		
	/* reopen stdin/stdout/stderr */
	if (dup2(devnull, 0) < 0) 
		quit("Can't dup /dev/null to STDIN: %s\n", strerror(errno));

	if (dup2(devnull, 1) < 0)  
		quit("Can't dup /dev/null to STDOUT: %s\n", strerror(errno));

	if (dup2(devnull, 2) < 0) 
		quit("Can't dup /dev/null to STDERR: %s\n", strerror(errno));

	if (chdir("/") < 0) 
		quit("Can't change into the root directory: %s\n",
		     strerror(errno));

	pid = fork();
	if (pid < 0) 
		quit("Can't fork: %s\n", strerror(errno));

	if (pid)  /* parent */
		exit(0);
	setsid();
#endif
	is_daemon = 1;
}

static void write_pidfile(void)
{
	char *pidfile = cfg_getstr(cfg, "pidfile");
	FILE *pf;

	if (!pidfile)
		return;

	pf = fopen(pidfile, "w");
	if (pf == NULL)
		quit("Can't open pidfile %s: %s\n", pidfile, strerror(errno));

	fprintf(pf, "%i\n", getpid());

	if (fclose(pf) < 0)
		quit("Can't close pidfile %s: %s\n", pidfile, strerror(errno));
}

static void init_syslog(void)
{
	openlog("bmon", LOG_CONS | LOG_PID, LOG_DAEMON);
}

int main(int argc, char *argv[])
{
	unsigned long sleep_time;
	double read_interval;
	

	start_time = time(0);

	parse_args_pre(argc, argv);
	configfile_read();
	parse_args_post(argc, argv);

	conf_init();
	module_init();

	read_interval = cfg_read_interval;
	sleep_time = cfg_getint(cfg, "sleep_time");

	if (((double) sleep_time / 1000000.0f) > read_interval)
		sleep_time = (unsigned long) (read_interval * 1000000.0f);

	// pipe_start();

	if (cfg_getbool(cfg, "daemon")) {
		init_syslog();
		daemonize();
		write_pidfile();
	}

	drop_privs();

	do {
		/*
		 * E  := Elapsed time
		 * NR := Next Read
		 * LR := Last Read
		 * RI := Read Interval
		 * ST := Sleep Time
		 * C  := Correction
		 */
		timestamp_t e, ri, tmp;
		unsigned long st;

		float_to_timestamp(&ri, read_interval);

		/*
		 * NR := NOW
		 */
		update_timestamp(&rtiming.rt_next_read);
		
		for (;;) {
			output_pre();

			/*
			 * read the chucka chucka pipe
			 */
			// pipe_handle();

			/*
			 * E := NOW
			 */
			update_timestamp(&e);

			/*
			 * IF NR <= E THEN
			 */
			if (timestamp_le(&rtiming.rt_next_read, &e)) {
				timestamp_t c;

				/*
				 * C :=  (NR - E)
				 */
				timestamp_sub(&c, &rtiming.rt_next_read, &e);

				//calc_variance(&c, &ri);

				/*
				 * LR := E
				 */
				copy_timestamp(&rtiming.rt_last_read, &e);

				/*
				 * NR := E + RI + C
				 */
				timestamp_add(&rtiming.rt_next_read, &e, &ri);
				timestamp_add(&rtiming.rt_next_read,
				       &rtiming.rt_next_read, &c);


				reset_update_flags();
				input_read();
				free_unused_elements();
				output_draw();
				output_post();
			}

			if (do_quit)
				exit(0);

			/*
			 * ST := Configured ST
			 */
			st = sleep_time;

			/*
			 * IF (NR - E) < ST THEN
			 */
			timestamp_sub(&tmp, &rtiming.rt_next_read, &e);

			if (tmp.tv_sec < 0)
				continue;

			if (tmp.tv_sec == 0 && tmp.tv_usec < st) {
				if (tmp.tv_usec < 0)
					continue;
				/*
				 * ST := (NR - E)
				 */
				st = tmp.tv_usec;
			}
			
			/*
			 * SLEEP(ST)
			 */
			usleep(st);
		}
	} while (0);

	return 0; /* buddha says i'll never be reached */
}

static void __init bmon_init(void)
{
	atexit(&sig_exit);
	signal(SIGINT, &sig_int);
}
