#include "work.h"

void* runner_1(void* arg);

int main() {
    printf("____TASK 01____\n");
    
    worker(runner_1);
}

// 线程执行函数
void* runner_1(void* arg) {
    for (int i = 0; i < y; i++) {
        cnt++;
    }
    return NULL;
}
