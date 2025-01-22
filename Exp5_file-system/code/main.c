#include "file_sys.h"


int main () {
    printf("1.格式化磁盘\n");
    format();

    printf("2.初始状态下目录是空的\n");
    directory();

    printf("3.成功和失败的创建\n");
    char *filenames[] = {"111111111111111111111111111111", "0001", "0002", "0001"};
    for(int i = 0; i < 4; i++) {
        create(filenames[i]);
        
    }
    printf("\n");
    directory();

    printf("4.文件打开与写入\n");
    printf("4.1 打开失败\n");
    open("0003");
    printf("4.2 打开成功，写入小段内容\n");
    fo* f1 = open("0001");
    char* message = "Hello World!";
    write(f1, message, strlen(message));
    printf("4.3 -打开成功，写入大段内容（超过一个逻辑块）\n");
    fo* f2 = open("0002");
    char long_message[3001];
    for(int i = 0; i < 3000; i++) {
        long_message[i] = 'a' + (i % 26);
    }
    write(f2, long_message, 3000);
    printf("4.4 查看此时的目录\n");
    printf("\n");
    directory();
    
    printf("5. 移动文件操作位置（失败的和成功的操作）\n");
    lseek(f1, 20);
    lseek(f1, 6);
    printf("\n");

    printf("6. 读取文件内容（失败的和成功的操作）\n");
    char text[100];
    read(f1, text, 10);
    read(f1, text, 5);
    text[5] = '\0';
    printf("Text: %s\n", text);
    printf("-验证当前操作位置确实移动到了预期位置\n");
    read(f1, text, 1);
    text[1] = '\0';
    printf("Text: %s\n", text);
    printf("\n");
    
    printf("7. 关闭文件\n");
    close(f1);

    printf("7.1 关闭后再次尝试进行关闭、读取、写入和移动操作，均会失败\n");
    close(f1);
    read(f1, text, 1);
    write(f1, message, 1);
    lseek(f1, 0);
    
    printf("7.2 对并不处于打开文件列表中的操作符执行关闭、读取、写入、移动操作也都会失败\n");
    fo* f3 = (fo*)malloc(FO_SIZE);
    f3->fd_index = 1;
    f3->position = 0;
    close(f3);
    read(f3, text, 1);
    write(f3, message, 1);
    lseek(f3, 0);
    printf("\n");

    printf("8. 覆盖写入\n");
    lseek(f2, 0);
    char long_message2[52];
    for(int i = 0; i < 52; i++) {
        long_message2[i] = 'A' + (i % 26);
    }
    write(f2, long_message2, 52);
    lseek(f2, 0);
    read(f2, text, 78);
    text[78] = '\0';
    printf("Text: %s\n", text);
    printf("\n");

    printf("9. 删除文件\n");
    printf("9.1 删除已打开或不存在的文件均会失败\n");
    destroy("0003");
    destroy("0002");
    printf("9.2 删除成功\n");
    close(f2);
    destroy("0002");
    printf("\n");
    printf("9.3 查看此时的目录\n");
    directory();
}
