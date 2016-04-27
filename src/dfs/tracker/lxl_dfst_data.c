
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_dfst.h>


lxl_list_t    lxl_dfst_idc_list;
lxl_array1_t  lxl_dfst_area_array;
lxl_array1_t  lxl_dfst_idc_array;
lxl_file_t	  lxl_dfst_storages_binlog_file;
lxl_file_t	  lxl_dfst_fids_binlog_file;


int
lxl_dfst_storages_init()
{
	char					 name[64];
	size_t					 len, size;
	ssize_t					 n;
	uint16_t				 flags, rtype;
	lxl_uint_t  			 i, j, k, records = 0;
	lxl_buf_t  				 buffer, *b;
	lxl_dfs_storage_state_t  sstate;

	len = (size_t) snprintf(name, sizeof(name), "data/storages.binlog");
	lxl_dfst_storages_binlog_file_file.name.data = lxl_alloc(len + 1);
	if (lxl_dfst_storages_binlog_file.name.data == NULL) {
		return -1;
	}

	lxl_str_memcpy(&lxl_dfst_storages_binlog_file.name, name, len);
	
	b = &buffer;
	b->start = lxl_alloc(8192);
	if (b->start == NULL) {
		goto failed;
	}

	b->pos = b->start;
	b->last = b->pos;
	b->end = b->pos + 8192;

	lxl_dfst_storages_binlog_file.fd = open(lxl_dfst_storages_binlog_file.name.data, O_CREAT|O_SYNC|O_APPEND|O_RDWR, 0644);
	if (lxl_dfst_storages_binlog_file.fd == -1) {
		lxl_log_error(LXL_LOG_ERROR, errno, "open() %s failed", lxl_dfst_storages_binlog_file.name.data);
		goto failed;
	}

	memset(&sstate, 0x00, sizeof(sstate));
	for ( ;; ) {
		if (b->last == b->end) {
			size = b->last - b->pos;
			k = size % 8;
			j = size / 8;
			records += j;

			lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst idc servers %lu %lu", j, k);

			for (i = 0; i < j; ++i) {
				flags = *((uint16_t *) b->pos);
				b->pos += 2;
				rtype = flags && 0xff00;
				sstate.idc_id = flags && 0x00ff;
				sstate.port = *((uint16_t *) b->pos);
				b->pos += 2;
				sstate.ip = *((uint32_t *) b->pos);
				b->pos += 4;
			
				if (rtype == 1) {
					if (lxl_dfst_storage_add(&sstate, 0) == -1) {
						goto failed;
					}
				} else {
					if (lxl_dfst_storage_del(&sstate, 0) == -1) {
						goto failed;
					}
				}
			}

			if (k) {
				memmove(b->start, b->pos, k);		
				b->pos = b->start;
				b->last = b->pos + k;
			} else {
				lxl_reset_buf(b);
			}
		}

		size = b->end - b->last;
		n = lxl_read_file1(&lxl_dfst_storages_binlog_file, b->last, size);
		if (n == -1) {
			goto failed;
		}

		b->last += n;
		if (n < size) {
			break;
		}
	}

	size = b->last - b->pos;
	k = size % 8;
	j = size / 8;
	records += j;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst idc servers %lu %lu", j, k);

	if (k != 0) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfst idc server record len is 8");
		goto failed;
	}

	for (i = 0; i < j; ++i) {
		flags = *((uint16_t *) b->pos);
		b->pos += 2;
		rtype = flags && 0xff00;
		//idc_id = flags && 0x00ff;
		sstate.idc_id = flags && 0x00f;
		sstate.port = *((uint16_t *) b->pos);
		b->pos += 2;
		sstate.ip = *((uint32_t *) b->pos);
		b->pos += 4;

		if (tag == 1) {
			if (lxl_dfst_storage_add(&sstate, 0) == -1) {
				goto failed;
			}
		} else {
			if (lxl_dfst_storage_del(&sstate, 0) == -1) {
				goto failed;
			}
		}
	}

	if (records) {
		/* compaction */
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss idc servers compaction not implement %lu", records);
	}
	
	lxl_free(b->start);

	return 0;

failed:

	if (lxl_dfst_storages_binlog_file.name.data) {
		lxl_free(lxl_dfst_storages_binlog_file.name.data);
	}

	if (b->start) {
		lxl_free(b->start);
	}

	return -1;
}

