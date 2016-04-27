
/*
 * Copyrgiht (C) xianliang.li
 */


#ifndef LXL_EVENT_CONNECT_INCLUDE
#define LXL_EVENT_CONNECT_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_event.h>


#define LXL_PEER_NEXT	2
#define LXL_PEER_FAILED 4


typedef struct lxl_peer_connection_s    lxl_peer_connection_t;

typedef int	(*lxl_event_get_peer_pt) (lxl_peer_connection_t *pc, void *data);
typedef void (*lxl_event_free_peer_pt) (lxl_peer_connection_t *pc, void *data, lxl_uint_t state);

struct lxl_peer_connection_s {
	lxl_connection_t 	   *connection;
		
	struct sockaddr	 	   *sockaddr;
	socklen_t 		  		socklen;
	char 				   *name;
	/* lxl_addr_t replace ? */

	lxl_uint_t 		  		tries;

	lxl_event_get_peer_pt	get;
	lxl_event_free_peer_pt	free;
	void 				   *data;

//	lxl_addr_t 		 	   *local;
	int 			  		rcvbuf;
};


int	lxl_event_get_peer(lxl_peer_connection_t *pc, void *data);
int lxl_event_connect_peer(lxl_peer_connection_t *pc);
int	lxl_event_udp_connect_peer(lxl_peer_connection_t *pc);


#endif	/* LXL_EVENT_CONNECT_INCLUDE */
