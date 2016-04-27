
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
/*#include <lxl_log.h>
#include <lxl_times.h>
#include <lxl_cycle.h>
#include <lxl_event.h>
#include <lxl_process.h>*/


typedef struct {
	int		signo;
	char   *signame;
	char   *name;
	void   (*handler) (int signo);
} lxl_signal_t;


static void lxl_start_worker_process(lxl_cycle_t *cycle, lxl_int_t n);
static void lxl_worker_process_cycle(lxl_cycle_t *cycle, lxl_int_t worker);
static void lxl_worker_process_init(lxl_cycle_t *cycle, lxl_int_t worker);

static void lxl_signal_handler(int signo);
static void lxl_process_get_status(void);


lxl_uint_t lxl_process;

lxl_signal_t signals[] = {
	{ SIGCHLD, "SIGCHLD", "", lxl_signal_handler },
	
	{ 0, NULL, "", NULL}
};


int  
lxl_init_signals(void) 
{
	lxl_signal_t *sig;
	struct sigaction sa;
	
	for (sig = signals; sig->signo != 0; ++sig) {
		memset(&sa, 0x00, sizeof(sigaction));
		sa.sa_handler = sig->handler;
		sigemptyset(&sa.sa_mask);
		/* valgrind */
		if (sigaction(sig->signo, &sa, NULL) == -1) {
			lxl_log_error(LXL_LOG_EMERG, errno, "sigaction(%s) failed", sig->signame);
			return -1;
		}
	}

	return 0;
}

void 
lxl_master_process_cycle(lxl_cycle_t *cycle)
{
	lxl_core_conf_t *ccf;

	ccf = (lxl_core_conf_t *) lxl_get_conf(cycle->conf_ctx, lxl_core_module);
	lxl_start_worker_process(cycle, ccf->worker_process);
	lxl_msleep(100);
	
	for (; ;) {
		lxl_sleep(1);
	}
}

static void
lxl_start_worker_process(lxl_cycle_t *cycle, lxl_int_t n)
{
	pid_t pid;
	lxl_int_t i;
	
	lxl_log_error(LXL_LOG_INFO, 0, "start worker process");
	for (i = 0; i < n; ++i) {
		pid = fork();
		switch (pid) {
			case -1:
				lxl_log_error1(LXL_LOG_ALERT, errno, "fork() failed");
				return;

			case 0:
				lxl_pid = getpid();
				lxl_worker_process_cycle(cycle, i);

			default:
				break;
		}
	}
}

static void 
lxl_worker_process_cycle(lxl_cycle_t *cycle, lxl_int_t worker)
{
	lxl_worker_process_init(cycle, worker);
	
	for (; ;) {
		lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "worker cycle");
		lxl_process_events_and_timers(cycle);
	}
}


static void 
lxl_worker_process_init(lxl_cycle_t *cycle, lxl_int_t worker)
{
	lxl_uint_t i;
	struct rlimit rlmt, getrlmt;

	lxl_log_error(LXL_LOG_INFO, 0, "worker process init");

	rlmt.rlim_cur = (rlim_t) 1024 * 1024 * 100;	/* 100m bocks */
	rlmt.rlim_max = (rlim_t) 1024 * 1024 * 100;
	if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
		lxl_log_error1(LXL_LOG_ALERT, errno, "setrlimit(RLIMIT_CORE, %ld) failed", 1024 * 1024 * 100);
	}
	
	if (getrlimit(RLIMIT_CORE, &getrlmt) == -1) {
		lxl_log_error1(LXL_LOG_ERROR, errno, "getrlimit(RLIMIT_CORE) failed");
	} else {
		lxl_log_error1(LXL_LOG_INFO, errno, "getrlimit(RLIMIT_CORE), rlim_cur:%ld rlit_max:%ld", getrlmt.rlim_cur, getrlmt.rlim_max);
	}

	if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
		lxl_log_error1(LXL_LOG_ALERT, errno, "prctl(PR_SET_DUMPABLE) failed");
	}

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->init_process) {
			if (lxl_modules[i]->init_process(cycle) == -1) {
				/* fatal */
				lxl_log_error1(LXL_LOG_EMERG, 0, "lxl_modules[%lu] init process failed", i);
				exit(2);
			}
		}
	}
}

void 
lxl_signal_handler(int signo)
{
	int			  err;
	char 		 *action;
	lxl_signal_t *sig;

	
	err = errno;
	action = "";
	for (sig = signals; sig->signo != 0; ++sig) {
		if (sig->signo == signo) {
			break;
		}
	}

	// timeupdate
	lxl_time_update();
	
	switch (signo) {
	case SIGCHLD:
		//lxl_reap = 1;
		break;
	}

	lxl_log_error1(LXL_LOG_INFO, 0, "signal %d (%s) received%s", signo, sig->signame, action);
	if (signo == SIGCHLD) {
		lxl_process_get_status();
	}

	errno = err;
}

static void 
lxl_process_get_status(void)
{
	int one, status;
	pid_t pid;
	char *process;

	process = "worker process";
	one = 0;
	for (; ;) {
		pid = waitpid(-1, &status, WNOHANG);
		if (pid == 0) {
			return;
		}

		if (pid == -1) {
			if (errno == EINTR) {
				continue;
			}

			if (errno == ECHILD && one) {
				return;
			}

			if (errno == ECHILD) {
				lxl_log_error1(LXL_LOG_INFO, errno, "waitpid() failed");
				return;
			}

			lxl_log_error1(LXL_LOG_ALERT, errno, "waitpid() failed");
			return;
		}

		one = 1;
		if (WTERMSIG(status)) {
			/* wcoredump */
			//lxl_log_error1(LXL_LOG_ALERT, 0, "%s %d exited on signal %d", process, pid, WTERMSIG(status));
			lxl_log_error1(LXL_LOG_ALERT, 0, "%s %d exited on signal %d%s", process, pid, WTERMSIG(status),
							WCOREDUMP(status) ? " (core dumped)" : "");
		} else {
			lxl_log_error1(LXL_LOG_INFO, 0, "%s %d exited with code %d", process, pid, WEXITSTATUS(status));
		}

		if (WEXITSTATUS(status) == 2) {
			lxl_log_error1(LXL_LOG_ALERT, 0, "%s %d exited with fatal code %d and cannot be respawned", process, pid, WEXITSTATUS(status));
		}
	}
}
