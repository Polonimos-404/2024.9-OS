#include <stdio.h>
#include <stdlib.h>


#define clear() printf("\033[H\033[J")  // 清空shell显示内容

#define SAFE_DELETE(p) if(p) { free(p); (p) = NULL; }  // 安全删除指针