int
lxl_dfst_fids_init()
{
	char			   name[64];
	size_t			   len, size;
	uint16_t		   flags, rtype;
	lxl_uint_t		   records = 0;
	lxl_buf_t		   buffer, *b;
	lxl_str_t	  	   fid;
	lxl_dfs_fid_t	   dfid;
	lxl_dfs_storage_t  storage, *elt;
	lxl_dfst_fid_t    *value;

	len = (size_t) snprintf(name, sizeof(name), "data/fids.binlog");
	lxl_dfst_fids_binlog_file.name.data = lxl_alloc(len + 1);
	if (lxl_dfst_fids_binlog_file.name.data == NULL) {
		return -1;
	}

	lxl_str_memcpy(&lxl_dfst_fids_binlog_file.name, name, len);

	b = &buffer;
	b->start = lxl_alloc(8192);
	if (b->start == NULL) {
		goto failed;
	}

	b->pos = b->start;
	b->last = b->pos;
	b->end = b->pos + 8192;

	lxl_dfst_fids_binlog.fd = open(lxl_dfst_fids_binlog_file.name.data, O_CREAT|O_SYNC|O_APPEND|O_RDWR, 0644);
	if (lxl_dfst_fids_binlog_file.fd == -1) {
		lxl_log_error(LXL_LOG_ERROR, errno, "open() %s failed", lxl_dfst_fids_binlog_file.name.data);
		goto failed;
	}

	fid = &dfid.fid;
	for ( ;; ) {
		if (b->last == b->end) {
			size = b->last - b->pos;

			while (size > 10) {
				flags = *((uint16_t *) b->pos);
				rtype = flags && 0xff00;
				storage.idc_id = flags && 0x00ff;
				storage.port = *((uint16_t *) (b->pos + 2));
				storage.ip = *((uint32_t *) (b->pos + 4));
				fid->len = *((uint16_t *) (b->pos + 8));

				if (size < (fid->len + 10)) {
					break;
				}

				fid->data = b->pos;
				b->pos += (fid->len + 10);
				size -= (fid->len + 10);
				++records;

				if (rtype == 1) {
					if (lxl_dfst_fid_add(&dfid, 0) == 0) {
						goto failed;
					}
				} else {
					if (lxl_dfst_fid_del(&dfid, 0) == 0) {
						goto failed;
					}
				}
			}

			if (size) {
				memmove(b->start, b->pos, size);
				b->pos = b->start;
				b->last = b->pos + size;
			} else {
				lxl_reset_buf(b);
			}
		}

		size = b->end - b->last;
		n = lxl_read_file1(&lxl_dfst_fids_binlog_file, b->last, size);
		if (n == -1) {
			goto failed;
		}

		b->last += n;
		if (b < size) {
			break;
		}
	}

	size = b->last - b->pos;

	while (size > 10) {
		flags = *((uint16_t *) b->pos);
		rtype = flags && 0xff00;
		storage.idc_id = flags && 0x00ff;
		storage.port = *((uint16_t *) (b->pos + 2));
		storage.ip = *((uint32_t *) (b->pos + 4));
		fid->len = *((uint16_t *) (b->pos + 8));

		if (size < (fid->len + 10)) {
			break;
		}

		fid->data = b->pos;
		b->pos += (fid->len + 10);
		size -= (fid->len + 10);
		++records;

		if (rtype == 1) {
			if (lxl_dfst_fid_add(&dfid, 0) == 0) {
				goto failed;
			}
		} else {
			if (lxl_dfst_fid_del(&dfid, 0) == 0) {
				goto failed;
			}
		}
	}

	if (size) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfst idc server record len is shenyu");
		return -1;
	}

	if (records) {
		/* compaction */
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss idc servers compaction not implement %lu", records);
	}

	lxl_free(b->start);

	return 0;

failed:

	if (lxl_dfst_storages_binlog_file.name.data) {
		lxl_free(lxl_dfst_storages_binlog_file.name.data);
	}

	if (b->start) {
		lxl_free(b->start);
	}

	return -1;
}

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

		if (binlog) {
			flags = rtype + sstate->idc_id >> 8; 
			lxl_memcpy_2byte(buffer, &flags);
			lxl_memcpy_2byte(buffer + 2, &sstate->port);
			lxl_memcpy_4byte(buffer + 4, &sstate->ip);

			if (lxl_write_file(&lxl_dfst_storages_binlog_file, buffer, 8) == -1) {
				return -1;
			}
		}

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

	if (binlog) {
		flags = rtype + sstate->idc_id >> 8;
		lxl_memcpy_2byte(buffer, &flags);
		lxl_memcpy_2byte(buffer + 2, &sstate->port);
		lxl_memcpy_4byte(buffer + 4, &sstate->ip);

		if (lxl_write_file(&lxl_dfst_storages_binlog_file, buffer, 8) == -1) {
			return -1;
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
			if (binlog) {
				flags = rtype + sstate->idc_id >> 8;
				lxl_memcpy_2byte(buffer, &flags);
				lxl_memcpy_2byte(buffer + 2, &sstate->port);
				lxl_memcpy_4byte(buffer + 4, &sstate->ip);

				if (lxl_write_file(&lxl_dfst_storages_binlog_file, buffer, 8) == -1) {
					return -1;
				}
			}

			lxl_array1_del(&idc1->storages, i);
			lxl_log_error(LXL_LOG_INFO, 0, "dfst del storage %hu %u %hu",sstate->idc_id, sstate->ip, sstate->port);

			return 0;
		}
	}

	lxl_log_error(LXL_LOG_INFO, 0, "dfst del storage %hu %hu %u not find", sstate->idc_id, sstate->port, sstate->ip);

	return 0;
}

