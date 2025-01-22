#include "work.h"

volatile int turn = 0, flag[2] = {0, 0};
volatile int thread_cnt = 0;

void* runner_3(void* arg);

int main() {
    printf("____TASK 02 peterson____\n");

    worker(runner_3);
}

void* runner_3(void* arg) {
    int idx = thread_cnt++;
    for (int i = 0; i < y; i++) {
        flag[idx] = 1;
        turn = !idx;
        while(flag[!idx] && turn == !idx) {};
        cnt++;
        flag[idx] = 0;
    }
}