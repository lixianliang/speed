
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_dfst.h>


lxl_list_t    lxl_dfst_idc_list;
lxl_array1_t  lxl_dfst_area_array;
lxl_array1_t  lxl_dfst_idc_array;
lxl_file_t	  lxl_dfst_storages_binlog_file;


static int lxl_dfst_storage_cmp(void *elt1, void *elt2);


int 
lxl_dfst_storage_add(lxl_dfs_storage_state_t *sstate, lxl_int_t binlog)
{
	char				 buffer[16];
	uint16_t			 flags, rtype = 0x0100;
	lxl_uint_t 			 i, n;
	struct in_addr       addr;
	lxl_dfst_idc_t      *idc, *idc1 = NULL;
	lxl_dfst_storage_t  *storage;
	
	n = lxl_array1_nelts(&lxl_dfst_idc_array);
	for (i = 0; i < n; ++i) {
		idc = lxl_array1_data(&lxl_dfst_idc_array, lxl_dfst_idc_t, i);
		if (idc->id == sstate->idc_id) {
			idc1 = idc;
			break;
		}
	}

	if (idc1 == NULL) {
		idc = lxl_array1_push(&lxl_dfst_idc_array);
		if (idc == NULL) {
			return -1;
		}

		idc->id = sstate->idc_id;
		//idc->sort = 0;

		if (lxl_array1_init(&idc->storages, 1024, sizeof(lxl_dfst_storage_t)) == -1) {
			return -1;
		}

		lxl_log_error(LXL_LOG_INFO, 0, "dfst add idc %hu", idc->id);

		storage = lxl_array1_push(&idc->storages);
		if (storage == NULL) {
			return -1;
		}

		storage->state = 0;
		storage->port = sstate->port;
		storage->ip = sstate->ip;
		storage->loadavg = sstate->loadavg;
		storage->cpu_idle = sstate->cpu_idle;
		storage->disk_free_mb = sstate->disk_free_mb;

		addr.s_addr = sstate->ip;
		lxl_log_error(LXL_LOG_INFO, 0, "dfst add storage %hu %s:%hu", sstate->idc_id, inet_ntoa(addr), ntohs(sstate->port));

		if (binlog) {
			flags = rtype + sstate->idc_id >> 8; 
			lxl_memcpy_2byte(buffer, &flags);
			lxl_memcpy_2byte(buffer + 2, &sstate->port);
			lxl_memcpy_4byte(buffer + 4, &sstate->ip);

			if (lxl_write_file(&lxl_dfst_storages_binlog_file, buffer, 8) == -1) {
				return -1;
			}
		}

		return 0;
	}

	n = lxl_array1_nelts(&idc1->storages);
	for (i = 0; i < n; ++i) {
		storage = lxl_array1_data(&idc1->storages, lxl_dfst_storage_t, i);
		if (storage->port == sstate->port && storage->ip == sstate->ip) {
			storage->loadavg = sstate->loadavg;
			storage->cpu_idle = sstate->cpu_idle;
			storage->disk_free_mb = sstate->disk_free_mb;

			return 0;
		}
	}

	storage = lxl_array1_push(&idc1->storages);
	if (storage == NULL) {
		return -1;
	}

	storage->state = 0;
	storage->port = sstate->port;
	storage->ip = sstate->ip;
	storage->loadavg = sstate->loadavg;
	storage->cpu_idle = sstate->cpu_idle;
	storage->disk_free_mb = sstate->disk_free_mb;

	addr.s_addr = sstate->ip;
	lxl_log_error(LXL_LOG_INFO, 0, "dfst add storage %hu %s:%hu", 
				sstate->idc_id, inet_ntoa(addr), ntohs(sstate->port));

	if (binlog) {
		flags = rtype + sstate->idc_id >> 8;
		lxl_memcpy_2byte(buffer, &flags);
		lxl_memcpy_2byte(buffer + 2, &sstate->port);
		lxl_memcpy_4byte(buffer + 4, &sstate->ip);

		if (lxl_write_file(&lxl_dfst_storages_binlog_file, buffer, 10) == -1) {
			return -1;
		}
	}

	return 0;
}

int
lxl_dfst_storage_del(lxl_dfs_storage_state_t *sstate, lxl_int_t binlog)
{
	char				 buffer[16];
	uint16_t			 flags, rtype = 0x0000;
	lxl_uint_t	 		 i, n;
	lxl_dfst_idc_t  	*idc, *idc1 = NULL;
	lxl_dfst_storage_t  *storage;
	
	n = lxl_array1_nelts(&lxl_dfst_idc_array);
	for (i = 0; i < n; ++i) {
		idc = lxl_array1_data(&lxl_dfst_idc_array, lxl_dfst_idc_t, i);
		if (idc->id == sstate->idc_id) {
			idc1 = idc;
			break;
		}
	}

	if (idc1 == NULL) {
		lxl_log_error(LXL_LOG_INFO, 0, "dfst del storage idc %hu not find", sstate->idc_id);
		return 0;
	}

	n = lxl_array1_nelts(&idc1->storages);
	for (i = 0; i < n; ++i) {
		storage = lxl_array1_data(&idc1->storages, lxl_dfst_storage_t, i);
		if (storage->port == sstate->port && storage->ip == sstate->ip) {
			lxl_array1_del(&idc1->storages, i);

			if (binlog) {
				flags = rtype + sstate->idc_id >> 8;
				lxl_memcpy_2byte(buffer, &flags);
				lxl_memcpy_2byte(buffer + 2, &sstate->port);
				lxl_memcpy_4byte(buffer + 4, &sstate->ip);

				if (lxl_write_file(&lxl_dfst_storages_binlog_file, buffer, 10) == -1) {
					return -1;
				}
			}

			return 0;
		}
	}

	lxl_log_error(LXL_LOG_INFO, 0, "dfst del storage %hu %u not find", sstate->port, sstate->ip);

	return 0;
}

