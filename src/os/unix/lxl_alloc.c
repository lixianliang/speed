
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>


void *
lxl_alloc(size_t size) 
{
	void *p;
	
	p = malloc(size);
	if (p == NULL) {
		lxl_log_error(LXL_LOG_EMERG, errno, "malloc(%lu) failed", size);
		//p = NULL;
	}

	lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "malloc(%lu) %p", size, p); 

	return p;
}

void *
lxl_calloc(size_t n, size_t size) 
{
	void *p;

	p = calloc(n, size);
	if (p == NULL) {
		lxl_log_error(LXL_LOG_EMERG, errno, "calloc(%lu, %lu) failed", n, size);
		return NULL;
	}

	lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "calloc(%lu, %lu) %p", n, size, p); 

	return p;
}

void *
lxl_memalign(size_t alignment, size_t size)
{
	void *p;

	p = memalign(alignment, size);
	if (p == NULL) {
		lxl_log_error(LXL_LOG_EMERG, errno, "memalign(%lu, %lu) failed", alignment, size);
	}

	lxl_log_debug(LXL_LOG_DEBUG_ALLOC, 0, "memalign(%lu, %lu) %p", alignment, size, p); 

	return p;
}
