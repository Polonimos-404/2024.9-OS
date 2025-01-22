#include "work.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // 使用POSIX互斥锁

void* runner_2_2(void* arg);

int main() {
    printf("____TASK 02 mutex-opt____\n");

    worker(runner_2_2);

    pthread_mutex_destroy(&mutex);
}

void* runner_2_2(void* arg) {
    for (int i = 0; i < y; i++) {
        pthread_mutex_lock(&mutex);
        cnt++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}