#include <semaphore.h>

#include "work.h"

sem_t mutex;


void* runner_4(void* arg);

int main() {
    printf("____TASK 03____\n");

    if (sem_init(&mutex, 0, 1) != 0) {
        perror("Semaphore initialization error");
        return -1;
    }

    worker(runner_4);

    sem_destroy(&mutex);
}

void* runner_4(void* arg) {
    for (int i = 0; i < y; i++) {
        sem_wait(&mutex);
        cnt++;
        sem_post(&mutex);
    }
}