lxl_dfst_idc_t *
lxl_dfst_idc_find(uint16_t idc_id)
{
	lxl_uint_t        i, n;
	lxl_dfst_idc_t   *idc;

	n = lxl_array1_nelts(&lxl_dfst_idc_array);
	for (i = 0; i < n; ++i)	{
		idc = lxl_array1_data(&lxl_dfst_idc_array, lxl_dfst_idc_t, i);
		if (idc->id == idc_id) {
			return idc;
		}
	}

	return idc;
}

static int 
lxl_dfst_storage_cmp(void *elt1, void *elt2)
{
	lxl_dfst_storage_t  *e1 = elt1, *e2 = elt2;

	if (e1->port == e2->port && e1->ip == e2->ip) {
		return 0;
	}

	return -1;
}

/*int
lxl_dfst_idc_storage_add(lxl_dfs_idc_info_t *info)
{
	lxl_list_t 			*list;
	lxl_dfst_idc_t 	*idc;
	lxl_dfst_storage_t  *storage;

	idc = lxl_dfst_idc_find(info->idc_id);
	if (!idc) {
		idc = lxl_dfst_idc_add(info->idc_id);
		if (!idc) {
			return -1;
		}
	}

	storage = lxl_dfst_storage_find(idc, info->storage_id);
	if (storage) {
		lxl_log_error(LXL_LOG_INFO, 0, "dfst idc storage add, idc id: %hu, storage id: %hu exist", info->idc_id, info->storage_id);
		if (storage->ip != info->ip || storage->port != info->port) {
			lxl_log_error(LXL_LOG_WARN, 0, "dfst idc storage src ip:%u port:%hu, dst ip:%u port:%hu", storage->ip, storage->port, info->ip, info->port);
			return -1;
		}

		return 0;
	}

	storage = lxl_dfst_storage_add(idc, info);
	if (!storage) {
		return -1;
	}

	return 0;
}

lxl_dfst_idc_t *
lxl_dfst_idc_add(uint16_t id)
{
	lxl_dfst_idc_t  *idc;

	lxl_log_error(LXL_LOG_INFO, 0, "dfst add idc %hu", id);

	idc = lxl_alloc(sizeof(lxl_dfst_idc_t));
	if (idc == NULL) {
		return NULL;
	}

	idc->id = id;
	idc->number = 1;
	lxl_list_init(&idc->list);

	lxl_list_add_tail(&lxl_dfst_idc_list, &idc->list);

	return idc;
}

lxl_dfst_idc_t *
lxl_dfst_idc_find(uint16_t id)
{
	lxl_list_t       *list;
	lxl_dfst_idc_t  *idc;

	for (list = lxl_list_head(&lxl_dfst_idc_list);
		list != lxl_list_sentinel(&lxl_dfst_idc_list); list = lxl_list_next(list)) {
		idc = lxl_list_data(list, lxl_dfst_idc_t, list);
		if (idc->id == id) {
			return idc;
		} 
	}

	return NULL;
}

lxl_dfst_storage_t *
lxl_dfst_storage_add(lxl_dfst_idc_t *idc, lxl_dfs_storage_info_t *info)
{
	struct in_addr 	     addr;
	lxl_dfst_storage_t  *storage;

	addr.s_addr = info->ip;
	lxl_log_error(LXL_LOG_INFO, 0, "dfst add storage: %hu, %s:%hu", 
					info->idc_id, inet_ntoa(addr), ntohs(info->port));

	storage = lxl_alloc(sizeof(lxl_dfst_storage_t));
	if (storage == NULL) {
		return NULL;
	}

	++idc->number;
	idc->sort = 0; 
	storage->state = LXL_DFS_STORAGE_ONLINE;
	storage->port = info->port;
	storage->ip = info->ip;
	storage->cpu_idle = info->cpu_idle;
	storage->disk_free_mb = info->disk_free_mb;
	
	return storage;
}

lxl_dfst_storage_t *
lxl_dfst_storage_find(lxl_dfst_idc_t *idc, lxl_dfs_ip_port_t *ip_port)
{
	lxl_list_t  		*list;
	lxl_dfst_storage_t  *storage;

	for (list = lxl_list_head(&idc->list);
		list != lxl_list_sentinel(&idc->list); list = lxl_list_next(list)) {
		storage = lxl_list_data(list, lxl_dfst_storage_t, list);
		if (storage->ip == ip_port->ip && storage->port == ip_port->port) {
			return storage;
		}
	}

	return NULL;
}*/
