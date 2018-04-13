/**
 * @author
 * @author  Swetank Kumar Saha <swetankk@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * AUTHOR [Control Code: 0x00]
 */

#include <string.h>
#include<stdlib.h> 
#include<stdio.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <sys/types.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/connection_manager.h"


#define INTERVAL_OFFSET 2
#define ROUTER_ID_OFFSET 4
#define ROUTER_PORT_OFFSET 6
#define DATA_PORT_OFFSET 8
#define LINK_COST_OFFSET 10
#define IP_ADDRESS_OFFSET 12
#define ROUTER_INFO 12


//Offset values for routing update packet to other routers
#define SOURCE_ROUTER_PORT 2
#define SOURCE_ROUTER_IP 4
#define DEST_ROUTER_IP 8
#define DEST_ROUTER_PORT 12
#define DEST_ROUTER_ID 16
#define DEST_COST 18
#define DEST_INFO 12 


//offset values for Seding Routing packet to controller
#define NEXT_HOP_ID 4
#define LINK_COST 6
#define ROUTER_INFO_SIZE 8
uint16_t get_link_cost(uint16_t id);

void init_action(int sock_index, char *cntrl_payload)
{

	memcpy(&No_of_routers, cntrl_payload, sizeof(No_of_routers));
	memcpy(&periodic_interval, cntrl_payload+INTERVAL_OFFSET, sizeof(periodic_interval));
	
	No_of_routers = ntohs(No_of_routers);	
	periodic_interval = ntohs(periodic_interval);
	temp_tv.tv_sec = periodic_interval;
	temp_tv.tv_usec = 0;	

	memset(&peers,0,sizeof peers);

	for(int i=0;i<No_of_routers;i++)
	{
		memcpy(&peers[i].router_id, cntrl_payload+ROUTER_ID_OFFSET+(i*ROUTER_INFO), sizeof(peers[i].router_id));
		memcpy(&peers[i].router_port, cntrl_payload+ROUTER_PORT_OFFSET+(i*ROUTER_INFO), sizeof(peers[i].router_port));
		memcpy(&peers[i].data_port, cntrl_payload+DATA_PORT_OFFSET+(i*ROUTER_INFO), sizeof(peers[i].data_port));
		memcpy(&peers[i].init_link_cost, cntrl_payload+LINK_COST_OFFSET+(i*ROUTER_INFO), sizeof(peers[i].init_link_cost));
		memcpy(&peers[i].link_cost, cntrl_payload+LINK_COST_OFFSET+(i*ROUTER_INFO), sizeof(peers[i].link_cost));
		memcpy(&peers[i].router_ip_add, cntrl_payload+IP_ADDRESS_OFFSET+(i*ROUTER_INFO), sizeof(peers[i].router_ip_add));
		peers[i].router_id=ntohs(peers[i].router_id);
		peers[i].router_port=ntohs(peers[i].router_port);
		peers[i].data_port=ntohs(peers[i].data_port);
		peers[i].init_link_cost=ntohs(peers[i].init_link_cost);
		peers[i].link_cost=ntohs(peers[i].link_cost);
		peers[i].router_ip_add=ntohl(peers[i].router_ip_add);
		peers[i].alive=1;
		peers[i].timeout=-1;
		
		if(peers[i].init_link_cost==0)
		{
			router_index=i;
		}

		if(peers[i].init_link_cost==65535)
		{
			peers[i].next_hop=65535;
		}
		else
		{
			peers[i].next_hop=peers[i].router_id;
		}
		
//		printf("Router id %d : \r\n",peers[r_id].router_id);
//		printf("Router port %d : \r\n",peers[r_id].router_port);
//		printf("Data Port %d : \r\n",peers[r_id].data_port);
//		printf("link cost %d : \r\n",peers[r_id].link_cost);
//		printf("init link cost %d : \r\n",peers[r_id].init_link_cost);
//		printf("Next Hop %d : \r\n",peers[r_id].next_hop);
	}
	
	router_socket=socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in raddr;
	memset(&raddr,0,sizeof raddr);
	
	raddr.sin_family = AF_INET;
    	raddr.sin_port = htons(peers[router_index].router_port);
    	raddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	bind(router_socket,(struct sockaddr*) &raddr, sizeof(raddr));
	
	FD_SET(router_socket,&master_list);
        if(router_socket > head_fd)
                head_fd = router_socket;
 
	return;
}

