#include <string.h>

#include "config.h"

#ifndef IO_SYS_H
#define IO_SYS_H


/**
 * @brief 拷贝逻辑块ldisk[i]到以指针p指定的位置开始的内存中，拷贝的字符数和块长度相等
 * @param i 逻辑块索引
 * @param p 目标起始内存地址指针
 */
void read_block (int i, void *p) {
    memcpy(p, ldisk[i].data, BLOCK_SIZE);
}

/**
 * @brief 从以指针p指定的位置所开始的内存中，拷贝和块长度相等的字符数到逻辑块ldisk[i]中
 * @param i 逻辑块索引
 * @param p 目标起始内存地址指针
 */
void write_block(int i, void *p) {
    memcpy(ldisk[i].data, p, BLOCK_SIZE);
}

#endif
