
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DFST_IDC_H_INCLUDE
#define LXL_DFST_IDC_H_INCLUDE


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


//int lxl_dfst_idc_storage_add(lxl_dfs_idc_info_t *info);
int lxl_dfst_storage_add(lxl_dfs_storage_state_t *sstate, lxl_int_t flags);
int	lxl_dfst_storage_del(lxl_dfs_storage_state_t *sstate, lxl_int_t flags);
//lxl_dfst_storage_t * lxl_dfst_storage_find(lxl_dfs_storage_t *storage);

lxl_dfst_idc_t * lxl_dfst_idc_find(uint16_t idc_id);
//lxl_dfst_idc_t *   lxl_dfst_idc_find(uint16_t id);
//lxl_dfst_storage_t * lxl_dfst_storage_add(lxl_dfst_idc_t *idc, lxl_dfs_storage_info_t *info);
//lxl_dfst_storage_t * lxl_dfst_storage_find(lxl_dfst_idc_t *idc, lxl_dfs_ip_port_t *ip_port);


#endif	/* LXL_DFST_IDC_H_INCLUDE */
