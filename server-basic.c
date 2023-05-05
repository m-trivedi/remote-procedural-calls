#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "udp.h"
#include "server_functions.h"

struct call_table_info {
    int client_id;
    int seq_num;
    int key;
    int val;
    int status;
};

struct call_table_info call_table_array[200];
int call_table_array_size = 0;

int main (int argc, char** argv) {
    if (argc != 2) {
        printf("Invalid number of arguments.\n");
        exit(0);
    }

    int port = atoi(argv[1]);

    printf("port: %d\n", port);

    // start server
    printf("Starting server...\n");
    struct socket my_socket = init_socket(port);

    while (1) {
        struct packet_info my_packet = receive_packet(my_socket);

        int i = 0;
        int arr[5];

        char * token = strtok(my_packet.buf, " ");
        while( token != NULL ) {
            arr[i] = atoi(token);
            token = strtok(NULL, " ");
            i++;
        }

        int func = arr[0];
        int client_id = arr[1];
        int seq_num = arr[2];
        int key = arr[3];
        int val = arr[4];

        printf("Func: %d, Client ID: %d, Seq Num: %d, Key: %d, Val: %d\n", func, client_id, seq_num, key, val);

        int flag = 1;
        for (int j = call_table_array_size - 1; j >= 0; j--) {
            if (call_table_array[j].client_id == client_id) {
                if (call_table_array[j].seq_num > seq_num) {
                    flag = 0; // don't service request.
                } else if (call_table_array[j].seq_num == seq_num) {
                    flag = 0; // don't service request.
                    if (call_table_array[j].status != 0) {
                        // resend result to client
                        printf("Resending Result: %d\n", call_table_array[j].val);
                        char payload[100];
                        sprintf(payload, "%d", call_table_array[j].val);
                        send_packet(my_socket, my_packet.sock, my_packet.slen, payload, sizeof(payload));
                    } else {
                        // send acknowledgement
                        printf("Sending ACK\n");
                        char payload[100] = "ACK";
                        send_packet(my_socket, my_packet.sock, my_packet.slen, payload, sizeof(payload));
                    }
                }
                break;
            }
        }

        if (flag == 0) {
            continue;
        }

        call_table_array[call_table_array_size].client_id = client_id;
        call_table_array[call_table_array_size].seq_num = seq_num;
        call_table_array[call_table_array_size].key = key;
        call_table_array[call_table_array_size].val = val;
        call_table_array[call_table_array_size].status = 0;

        // check the fifo here

        if (func == 0) {
            // call idle
        } else if (func == 1) {
            // call get
            int res = get(key);

            printf("sending res: %d", res);

            char payload[100];
            sprintf(payload, "%d", res);
            send_packet(my_socket, my_packet.sock, my_packet.slen, payload, sizeof(payload));

            call_table_array[call_table_array_size].val = res;
            call_table_array[call_table_array_size].status = 1;

        } else if (func == 2) {
            // call put
            put(key, val);

            call_table_array[call_table_array_size].val = val;
            call_table_array[call_table_array_size].status = 1;

        }
        call_table_array_size++;
    }
    return 0;
}