#include <stdlib.h>
#include <stdio.h>

#include "io_sys.h"


/**
 * @brief 按索引读取文件描述符内容
 */
fd read_fd (short i) {
    uchar *fd_block = (uchar*)malloc(FD_SIZE);
    short offset = FD_BASE + i * FD_BLOCK_SIZE;
    // 逐个块读取
    for(short j = 0; j < FD_BLOCK_SIZE; j++) {
        read_block(offset + j, fd_block + j * BLOCK_SIZE);
    }

    // 完成后转换为fd类型
    fd fd_;
    memcpy(&fd_, fd_block, FD_SIZE);
    free(fd_block);
    return fd_;
}

/**
 * @brief 按索引写入文件描述符内容
 */
void write_fd (fd *fd_, short i) {
    uchar *fd_block = (uchar*)fd_;
    short offset = FD_BASE + i * FD_BLOCK_SIZE;
    // 逐个块写入
    for (short j = 0; j < FD_BLOCK_SIZE; j++) {
        write_block(offset + j, fd_block + j * BLOCK_SIZE);
    }
}

/**
 * @brief 在目录中根据文件名查找对应文件描述符
 */
short filename_to_fd (fr *dir, char *filename) {
    for(short i = 0, j = 0; j < file_count; i++) {
        if(dir[i].fd_index != -1 ) {
            j++;
            if(!strcmp(filename, dir[i].filename)){
                return dir[i].fd_index;
            }
        }
    }
    return 0;
}

/**
 * @brief 在位图区间中查找空闲项
 * @param group 1: 查找文件描述符位图 0: 查找逻辑块位图
 * @param k 请求的空闲项数量
 */
short *bitmap_search_and_acquire (uchar *bitmap, short group, short k) {
    short start, end;
    if(group)  {
        start = FD_BITMAP_BASE, end = FD_BITMAP_END;
    } else {
        start = BLOCK_BITMAP_BASE, end = FD_BITMAP_BASE;
    }
    short *res =(short*)malloc(k * sizeof(short)), res_index = 0;
    // 缓存检索到的非0字节索引及其在分配后的情况，若全部空闲项分配成功则据此更新位图
    short *cache_byteoff = (short*)malloc(k * sizeof(short)), cache_index = 0;
    uchar *cache_uchar = (uchar*)malloc(k * sizeof(uchar));
    for (short byte_off = start; byte_off < end && k; byte_off++) {
        // 查找非0字节
        if (bitmap[byte_off]) {
            uchar byte = bitmap[byte_off];
            short bit_off = 0;
            for (; bit_off < 8 && k; bit_off++) {
                if (byte & 0x80) {
                    res[res_index++] = byte_off * 8 + bit_off - start * 8;
                    k--;
                }
                byte <<= 1;
            }
            byte >>= bit_off;
            cache_byteoff[cache_index] = byte_off;
            cache_uchar[cache_index] = byte;
            cache_index++;
        }
    }

    if(k) {
        return NULL;
    } else {
        for(short i = 0; i < cache_index; i++) {
            bitmap[cache_byteoff[i]] = cache_uchar[i];
        }
        return res;
    }
}

/**
 * @brief 将位图中指定的位置设置为空闲项
 * @param fd_index 文件描述符索引
 * @param block_indices 磁盘逻辑块索引
 */
void bitmap_loc_and_free (uchar *bitmap, short fd_index, short *block_indices) {
    // 释放文件描述符
    short byte_off = FD_BITMAP_BASE + fd_index / 8, bit_off = fd_index % 8;
    bitmap[byte_off] |= 1 << bit_off;

    // 释放磁盘逻辑块
    for(short i = 0; block_indices[i] != -1; i++) {
        byte_off = BLOCK_BITMAP_BASE + block_indices[i] / 8, bit_off = block_indices[i] % 8;
        bitmap[byte_off] |= 1 << bit_off;
    }
}


// 文件操作符
typedef struct file_operator {
    short fd_index;                 // 文件描述符索引
    fd fd;                          // 文件描述符(文件大小，文件占用逻辑块索引)
    int position;                   // 当前操作位置(偏移字节数)
} fo;
const short FO_SIZE = sizeof(fo);

short open_files;                        // 已打开文件数
fo *current_fos[FILE_CAPACITY];          // 已打开文件操作符

/**
 * @brief 将文件操作符追加到已打开文件列表中
 */
void fo_append (fo *file) {
    for(int i = 0; i < FILE_CAPACITY; i++) {
        if(current_fos[i] == NULL) {
            current_fos[i] = file;
            open_files++;
            break;
        }
    }
}

/**
 * @brief 检查文件操作符是否在已打开文件列表中
 */
int fo_exists (fo *file, int set_to_null) {
    for(int i = 0, j = 0; j < open_files; i++) {
        if(current_fos[i] != NULL) {
            j++;
            if(current_fos[i] == file) {
                if(set_to_null) {
                    current_fos[i] = NULL;
                    open_files--;
                }
                return 1;
            }
        }
        
    }
    return 0;
}


/**
 * @brief 对字节格式的文件长度进行单位转换
 */
void transform(double size, char *res) {
    const char *suffix[] = {"K", "M"};
    int suf_index = -1;
    while(size >= 1024.0) {
        size /= 1024.0;
        suf_index++;
    }
    if(suf_index == -1) {
        sprintf(res, "%.0f", size);
    } else{
        sprintf(res, "%.2f%s", size, suffix[suf_index]);
    }
}
