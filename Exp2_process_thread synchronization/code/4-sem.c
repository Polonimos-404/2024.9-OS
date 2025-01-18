#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#pragma warning( disable : E0028 )

const char* IN_FILE_NAME = "in.txt", * OUT_FILE_NAME = "out.txt";   // 输入和输出文件名
int buf_size;   // 缓存区大小
int* buffer;    // 缓存区
int produced = 0, consumed = 0;     // 生产和消费进度索引
sem_t mutex, empty, full;           // 信号量

void* producer(void* arg);
void* consumer(void* arg);

int main() {
    printf("____TASK 04____\n");

    // 读入缓冲区大小和顾客数量
    FILE* f = fopen(IN_FILE_NAME, "r");
    if (f == NULL) { // f为NULL,则打开文件失败,退出程序
        perror("Failed to open input text");
        return -1;
    }
    int csmr_cnt;
    fscanf(f, "%d %d", &buf_size, &csmr_cnt);
    fclose(f);

    // 设定缓存区大小、信号量初值；清空out.txt
    buffer = malloc(buf_size * sizeof(int));
    for (int i = 0; i < buf_size; i++) {
        buffer[i] = -1;
    }
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, buf_size);
    sem_init(&full, 0, 0);
    f = fopen(OUT_FILE_NAME, "w");
    if (f == NULL) { // f为NULL,则打开文件失败,退出程序
        perror("Failed to open output text");
        return -1;
    } else {
        printf("Output text refreshed.\n");
    }
    fclose(f);


    // 创建和运行线程
    pthread_t producers[csmr_cnt], consumers[csmr_cnt];
    for (int i = 0; i < csmr_cnt; i++) {
        int ret = pthread_create(&producers[i], NULL, producer, (void*)(long)i);
        if (ret) {
            printf("Pthread_create error: error_code = %d\n", ret);
            return -1;
        }
        printf("Thread [p%d] created successfully, current thread id: %#lx\n", i, producers[i]);

        ret = pthread_create(&consumers[i], NULL, consumer, NULL);
        if (ret) {
            printf("Pthread_create error: error_code = %d\n", ret);
            return -1;
        }
        printf("Thread [c%d] created successfully, current thread id: %#lx\n", i, consumers[i]);
    }
    for (int i = 0; i < csmr_cnt; i++) {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);

    printf("\nRESULT:\n");
    // 检查运行结果
    f = fopen(OUT_FILE_NAME, "r");
    if (f == NULL) { // f为NULL,则打开文件失败,退出程序
        perror("Failed to open output text");
        return -1;
    }
    char line[12];
    for (int i = 0; i < 2 * csmr_cnt; i++) {
        fgets(line, 12, f);
        printf("%s", line);
    }
    fclose(f);

    return 0;
}

// 生产者进程
void* producer(void* arg) {
    int idx = (int)(long)arg;
    sem_wait(&empty);
    sem_wait(&mutex);
    buffer[produced % buf_size] = idx;
    produced++;
    FILE* f = fopen(OUT_FILE_NAME, "a");
    fprintf(f, "p%d\n", idx);
    fclose(f);
    sem_post(&mutex);
    sem_post(&full);
}

// 消费者进程
void* consumer(void* arg) {
    sem_wait(&full);
    sem_wait(&mutex);
    int idx = buffer[consumed % buf_size];
    consumed++;
    FILE* f = fopen(OUT_FILE_NAME, "a");
    fprintf(f, "c%d\n", idx);
    fclose(f);
    sem_post(&mutex);
    sem_post(&empty);
}