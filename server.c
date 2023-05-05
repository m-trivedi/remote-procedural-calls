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
    int status;
};

struct call_info call_table[200];
int call_table_size = 0;

struct thread_data {
    int key;
    int value;
    struct packet_info my_pac;
    struct socket my_soc;
};

// Initialize the lock
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *my_idle(void *arg) {
    struct thread_data *data = (struct thread_data *) arg;
    int time = data->key;
    idle(time);

    // tell the client i've finished and ready for a new request.
    char payload[100] = "FIN";
    send_packet(data->my_soc, data->my_pac.sock, data->my_pac.slen, payload, sizeof(payload));

    return NULL;
}

// my_get for threads
void *my_get(void *arg) {
    struct thread_data *data = (struct thread_data *) arg;
    int key = data->key;
    pthread_mutex_lock(&lock);
    if (key >= NUMKEYS) {
        data->value = -1;
    } else {
        data->value = get(key);
    }
    // send value to client
    char payload[100];
    sprintf(payload, "%d", data->value);
    send_packet(data->my_soc, data->my_pac.sock, data->my_pac.slen, payload, sizeof(payload));
    pthread_mutex_unlock(&lock);
    return NULL;
}

// my_put for threads
void *my_put(void *arg) {
    struct thread_data *data = (struct thread_data *) arg;
    int key = data->key;
    int value = data->value;
    pthread_mutex_lock(&lock);
    if (key >= NUMKEYS) {
        data->value = -1;
    } else {
        put(key, value);
    }
    char payload[100];
    sprintf(payload, "%d", data->value);
    send_packet(data->my_soc, data->my_pac.sock, data->my_pac.slen, payload, sizeof(payload));

    // tell the client i've finished and ready for a new request.
    //char payload[100] = "FIN";
    //send_packet(data->my_soc, data->my_pac.sock, data->my_pac.slen, payload, sizeof(payload));
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

    // listen for client requests
    while (1) {
        struct packet_info my_packet = receive_packet(my_socket);
  
        // Convert data received in packet to data structure
        int i = 0;
        int arr[5];

        char * token = strtok(my_packet.buf, " ");
        while( token != NULL ) {
            arr[i] = atoi(token);
            token = strtok(NULL, " ");
            i++;
        }
    
        int flag = 1;
        for (int i = call_table_size - 1; i >= 0; i--) {
            if (call_table[i].client_id == arr[1]) {
                if (call_table[i].seq_num > arr[2]) {
                    // old RPC, discard message and do not reply
                    flag = 0;
                } else if (call_table[i].seq_num == arr[2]) {
                    if (call_table[i].status != 0) { // doubtful: what if result of a prev get was value -1 ?
                        // resend result to client
                        flag = 0;
                        if (arr[0] != 0) { // get, put
                            // resend result
                            char payload[100];
                            sprintf(payload, "%d", call_table[i].result);
                            send_packet(my_socket, my_packet.sock, my_packet.slen, payload, sizeof(payload));
                        } else { // idle
                            char payload[100] = "FIN";
                            send_packet(my_socket, my_packet.sock, my_packet.slen, payload, sizeof(payload));
                        }
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
            // old RPC discard message
            continue;
        }

        call_table[call_table_size].client_id = arr[1];
        call_table[call_table_size].seq_num = arr[2];
        call_table[call_table_size].result = arr[4];
        call_table[call_table_size].status = 0; // 0 means we're working on the request

        // initialize thread
        pthread_t my_thread;
        struct thread_data arg;
        arg.key = arr[3];
        arg.value = arr[4];
        arg.my_soc = my_socket;
        arg.my_pac = my_packet;
        

        if (arr[0] == 0) { 
            // idle
            pthread_create(&my_thread, NULL, my_idle, &arg);
        } else if (arr[0] == 1) { 
            // get
            pthread_create(&my_thread, NULL, my_get, &arg);
        } else if (arr[0] == 2) { 
            // put
            pthread_create(&my_thread, NULL, my_put, &arg);
        }

        pthread_join(my_thread, NULL);

        // update call table entry result
        call_table[call_table_size].result = arg.value;
        call_table[call_table_size].status = 1;
        call_table_size++;

    }
    return 0;
}