void init_response(int sock_index)
{
	uint16_t payload_len=0, response_len;
        char *cntrl_response;

        cntrl_response = create_response_header(sock_index, 1, 0, payload_len);
        response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
        sendALL(sock_index, cntrl_response, response_len);
        free(cntrl_response);
	return;
}

void send_routing_updates()
{
	uint16_t temp_no_routers, temp_source_port,temp_dest_port, temp_dest_id, temp_dest_cost;
	uint32_t temp_source_ip, temp_dest_ip;

	struct sockaddr_in paddr;	
	int size=ROUTING_PACKET_F+(ROUTING_PACKET_V * No_of_routers);
	
	char *routing_update_response = (char *) malloc(size);
	memset(routing_update_response,0, size);
	
	temp_no_routers=htons(No_of_routers);
	temp_source_port=htons(peers[router_index].router_port);
	temp_source_ip=htonl(peers[router_index].router_ip_add);

	memcpy(routing_update_response,&temp_no_routers,sizeof temp_no_routers);
	memcpy(routing_update_response+SOURCE_ROUTER_PORT,&temp_source_port ,sizeof temp_source_port);
	memcpy(routing_update_response+SOURCE_ROUTER_IP,&temp_source_ip,sizeof temp_source_ip);
	
	for(int i =0;i<No_of_routers;i++)
	{
		temp_dest_ip=htonl(peers[i].router_ip_add);
		temp_dest_port=htons(peers[i].router_port);
		temp_dest_id=htons(peers[i].router_id);
		temp_dest_cost=htons(peers[i].link_cost);

		memcpy(routing_update_response+DEST_ROUTER_IP+(i*DEST_INFO),&temp_dest_ip , sizeof temp_dest_ip);
		memcpy(routing_update_response+DEST_ROUTER_PORT+(i*DEST_INFO),&temp_dest_port, sizeof temp_dest_port);
		memcpy(routing_update_response+DEST_ROUTER_ID+(i*DEST_INFO),&temp_dest_id, sizeof temp_dest_id);
		memcpy(routing_update_response+DEST_COST+(i*DEST_INFO),&temp_dest_cost, sizeof temp_dest_cost);
	}
 
	for(int i=0;i<No_of_routers;i++)
	{
		if(peers[i].init_link_cost!=0 && peers[i].init_link_cost!=65535 && peers[i].alive==1)
		{
			uint32_t router_ip=htonl(peers[i].router_ip_add);
			memset(&paddr, 0 ,sizeof(paddr));

			paddr.sin_family = AF_INET;
		        paddr.sin_port = htons(peers[i].router_port);
			memcpy(&paddr.sin_addr.s_addr,&router_ip,4);

			int snt_bytes = sendto(router_socket, routing_update_response, size ,0,&paddr,sizeof(paddr));
		}
	}

	if(routing_update_response)
       		free(routing_update_response);
	return;
}

void send_routing_to_controller(int sock_index)
{
	uint16_t payload_len, response_len;
        char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;
	
	payload_len=No_of_routers*ROUTING_TO_CONTROLLER_PACKET;
	cntrl_response_payload = (char *) malloc(payload_len);
	memset(cntrl_response_payload,0,payload_len);

	cntrl_response_header = create_response_header(sock_index, 2, 0,payload_len);

	//making payload
	for(int i=0;i<No_of_routers;i++)
	{
		uint16_t temp_id,temp_next_hop,temp_cost;
		temp_id=htons(peers[i].router_id);
		temp_next_hop=htons(peers[i].next_hop);
		temp_cost=htons(peers[i].link_cost);

		memcpy(cntrl_response_payload+(i*ROUTER_INFO_SIZE),&temp_id,sizeof temp_id);
		memcpy(cntrl_response_payload+NEXT_HOP_ID+(i*ROUTER_INFO_SIZE),&temp_next_hop,sizeof temp_next_hop);
		memcpy(cntrl_response_payload+LINK_COST+(i*ROUTER_INFO_SIZE),&temp_cost,sizeof temp_cost);
	}

        response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
        cntrl_response = (char *) malloc(response_len);
	memset(cntrl_response,0,response_len);

        /* Copy Header */
        memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
        free(cntrl_response_header);

        /* Copy Payload */
        memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
        free(cntrl_response_payload);

        sendALL(sock_index, cntrl_response, response_len);
	
	if(cntrl_response)
        	free(cntrl_response);	
	return;

}

