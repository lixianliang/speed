
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DFS_H_INCLUDE
#define LXL_DFS_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_string.h>


#define	LXL_DFS_UPLOAD					0x0001	
#define LXL_DFS_DOWNLOAD				0x0002	
#define LXL_DFS_DELETE					0x0004
#define LXL_DFS_UPLOAD_SC				0x0008
//#define LXL_DFS_UPLOADX				0x0008	/* sync backup */


//#define LXL_DFS_STAT					0x0010
#define LXL_DFS_STORAGE_OFFLINE			0x0020
#define LXL_DFS_STORAGE_ONLINE			0x0040

#define LXL_DFS_STORAGE_SYNC_PUSH		0x0100
#define LXL_DFS_STORAGE_SYNC_PULL		0x0200
//#define LXL_DFS_STORAGE_SYNC_FID		0x0400
//#define LXL_DFS_TRACKER_SYNC_FID		0x0800

#define LXL_DFS_STORAGE_REPORT_STATE	0x1000
#define LXL_DFS_STORAGE_REPORT_FID		0x2000
#define LXL_DFS_STORAGE_REPORT_SYNC_FID	0x4000


#define LXL_DFS_OK						200

#define LXL_DFS_BAD_REQUEST				400
//#define LXL_DFS_REQUEST_TOO_LONG		401
#define LXL_DFS_NOT_FIND				404
#define LXL_DFS_REQUEST_TIMEOUT			408
#define LXL_DFS_NOT_FIND_GROUP			410
#define LXL_DFS_NOT_FIND_IDC			411
#define LXL_DFS_NOT_FIND_STORAGE        412
#define LXL_DFS_NOT_FIND_FID			413
#define LXL_DFS_CLIENT_CLOSE_REQUEST    420

#define LXL_DFS_SERVER_ERROR  			500
#define LXL_DFS_NOT_IMPLEMENTED			501
#define LXL_DFS_BAD_GATEWAY				502		/* get from upstream invalid response */

/*#define LXL_DFS_SERVER_ERROR			1
#define LXL_DFS_NOT_IMPLEMENTED			2
#define LXL_DFS_CLIENT_CLOSE_REQUEST	6
#define LXL_DFS_GROUP_STORAGE_EXIST		7
#define LXL_DFS_BAD_TRACKER				8
#define LXL_DFS_BAD_STORAGE				9
#define LXL_DFS_NOT_FIND_GROUP			10
#define LXL_DFS_NOT_FIND_STORAGE		11
#define LXL_DFS_BAD_RESPONSE			12*/
//#define LXL_DFS_TRACKER_TIME_OUT		9
//#define LXL_DFS_STORAGE_TIME_OUT		10

#define LXL_DFS_REPORT_STATE_UDP_LEN	26
#define LXL_DFS_MIN_UDP_LENGTH			32


#pragma pack(2)
typedef struct {
	uint32_t	 body_n;		/* udp protocol body_n is id */
	uint16_t	 flen;		
	uint16_t	 qtype;
} lxl_dfs_request_header_t;
#pragma pack()

#pragma pack(2)
typedef struct {
	uint32_t 	 body_n;
	uint16_t	 flen;
	uint16_t 	 rcode;
} lxl_dfs_response_header_t;
#pragma pack()

typedef struct {
//	uint16_t	 area_id;
  	uint16_t	 idc_id;
} lxl_dfs_idc_t;

typedef struct {
//	uint16_t	 area_id;
  	uint16_t	 idc_id;
	uint16_t     port;
	uint32_t	 ip;
} lxl_dfs_storage_t;

typedef struct {
//	uint16_t	 area_id;
	uint16_t  	 idc_id;
	uint16_t 	 port;
	uint32_t	 ip;

	uint16_t	 loadavg;
	uint16_t	 cpu_idle;
	uint32_t	 disk_free_mb;
} lxl_dfs_storage_state_t;

typedef struct  {
//	uint16_t   	 area_id;
  	uint16_t     idc_id;
	uint16_t	 port;
	uint32_t 	 ip;

	lxl_str_t	 fid;
} lxl_dfs_fid_t;

typedef struct {
	//uint16_t     idc_id;
	uint16_t	 port;
	uint32_t	 ip;
} lxl_dfs_ip_port_t;

typedef struct {
	uint16_t     idc_id;
	uint16_t	 port;
	uint32_t	 ip;
} lxl_dfs_ip_port1_t;

/*typedef struct {
	lxl_list_t   list;
	lxl_str_t 	 fid;
} lxl_dfs_fid_t;*/


#endif  /* LXL_DFS_H_INCLUDE */
