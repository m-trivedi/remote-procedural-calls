// to prevent duplicates,

// if the seq_num received is less than the current tracked seq num, it will discard the request

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "udp.h"

struct call_info {
    int client_id;
    int seq_num;
    int result;
};

struct call_info call_table[200];

int main(int argc, char** argv) {

    if (argc != 2) {
        printf("Invalid number of arguments.\n");
        return 0;
    }

    int port = atoi(argv[1]);

    // start server

    printf("Starting server...\n");
    struct socket my_socket = init_socket(port);

    // listen for client requests
    while (1) {
        struct packet_info my_packet = receive_packet(my_socket);
        printf("Received: %s\n", my_packet.buf);
    }


    /*

    Messages arrive with sequence number i

    get last seq_num by iterating the call_table from greatest to lowest index and stop when client_id matches

    i > last: new request - execute RPC and update call table entry
    i = last: duplicate of last RPC or duplicate of in progress RPC. Either resend result or send acknowledgement that RPC is being worked on.
    i < last: old RPC, discard message and do not reply

    */




    return 0;
}