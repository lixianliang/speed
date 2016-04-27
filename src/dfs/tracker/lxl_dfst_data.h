
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DFST_DATA_H_INCLUDE
#define LXL_DFST_DATA_H_INCLUDE


#include <lxl_dfst.h>


#define LXL_DFST_BINLOG		1


typedef struct {
	uint16_t	  id;
	lxl_array1_t  storages;		/* lxl_dfst_storage_t */
} lxl_dfst_idc_t;

typedef struct {
	uint16_t      state;
	uint16_t	  port;
	uint32_t      ip;
	
	uint16_t	  loadavg;
	uint16_t	  cpu_idle;
	uint32_t	  disk_free_mb;		/* m */
} lxl_dfst_storage_t;

typedef struct {
	lxl_array1_t  storages;			/* lxl_dfs_ip_port1_t ===> lxl_dfs_storage_t */
} lxl_dfst_fid_t;


int	lxl_dfst_init_storages(void);
int	lxl_dfst_init_fids(void);

int lxl_dfst_storage_add(lxl_dfs_storage_state_t *sstate, lxl_int_t binlog);
int	lxl_dfst_storage_del(lxl_dfs_storage_state_t *sstate, lxl_int_t binlog);
int lxl_dfst_fid_add(lxl_dfs_fid_t *dfid, lxl_int_t binlog);
int lxl_dfst_fid_del(lxl_dfs_fid_t *dfid, lxl_int_t binlog);

lxl_dfst_idc_t * lxl_dfst_idc_find(uint16_t idc_id);


#endif	/* LXL_DFST_DATA_H_INCLUDE */
