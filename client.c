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
#include <sys/time.h>
#include <time.h>
#include "client.h"
#include "udp.h"

struct rpc_connection RPC_init(int src_port, int dst_port, char dst_addr[]) {

    struct rpc_connection rpc;
    rpc.recv_socket = init_socket(src_port);
    
    // SET CLIENT ID USING TIME FUNCTION (BY PIAZZA)
    //struct timeval current_time;
    srand(time(NULL));
    rpc.client_id = rand();

    // POPULATE SOCKADDR
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
    sprintf(payload, "0 %d %d %d %d", rpc->client_id, rpc->seq_number, time, 0);
    send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, payload, sizeof(payload));

    // implementation of a block till we get a response from server
    while(1) {
        struct packet_info my_packet = receive_packet(rpc->recv_socket);
        if (strcmp("FIN", my_packet.buf) == 0) {
            // the server finished
            break;
        }
    }

    rpc->seq_number++;
}

// gets the value of a key on the server store
int RPC_get(struct rpc_connection *rpc, int key) {
    char payload[100];
    sprintf(payload, "1 %d %d %d %d", rpc->client_id, rpc->seq_number, key, 0);
    send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, payload, sizeof(payload));

    // start listening for message...
    int value = -10;

    // this loops infinitely, in future change this to check if timer reaches 0
    int retries = 0;
    while (retries < RETRY_COUNT) {
        struct packet_info my_packet = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        if (my_packet.recv_len < 0) {
            // timed out
        } else if (strcmp("ACK", my_packet.buf) == 0) {
            // received acknowledgement
            sleep(1);
        } else {
            value = atoi(my_packet.buf);
            break;
        }
        retries++;
    }

    if (retries == RETRY_COUNT) {
        printf("Error: No response from server in a while, exiting...\n");
        exit(1);
    }

    rpc->seq_number++;
    return value;
}

// sets the value of a key on the server store
int RPC_put(struct rpc_connection *rpc, int key, int value) {
    char payload[100];
    sprintf(payload, "2 %d %d %d %d", rpc->client_id, rpc->seq_number, key, value);
    send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, payload, sizeof(payload));

    // start listening for message...
    int mvalue = -10;

    // this loops infinitely, in future change this to check if timer reaches 0
    int retries = 0;
    while (retries < RETRY_COUNT) {
        struct packet_info my_packet = receive_packet_timeout(rpc->recv_socket, TIMEOUT_TIME);
        if (my_packet.recv_len < 0) {
            // timed out
        } else if (strcmp("ACK", my_packet.buf) == 0) {
            // received acknowledgement
            sleep(1);
        } else {
            mvalue = atoi(my_packet.buf);
            break;
        }
        retries++;
    }

    if (retries == RETRY_COUNT) {
        printf("Error: No response from server in a while, exiting...\n");
        exit(1);
    }

    if (mvalue != -1) {
        mvalue = 0;
    }

    rpc->seq_number++;
    return mvalue;
}

// closes the RPC connection to the server
void RPC_close(struct rpc_connection *rpc) {
    close_socket(rpc->recv_socket);
}