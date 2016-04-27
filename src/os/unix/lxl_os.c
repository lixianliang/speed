
/*
 * Copyrigth (C) xianliang.li
 */


#include <lxl_config.h>
//#include <lxl_core.h>


lxl_int_t lxl_ncpu;


int
lxl_os_init()
{
	lxl_ncpu = sysconf(_SC_NPROCESSORS_ONLN);
	
	if (lxl_ncpu < 1) {
		lxl_ncpu = 1;
	}
	
	return 0;
}