void crash_response(int sock_index)
{

	uint16_t payload_len=0, response_len;
        char *cntrl_response;

        cntrl_response = create_response_header(sock_index, 4, 0, payload_len);
        response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
        sendALL(sock_index, cntrl_response, response_len);
        free(cntrl_response);
        return;	
}


void run_distance_vector(char *routing_packet)
{	
	
	uint16_t no_update_fields,source_port, source_id;
	uint32_t source_ip,peer_ip;
	uint16_t peer_port, peer_id, peer_cost;

	struct distance_vector_from_peer peer_info[5];
	memset(&peer_info,0,sizeof peer_info);
	
	memcpy(&no_update_fields,routing_packet,sizeof no_update_fields);
        memcpy(&source_port,routing_packet+SOURCE_ROUTER_PORT,sizeof source_port );
        memcpy(&source_ip,routing_packet+SOURCE_ROUTER_IP,sizeof source_ip);
	
	no_update_fields=ntohs(no_update_fields);
	source_port=ntohs(source_port);
	source_ip=ntohl(source_ip);


	for(int i=0;i<no_update_fields;i++)
	{
		memcpy(&peer_id, routing_packet+DEST_ROUTER_ID+(i*DEST_INFO), sizeof peer_id);
		memcpy(&peer_ip, routing_packet+DEST_ROUTER_IP+(i*DEST_INFO), sizeof peer_ip);
		memcpy(&peer_port, routing_packet+DEST_ROUTER_PORT+(i*DEST_INFO),sizeof peer_port);
		memcpy(&peer_cost, routing_packet+DEST_COST+(i*DEST_INFO), sizeof peer_cost);

		peer_id=(int)ntohs(peer_id);
		peer_ip=ntohl(peer_ip);
		peer_cost=ntohs(peer_cost);
		peer_port=ntohs(peer_port);
		
		peer_info[i].peer_id=peer_id;
		peer_info[i].peer_ip=peer_ip;
		peer_info[i].peer_cost=peer_cost;
		peer_info[i].peer_port=peer_port;
	}
	
	
	 for(int i=0;i<No_of_routers;i++)
        {
               	if(peers[i].router_port==source_port)
               	{
	        	 source_id=peers[i].router_id;
			 break;
		}
	}

	
	for(int i=0;i<No_of_routers;i++)
	{
		uint16_t new_cost, next_hop;

		if(i==router_index)
			continue;
		
		new_cost = get_link_cost(source_id);

		if(new_cost==65535)
			continue;

		for(int j=0;j<no_update_fields;j++)
        	{
			if(peers[i].router_id==peer_info[j].peer_id)
			{
				if(peer_info[j].peer_cost==65535)
					new_cost=65535;
				else
					new_cost = new_cost + peer_info[j].peer_cost;
				break;
			}
		}

		if(new_cost < peers[i].link_cost)
		{
			peers[i].link_cost=new_cost;	
			peers[i].next_hop=source_id;
		}			
	}

	return;
}

  uint16_t get_link_cost(uint16_t id)
{
	uint16_t ans=65535;
	for(int i=0;i<No_of_routers;i++)
        {
               if(peers[i].router_id==id)
		{
			ans= peers[i].init_link_cost;
			break;
		}
        }
	return ans;
}


 void update_link_cost(int sock_index,char*  cntrl_payload)
{
	uint16_t router_id, cost;
	uint16_t payload_len=0, response_len;
        char *cntrl_response;

	memcpy(&router_id, cntrl_payload, sizeof(router_id));
        memcpy(&cost, cntrl_payload+2, sizeof(cost));

	router_id = ntohs(router_id);
	cost = ntohs(cost);
	
	 for(int i=0;i<No_of_routers;i++)
        {
		if(peers[i].router_id==router_id)
		{
			peers[i].link_cost=cost;
			peers[i].init_link_cost=cost;
			break;
		}
	}

        cntrl_response = create_response_header(sock_index, 3, 0, payload_len);
        response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
        sendALL(sock_index, cntrl_response, response_len);
        free(cntrl_response);
	return;		
}


