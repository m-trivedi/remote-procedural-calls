#include <stdio.h>

#include "client.h"

int main(){
    struct rpc_connection rpc = RPC_init(1236, 8888, "127.0.0.1");

    RPC_put(&rpc, 1, 1234); // 0 -> 1
    rpc.seq_number--;       // 1 -> 0
    RPC_put(&rpc, 1, 2345); // i == last_seq_num return result (v) // does not put 2345 in 1st index

    int value = RPC_get(&rpc, 1); // 1234 (value)  0 -> 1
    printf("put get: %d\n", value); 

    rpc.seq_number--;   // 1 -> 0

    value = RPC_get(&rpc, 1); // this will not do any get operation, just return result from call table entry 1234
    printf("put get: %d\n", value); // 1234

    RPC_close(&rpc);
}
