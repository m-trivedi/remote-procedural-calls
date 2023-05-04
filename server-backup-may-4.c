#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "udp.h"
#include "server_functions.h"

struct call_info {
    int client_id;
    int seq_num;
    int result;
};

struct call_info call_table[200];
int call_table_size = 0;

struct thread_data {
    int iter;
    int key;
    int value;
};


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *my_get(void *arg) {
    struct thread_data *data = (struct thread_data *) arg;
    int iter = data->iter;
    int key = data->key;
    pthread_mutex_lock(&lock);
    int result = get(key);
    call_table[iter].result = result; // update call table entry result
    call_table_size++;
    data->value = result;             // update arg struct
    pthread_mutex_unlock(&lock);
    return NULL;
}


int main(int argc, char** argv) {

    if (argc != 2) {
        printf("Invalid number of arguments.\n");
        return 0;
    }

    int port = atoi(argv[1]);

    // start server
    printf("Starting server...\n");
    struct socket my_socket = init_socket(port);

    //pthread_t my_thread[100]; // 100 clients can be connected to this server
    //int my_thread_iter = 0;

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
            if (call_table[i].client_id == arr[1]) {
                if (call_table[i].seq_num > arr[2]) {
                    // old RPC, discard message and do not reply
                    flag = 0;
                    
                } else if (call_table[i].seq_num == arr[2]) {
                    if (call_table[i].result != 0) { // doubtful: what if result of a prev get was value 0 ?
                        // resend result to client
                        flag = 0;
                        char payload[100];
                        sprintf(payload, "%d", call_table[i].result);
                        send_packet(my_socket, my_packet.sock, my_packet.slen, payload, sizeof(payload));
                    } else {
                        // send acknowledgement
                        char payload[100] = "ACK";
                        send_packet(my_socket, my_packet.sock, my_packet.slen, payload, sizeof(payload));
                    }
                }
                break;
            }
        }

        if (flag == 0) {
            // old RPC: discard message
            continue;
        } else {
            call_table[call_table_size].client_id = arr[1];
            call_table[call_table_size].seq_num = arr[2];
        }

        printf("Client ID: %d, Seq Num: %d ", arr[1], arr[2]);


        // EXECUTE THE FUNCTIION

        if (arr[0] == 0) { // idle

            printf("Request to idle for %d seconds\n", arr[3]);
            idle(arr[3]);

        } else if (arr[0] == 1) { // get

            /*
            struct thread_data arg;
            arg.iter = call_table_size;
            arg.key = arr[3];

            pthread_create(&my_thread[my_thread_iter], NULL, my_get, &arg);
            my_thread_iter++;
            */

            printf("Request to get %d\n", arr[3]);
            int result = get(arr[3]);
            
            // send value to client
            char payload[100];
            sprintf(payload, "%d", result);
            send_packet(my_socket, my_packet.sock, my_packet.slen, payload, sizeof(payload));

            // update call table entry result
            call_table[call_table_size].result = result;
            call_table_size++;
            
        } else if (arr[0] == 2) { // put

            printf("Request to put %d at %d\n", arr[4], arr[3]);
            put(arr[3], arr[4]);

            // update call table entry result
            call_table[call_table_size].result = arr[4];
            call_table_size++;
        }
    }
    return 0;
}