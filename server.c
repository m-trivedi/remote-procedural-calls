// to prevent duplicates,

// if the seq_num received is less than the current tracked seq num, it will discard the request

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "udp.h"
#include "server_functions.h"

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
        // printf("Received: %s\n", my_packet.buf);
        if (my_packet.buf[0] == '0') {
            // call idle function
            printf("Idle was called.\n");
        } else if (my_packet.buf[0] == '1') {
            // call get function

            int key = atoi(my_packet.buf + 1);
            printf("Request to get %d\n", key);
            int value = get(key);
            
            // send value to client
            char payload[100];
            sprintf(payload, "%d", value);
            int payload_length = sizeof(payload);
            send_packet(my_socket, my_packet.sock, my_packet.slen, payload, payload_length);

        } else if (my_packet.buf[0] == '2') {
            // call put function

            int i = 0;
            while (i < strlen(my_packet.buf)) {
                if (my_packet.buf[i] == ' ') {
                    break;
                }
                i++;
            }

            char key_s[5];
            strncpy(key_s, my_packet.buf + 1, i - 1);
            int key = atoi(key_s);
            int value = atoi(my_packet.buf + i + 1);

            printf("Request to put %d at %d\n", value, key);

            if (put(key, value) != 0) {
                printf("Error with putting key in array\n");
            }
        }
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