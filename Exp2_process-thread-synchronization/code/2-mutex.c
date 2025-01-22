#include <stdbool.h>

#include "work.h"

volatile bool mutex = false;


void* runner_2_1(void* arg);

int main() {
    printf("____TASK 02 mutex____\n");

    worker(runner_2_1);
}


bool TAS(volatile bool* lock) {     // test_and_set操作
    return __sync_lock_test_and_set(lock, true); 
}

void acquire(volatile bool* lock) {     // 获取锁
    while(TAS(lock)) {};
}

void release(volatile bool* lock) {     // 释放锁
    __sync_synchronize();
    * lock = false;
}

void* runner_2_1(void* arg) {
    for (int i = 0; i < y; i++) {
        acquire(&mutex);
        cnt++;
        release(&mutex);
    }
    return NULL;
}
