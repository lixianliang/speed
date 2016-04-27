
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_QUEUE_H_INCLUDE
#define LXL_QUEUE_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>


typedef struct {
	void 	    *elts;

	lxl_uint_t  front;
	lxl_uint_t  rear;
	lxl_uint_t  nalloc;
	size_t	    size;

	lxl_pool_t  *pool;
} lxl_queue_t;

typedef struct {
	void 	   *elts;

	lxl_uint_t	front;	
	lxl_uint_t  rear;
	lxl_uint_t	nalloc;
	size_t		size;
} lxl_queue1_t;


lxl_queue_t *  lxl_queue_create(lxl_pool_t *p, lxl_uint_t n, size_t size);
lxl_queue1_t * lxl_queue1_create(lxl_uint_t n, size_t size);
//void lxl_queue_destroy(lxl_queue_t *q);

/*#define lxl_queue_free(q)						\
		free((q)->elts);						\
		free(q);								\
		q = NULL*/

static inline lxl_int_t
lxl_queue_init(lxl_queue_t *q, lxl_pool_t *p, lxl_uint_t n, size_t size)
{
	/* one is empty, add */
	q->elts = lxl_palloc(p, (n + 1) * size);	
	if (q->elts == NULL) {
		return -1;
	}

	q->front = 0;
	q->rear = 0;
	q->nalloc = n + 1;
	q->size = size;
	q->pool = p;

	return 0;
}

#define lxl_queue_empty(q)		((q)->front == (q)->rear)
#define	lxl_queue_full(q)		((q)->front == ((q)->rear + 1) % (q)->nalloc)

static inline void *
lxl_queue_in(lxl_queue_t *q)
{
	u_char 		*ptr;
	void 		*elt, *new;
	size_t 		 size;
	lxl_uint_t   i;
	lxl_pool_t 	*p;

	if (lxl_queue_full(q)) {
		size  = q->nalloc * q->size;
		p = q->pool;
		if ((u_char *) q->elts + size == p->d.last && p->d.last + q->size <= p->d.end) {
			if (q->front > q->rear) {
				i = (q->nalloc - q->front) * q->size;
				ptr = (u_char *) q->elts + q->front * q->size;
				/*do {
					--i;
					*(ptr + i + q->size) = *(ptr + i);
				} while (i);*/

				memmove(ptr + q->size, ptr, i);
			}

			p->d.last += q->size;
			++q->nalloc;
		} else {
			new = lxl_palloc(p, 2 * size);
			if (new == NULL) {
				return NULL;
			}

			if (q->front > q->rear) {
				i = (q->nalloc - q->front) * q->size;
				memcpy(new, q->elts + q->front, i);
				memcpy((u_char *) new + i, q->elts, q->rear * q->size);
				q->front = 0;
				q->rear = q->nalloc - 1;
			} else {
				memcpy(new, q->elts, q->rear * q->size);
			}

			q->nalloc *= 2;
			q->elts = new;
		}
	}

	elt = (u_char *) q->elts + q->rear * q->size;
	++q->rear;
	/* q->rear %= q->nalloc; */

	return elt;
}

static inline void *	
lxl_queue_out(lxl_queue_t *q)
{
	void  *elt;

	if (lxl_queue_empty(q)) {
		return NULL;
	} 

	elt = (u_char *) q->elts + q->front * q->size;
	++q->front;
	q->front %= q->nalloc;

	return elt;
}
#if 0
static inline void
lxl_queue_clear(lxl_queue_t *q)
{
	q->front = 0;
    q->rear = 0;
}
#endif 
static inline int
lxl_queue1_init(lxl_queue1_t *q, lxl_uint_t n, size_t size)
{
	q->elts = lxl_alloc((n + 1) * size);
	if (q->elts == NULL) {
		return -1;
	}

	q->front = 0;
	q->rear = 0;
	q->nalloc = n + 1;
	q->size = size;

	return 0;
}

static inline void *
lxl_queue1_in(lxl_queue1_t *q)
{
	void 	   *elt, *new;
	size_t	 	size;
	lxl_uint_t  i;

	if (lxl_queue_full(q)) {
		size = q->nalloc * q->size;
		new = lxl_alloc(2 * size);
		if (new == NULL) {
			return NULL;
		}

		if (q->front > q->rear) {
			i = (q->nalloc - q->front) * q->size;
			memcpy(new, q->elts + q->front, i);
			memcpy((u_char *) new + i, q->elts, q->rear * q->size);
			q->front = 0;
			q->rear = q->nalloc - 1;
		} else {
			memcpy(new, q->elts, q->rear * q->size);
		}

		q->nalloc *= 2;
		q->elts = new;
	}

	elt = (u_char *) q->elts + q->rear * q->size;
	++q->rear;
	/* q->rear %= q->nalloc; */

	return elt;

}

static inline void *
lxl_queue1_out(lxl_queue1_t *q)
{
	void  *elt;

	if (lxl_queue_empty(q)) {
		return NULL;
	}

	elt = (u_char *) q->elts + q->front * q->size;
	++q->front;
	q->front %= q->nalloc;

	return elt;
}


#endif	/* LXL_QUEUE_H_INCLUDE */
