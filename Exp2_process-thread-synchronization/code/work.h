#include <pthread.h>
#include <stdio.h>


const char* IN_FILE_NAME = "in.txt", * OUT_FILE_NAME = "out.txt";   // 输入和输出文件名
int y;  // 操作次数
int cnt = 0;    // 操作对象

int worker(void* (*runner)(void*)) {
    // 读入操作次数
    FILE* f = fopen(IN_FILE_NAME, "r");
    if (f == NULL) { // f为NULL,则打开文件失败,退出程序
        perror("Failed to open input text");
        return -1;
    }
    fscanf(f, "%d", &y);
    fclose(f);

    // 创建和运行线程
    pthread_t tid[2] = {0};
    for (int i = 0; i < 2; i++) {
        int ret = pthread_create(&tid[i], NULL, runner, NULL);
        if (ret) {
            printf("Pthread_create error: error_code = %d\n", ret);
            return -1;
        }
        printf("Thread [%d] created successfully, current thread id: %#lx\n", i, tid[i]);
    }
    // 等待线程退出
    for (int i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
    }

    // 写入结果
    f = fopen(OUT_FILE_NAME, "w");
    if (f == NULL) { // f为NULL,则打开文件失败,退出程序
        perror("Failed to open output text");
        return -1;
    }
    fprintf(f, "%d", cnt);
    fclose(f);

    // 判断结果是否符合预期并输出
    printf("\nExpected result: %d\nActual result: %d\n", 2 * y, cnt);
    if (cnt != 2 * y) {
        printf("Not equal.\n");
    }
    else {
        printf("Equal.\n");
    }
    return 0;
} 