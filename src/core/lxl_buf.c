
/*
 * Copyright (C) xianliang.li
 */


//#include <lxl_buf.h>
#include <lxl_config.h>
#include <lxl_core.h>



lxl_buf_t *
lxl_create_temp_buf(lxl_pool_t *pool, size_t size)
{
	lxl_buf_t *b;

	b = lxl_alloc_buf(pool);
	if (b == NULL) {
		return NULL;
	}

	b->start = lxl_palloc(pool, size);
	if (b->start == NULL) {
		return NULL;
	}

	b->pos = b->start;
	b->last = b->start;
	b->end =  b->start + size;

	return b;
}
