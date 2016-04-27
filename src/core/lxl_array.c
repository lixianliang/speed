
/*
 * Copyright (C) xianliang.li
 */


//#include <lxl_array.h>
#include <lxl_config.h>
#include <lxl_core.h>


lxl_array_t *
lxl_array_create(lxl_pool_t *p, lxl_uint_t n, size_t size)
{
	lxl_array_t *a = lxl_palloc(p, sizeof(lxl_array_t));
	if (a == NULL) {
		return NULL;
	}

	if (lxl_array_init(a, p, n, size) != 0) {
		return NULL;
	}

	return a;
}


lxl_array1_t *
lxl_array1_create(lxl_uint_t n, size_t size)
{
	lxl_array1_t *a = lxl_alloc(sizeof(lxl_array1_t));
	if (a == NULL) {
		return NULL;
	}

	if (lxl_array1_init(a, n, size) != 0) {
		return NULL;
	}

	return a;
}
