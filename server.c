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
int call_table_size = 0;

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

        // Convert data received in packet to data structure

        int i = 0;
        int arr[5];

        // arr[0] -> function to execute: idle (0), get(1), put(2)
        // arr[1] -> client id
        // arr[2] -> seq num
        // arr[3] -> key / time
        // arr[4] -> value

        char * token = strtok(my_packet.buf, " ");
        while( token != NULL ) {
            arr[i] = atoi(token);
            token = strtok(NULL, " ");
            i++;
        }

        // Before executing any request, first compare this client's last seq num and this seq num
        // i > last: new request - execute RPC and update call table entry
        // i = last: duplicate of last RPC or duplicate of in progress RPC. Either resend result or send acknowledgement that RPC is being worked on.
        // i < last: old RPC, discard message and do not reply

        int flag = 1;
        for (int i = call_table_size - 1; i >= 0; i--) {
            if (call_table[i].client_id ==arr[1]) {
                if (call_table[i].seq_num > arr[2]) {
                    // old RPC, discard message and do not reply
                    flag = 0;
                } else if (call_table[i].seq_num == arr[2]) {
                    if (call_table[i].result != -1) { // doubtful: what if result of a prev get was value -1 ?
                        // resend result to client
                        char payload[100];
                        sprintf(payload, "%d", call_table[i].result);
                        send_packet(my_socket, my_packet.sock, my_packet.slen, payload, sizeof(payload));
                    } else {
                        // TODO: send acknowledgement
                        
                    }
                }
                break;
            }
        }

        printf("Client ID: %d, Seq Num: %d ", arr[1], arr[2]);

        if (flag == 0) {
            // old RPC discard message
            continue;
        }

        // ----------

        if (arr[0] == 0) { // idle

            int time = arr[3];
            printf("Request to idle for %d seconds\n", time);
            idle(time);

        } else if (arr[0] == 1) { // get

            int key = arr[3];
            printf("Request to get %d\n", key);
            int value = get(key);
            
            // send value to client
            char payload[100];
            sprintf(payload, "%d", value);
            send_packet(my_socket, my_packet.sock, my_packet.slen, payload, sizeof(payload));

        } else if (arr[0] == 2) { // put

            int key = arr[3];
            int value = arr[4];

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