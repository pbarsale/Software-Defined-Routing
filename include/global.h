#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <time.h>

typedef enum {FALSE, TRUE} bool;

#define ERROR(err_msg) {perror(err_msg); exit(EXIT_FAILURE);}

/* https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/ */
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)])) // Interesting stuff to read if you are interested to know how this works

#define ROUTING_PACKET_F 8
#define ROUTING_PACKET_V 12  
#define ROUTING_TO_CONTROLLER_PACKET 8  



uint16_t CONTROL_PORT;
uint16_t No_of_routers;
uint16_t periodic_interval;
struct timeval temp_tv;


int router_index;
//struct sockaddr_in router_address;
struct sockaddr_in peer_address;

struct neighbour_routers 
{
	uint16_t router_id;
	uint16_t router_port;
	uint16_t data_port;
	uint16_t link_cost;
	uint16_t init_link_cost;
	uint32_t router_ip_add;
	uint32_t next_hop;
	uint32_t timeout;
	int alive;
}peers[5];

struct distance_vector_from_peer
{
	uint32_t peer_ip;
	uint16_t peer_port;
	uint16_t peer_cost;
	uint16_t peer_id;
	int valid;
};
	

#endif
