
/*
 * Copyright (c) xianliang.li
 */


#ifndef LXL_BUF_H_INCLUDE
#define LXL_BUF_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>


typedef struct {
	char 	   *pos;
	char 	   *last;
	char 	   *start;
	char 	   *end;
} lxl_buf_t;


lxl_buf_t *	lxl_create_temp_buf(lxl_pool_t *pool, size_t size);

#define lxl_alloc_buf(p)	lxl_palloc(p, sizeof(lxl_buf_t))
#define lxl_calloc_buf(p)	lxl_pcalloc(p, sizeof(lxl_buf_t))
#define lxl_reset_buf(b)	(b)->pos = (b)->start; (b)->last = (b)->start


static inline int 
lxl_buf_init(lxl_buf_t *buf, lxl_pool_t *p, size_t size)
{
	buf->start = lxl_palloc(p, size);
	if (buf->start == NULL) {
		return -1;
	}
		
	buf->pos = buf->start;
	buf->last = buf->start;
	buf->end = buf->start + size;

	return 0;
}


#endif	/* LXL_BUF_H_INCLUDE */
