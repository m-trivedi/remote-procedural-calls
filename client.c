/* select client id randomly using rand() on startup

send the server a:
 - message
 - client id
 - sequence number (for ordering, duplicates)

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "client.h"
#include "udp.h"

struct rpc_connection RPC_init(int src_port, int dst_port, char dst_addr[]) {
    printf("Initializing RPC Connection...\n");

    struct rpc_connection rpc;

    rpc.recv_socket = init_socket(src_port);                      // initialize socket
    rpc.client_id = rand();                                       // set random client id

    // in order to populate the sockaddress

    struct sockaddr_storage addr;
    socklen_t addrlen;
    populate_sockaddr(AF_INET, dst_port, dst_addr, &addr, &addrlen);
    rpc.dst_addr = *((struct sockaddr *)(&addr));
    rpc.dst_len = addrlen;
    rpc.seq_number = 0;
    
    return rpc;
}

// Sleeps the server thread for a few seconds
void RPC_idle(struct rpc_connection *rpc, int time) {
    char payload[100];
    sprintf(payload, "0 %d %d %d", rpc->client_id, rpc->seq_number, time);
    send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, payload, sizeof(payload));
    rpc->seq_number++;
}

// gets the value of a key on the server store
int RPC_get(struct rpc_connection *rpc, int key) {
    char payload[100];
    sprintf(payload, "1 %d %d %d", rpc->client_id, rpc->seq_number, key);
    send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, payload, sizeof(payload));

    // start listening for message...
    int value = 0;

    // this loops infinitely, in future change this to check if timer reaches 0
    int retries = 0;
    while (retries < RETRY_COUNT) {
        struct packet_info my_packet = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        if (my_packet.recv_len < 0) {
            // timed out
        } else if (strcmp("ACK", my_packet.buf) == 0) {
            // received acknowledgement
            sleep(1);
            printf("sleeping");
        } else {
            value = atoi(my_packet.buf);
            break;
        }
        retries++;
    }

    rpc->seq_number++;
    return value;
}

// sets the value of a key on the server store
int RPC_put(struct rpc_connection *rpc, int key, int value) {
    char payload[100];
    sprintf(payload, "2 %d %d %d %d", rpc->client_id, rpc->seq_number, key, value);
    send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, payload, sizeof(payload));
    rpc->seq_number++;
    return 0;
}

// closes the RPC connection to the server
void RPC_close(struct rpc_connection *rpc) {
    close_socket(rpc->recv_socket);
}