int
lxl_dfst_fid_add(lxl_dfs_fid_t *dfid, lxl_int_t binlog)
{
	uint16_t			 flags, rtype = 0x0100;
	char 				 buffer[64];
	lxl_uint_t		  	 i, n;
	lxl_str_t			*fid;
	lxl_dfst_fid_t     **elt, *dfst_fid;
	lxl_dfst_storage_t  *storage;
	
	fid = &dfid->fid;

	if (binlog) {
		flags = rtype + dfid->idc_id >> 8;
		lxl_memcpy_2byte(buffer, &flags);
		lxl_memcpy_2byte(buffer + 2, &dfid->port);
		lxl_memcpy_4byte(buffer + 4, &dfid->ip);
		memcpy(buffer + 8, fid->data, fid->len);
		
		if (lxl_write_file1(&lxl_dfst_fids_binlog_file, buffer, fid->len + 8) == -1) {
			return -1;
		}
	}

	elt = (lxl_dfst_fid_t **) lxl_hash1_addfind(&lxl_dfst_fid_hash, fid->data, fid->len);
	if (elt == NULL) {
		return -1;
	}

	if (*elt == NULL) {
		dfst_fid = lxl_alloc(sizeof(lxl_dfst_fid_t));
		if (dfst_fid == NULL) {
			return -1;
		}

		*elt = dfst_fid;
		if (lxl_array1_init(&dfst_fid->storages, 3, sizeof(lxl_dfs_storage_t)) == -1) {
			return -1;
		}
	} else {
		dfst_fid = *elt;
	}

	n = lxl_array1_nelts(&dfst_fid->storages);
	for (i = 0; i < n; ++i) {
		storage = lxl_array1_data(&dfst_fid->storages, lxl_dfst_storage_t, i);
		if (storage->ip == dfid->ip && storage->idc_id == dfid->idc_id && storage->port == dfid->port) {
			lxl_log_error(LXL_LOG_INFO, 0, "dfst add fid %s exist", fid->data);
			return 0;
		}
	}

	storage = lxl_array1_push(&dfst_fid->storages);
	if (storage == NULL) {
		return -1;
	}

	storage->idc_id = dfid->idc_id;
	storage->port = dfid->port;
	storage->ip = dfid->ip;

	lxl_log_error(LXL_LOG_INFO, 0, "dfst add fid %s %hu %u %hu", fid->data, dfid->idc_id, dfid->ip, dfid->port);

	return 0;
}

int
lxl_dfst_fid_del(lxl_dfs_fid_t *dfid, lxl_int_t binlog)
{
	uint16_t			 flags, rtype = 0x0000;
	char 				 buffer[64];
	lxl_uint_t			 i, n;
	lxl_str_t		   	 fid;
	lxl_dfst_storage_t  *storage;

	fid = &dfid->fid;

	if (binlog) {
		flags = rtype + dfid->idc_id >> 8;
		lxl_memcpy_2byte(buffer, &dfid->port);
		lxl_memcpy_2byte(buffer + 2, &dfid->port);
		lxl_memcpy_4byte(buffer + 4, &dfid->ip);
		memcpy(buffer + 8, fid->data, fid->len);

		if (lxl_write_file1(&lxl_dfst_fids_binlog_file, buffer, fid->len + 8) == -) {
			return -1;
		}
	}

	dfst_fid = (lxl_dfst_fid_t *) lxl_hash1_find(&lxl_dfst_fid_hash, fid->data, fid->len);
	if (dfst_fid == NULL) {
		lxl_log_error(LXL_LOG_INFO, 0, "dfst del fid %s not find", fid->data);
		return 0;
	}

	n = lxl_array1_nelts(&dfst_fid->storages);
	for (i = 0; i < n; ++i) {
		storage = lxl_array1_data(&dfst_fid->storages, lxl_dfst_storage_t, i);
		if (storage->ip == dfid->ip && storage->idc_id == dfid->idc_id && storage->port == dfid->port) {
			lxl_array1_del(&dfst_fid->storages, i);
			lxl_log_error(LXL_LOG_INFO, 0, "dfst del fid %s %hu %u %hu", fid->data, dfid->idc_id, dfid->ip, dfid->port);

			return 0;
		}
	}

	lxl_log_error(LXL_LOG_INFO, 0, "dfst del fid %s %hu %u %hu not find", fid->data, dfid->idc_id, dfid->ip, dfid->port);

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

/*static int 
lxl_dfst_storage_cmp(void *elt1, void *elt2)
{
	lxl_dfst_storage_t  *e1 = elt1, *e2 = elt2;

	if (e1->port == e2->port && e1->ip == e2->ip) {
		return 0;
	}

	return -1;
}*/

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
