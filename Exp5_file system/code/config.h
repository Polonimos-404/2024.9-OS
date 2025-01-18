typedef unsigned char uchar;

// 磁盘大小: 8MB
// 逻辑块长度: 1KB  逻辑块数: 8192
#define BLOCK_SIZE 1024
const short BLOCK_NUM = 8192;

# define MAX_FILE_BLOCKS 2046
// 文件描述符
typedef struct file_descriptor {
    int size;                      // 文件大小(字节数)
    short blocks[MAX_FILE_BLOCKS];   // 使用逻辑块
} fd;
// 文件描述符占用的字节数和逻辑块数
const short FD_SIZE = sizeof(fd), FD_BLOCK_SIZE = FD_SIZE / BLOCK_SIZE;


#define MAX_FILENAME_LEN 29
// 目录记录项
typedef struct file_record {
    char filename[MAX_FILENAME_LEN + 1];  // 文件名
    short fd_index;     // 文件描述符索引
} fr;
// 一个文件描述符的空间最多容纳4096B / 32B = 128个目录记录项，所以为文件描述符数组预留129 * 4KB = 516KB的空间
const short FR_SIZE = sizeof(fr);   // 文件容量
#define FILE_CAPACITY 128
short file_count;    // 磁盘内文件数目


// 预留磁盘块数: 517  实际可用存储容量: 7675KB
const short RESERVED_BLOCK_NUM = 517;
const short SPACE_CAPACITY = BLOCK_NUM - RESERVED_BLOCK_NUM;

// 磁盘逻辑块
typedef struct logic_block {
    uchar data[BLOCK_SIZE + 1];
} block;

// 磁盘开头指针
block *ldisk;

// 位图基址，位于磁盘开头，占用一个逻辑块(1KB)，实际使用7675(数据块) + 128(文件描述符)位((959B + 3bit)+(16B))
// 位图中0表示已占用，1表示空闲
const short BITMAP_BASE = 0; // 指首个逻辑块
const short BLOCK_BITMAP_BASE = 0;  // 指首个字节
const short FD_BITMAP_BASE = SPACE_CAPACITY / 8 + 1, FD_BITMAP_END = FD_BITMAP_BASE + FILE_CAPACITY / 8; // 指其中的第961/977个字节

// 文件描述符数组基址
const short FD_BASE = 1;

// 数据块基址
const short DATA_BLOCKS_BASE = RESERVED_BLOCK_NUM;
