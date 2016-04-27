

/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_PROCESS_H_INCLUDE
#define LXL_PROCESS_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>


pid_t lxl_pid;


int	 lxl_init_signals(void);
void lxl_master_process_cycle(lxl_cycle_t *cycle);


#endif	/* LXL_PROCESS_H_INCLUDE */
