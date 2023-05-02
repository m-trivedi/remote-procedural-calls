/* select client id randomly using rand() on startup

send the server a:
 - message
 - client id
 - sequence number (for ordering, duplicates)

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "client.h"
#include "udp.h"

int seq_num = 0;

struct rpc_connection RPC_init(int src_port, int dst_port, char dst_addr[]) {
    printf("Initializing RPC Connection...\n");

    struct rpc_connection rpc;

    rpc.recv_socket = init_socket(src_port);                      // initialize socket
    rpc.client_id = rand();                                       // set random client id

    // in order to populate the sockaddress the below code is picked from the image (now broken)
    // on the canvas project description page

    struct sockaddr_storage addr;
    socklen_t addrlen;
    populate_sockaddr(AF_INET, dst_port, dst_addr, &addr, &addrlen);
    rpc.dst_addr = *((struct sockaddr *)(&addr));
    rpc.dst_len = addrlen;

    // rpc.dst_len = strlen(dst_addr);                               // set dst_len
    rpc.seq_number = seq_num;                                     // set seq_num
    seq_num++;
    
    return rpc;
}

// Sleeps the server thread for a few seconds
void RPC_idle(struct rpc_connection *rpc, int time) {

}

// gets the value of a key on the server store
int RPC_get(struct rpc_connection *rpc, int key) {
    return 0;
}

// sets the value of a key on the server store
int RPC_put(struct rpc_connection *rpc, int key, int value) {

    char payload[100];
    sprintf(payload, "%d.%d", key, value);

    int payload_length = strlen(payload);

    send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, payload, payload_length);

    return 0;
}

// closes the RPC connection to the server
void RPC_close(struct rpc_connection *rpc) {
    
}