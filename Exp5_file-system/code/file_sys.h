#include "utils.h"


/**
 * @brief 使用指定的名称创建一个新文件
 */
int create (char *filename) {
    // 检查文件长度是否超过限制
    if(strlen(filename) > MAX_FILENAME_LEN) {
        printf("Name error: exceeds lenth limit\n");
        return 1;
    }
    // 检查文件数量是否达到上限
    if(file_count == FILE_CAPACITY) {
        printf("Directory error: no enough space for new file record\n");
        return 1;
    }

    // 检查是否存在重名文件
    fd dir_fd = read_fd(0);
    fr *dir = (fr*)(&dir_fd);
    if(filename_to_fd(dir, filename) != 0) {
        printf("Name error: file with the same name already exists\n");
        return 1;
    }

    // Step 1: 读/写位图
    uchar *bitmap = (uchar*)malloc(BLOCK_SIZE);
    read_block(BITMAP_BASE, bitmap);

    // 在位图中查找空闲块，并标记为占用
    short *block_index_ptr = bitmap_search_and_acquire(bitmap, 0, 1);
    if(block_index_ptr == NULL) {
        printf("Capacity error: no enough space to store new file\n");
        return 1;
    }
    short block_index = block_index_ptr[0];
    free(block_index_ptr);
    // 在位图中查找空闲文件描述符，并标记为占用
    short fd_index = bitmap_search_and_acquire(bitmap, 1, 1)[0] + 1;

    write_block(BITMAP_BASE, bitmap);

    // Step 2: 写入文件描述符
    fd fd_;
    memset(&fd_, -1, FD_SIZE);
    fd_.size = 0;
    fd_.blocks[0] = block_index;
    write_fd(&fd_, fd_index);

    // Step 3: 读/写目录记录项
    strcpy(dir[fd_index - 1].filename, filename);
    dir[fd_index - 1].fd_index = fd_index;
    write_fd(&dir_fd, 0);

    file_count++;

    printf("File created: %s\n", filename);

    return 0;
}

/**
 * @brief 删除指定名称的文件
 */
int destroy (char *filename) {
    // Step 1: 读/写目录记录项（必须覆写空内容，防止之后在目录中检索到实际已删除的文件）
    fd dir_fd = read_fd(0);
    fr *dir = (fr*)(&dir_fd);
    short fd_index = filename_to_fd(dir, filename);
    if(!fd_index) {
        printf("Name error: no such file in directory\n");
        return 1;
    }

    // 检查文件是否处于打开状态，如果打开则拒绝删除
    for(int i = 0, j = 0; j < open_files; i++) {
        if(current_fos[i] != NULL) {
            j++;
            if(current_fos[i]->fd_index == fd_index) {
                printf("Destroy error: file is currently opened\n");
                return 1;
            }
        }
    }

    memset(&dir[fd_index - 1], -1, FR_SIZE);
    write_fd(&dir_fd, 0);

    // Step 2: 读文件描述符（不需要写入，因为之后再次使用时会覆写，只要在位图中释放即可）
    fd fd_ = read_fd(fd_index);

    // Step 3: 读/写位图
    uchar *bitmap = (uchar*)malloc(BLOCK_SIZE);
    read_block(BITMAP_BASE, bitmap);
    bitmap_loc_and_free(bitmap, fd_index, (short*)(&(fd_.blocks)));
    write_block(BITMAP_BASE, bitmap);

    file_count--;

    printf("File destroyed: %s\n", filename);

    return 0;
}


/**
 * @brief 打开指定名称的文件
 */
fo *open (char *filename) {
    // 查询目录
    fd dir_fd = read_fd(0);
    fr *dir = (fr*)(&dir_fd);
    short fd_index = filename_to_fd(dir, filename);
    if(!fd_index) {
        printf("Name error: no such file in directory\n");
        return NULL;
    }
    // 创建文件操作符
    fd fd_ = read_fd(fd_index);
    fo *fo_ = (fo*)malloc(FO_SIZE);
    fo_->fd_index = fd_index;
    fo_->fd = fd_;
    fo_->position = 0;

    fo_append(fo_);

    printf("File opened: %s\n", filename);

    return fo_;
}

/**
 * @brief 关闭指定文件
 */
int close (fo *file) {
    if(!(fo_exists(file, 1)) || file == NULL) {
        printf("Close error: invalid file operator printer\n");
        return 1;
    }
    free(file);
    file = NULL;

    printf("File closed\n");

    return 0;
}


/**
 * @brief 从指定文件中顺序读取一定数目的字节到内存中
 * @param file 指定的文件操作符
 * @param mem_area 目标内存地址
 * @param count 被读的字节数
 */
int read (fo *file, void *mem_area, int count) {
    if(file == NULL || !fo_exists(file, 0)) {
        printf("Read error: invalid file operator printer\n");
        return 1;
    }

    // Step 1: 查看读取内容开头字节和结尾字节所在的逻辑块在文件描述符中的索引
    int end = file->position + count - 1;
    // 超过文件范围，拒绝读取
    if(end >= file->fd.size) {
        printf("Read error: exceeding the file space\n");
        return 1;
    }
    short start_block_pos = file->position / BLOCK_SIZE;
    short end_block_pos = end / BLOCK_SIZE;

    // Step 2: 将读取内容涉及的逻辑块全部读出
    uchar *blocks = (uchar*)malloc((end_block_pos - start_block_pos + 1) * BLOCK_SIZE);
    for(short i = start_block_pos, j = 0; i <= end_block_pos; i++, j++) {
        read_block(DATA_BLOCKS_BASE + file->fd.blocks[i], blocks + j * BLOCK_SIZE);
    }

    // Step 3: 从中选取所需的部分，分配给内存
    void *start = blocks + (file->position - start_block_pos * BLOCK_SIZE);
    memcpy(mem_area, start, count);

    // Step 4: 调整文件操作符
    file->position += count;

    printf("%d bytes read\n", count);

    return 0;
}

/**
 * @brief 把内存中的一定数目的字节写入到指定的文件中
 * @param file 指定的文件操作符
 * @param mem_area 目标内存地址
 * @param count 要写的字节数
 */
int write (fo *file, void *mem_area, int count) {
    if(file == NULL || !fo_exists(file, 0)) {
        printf("Write error: invalid file operator printer\n");
        return 1;
    }

    // Step 1: 查看写入开头字节和结尾字节所在的逻辑块在文件描述符中的索引
    int end = file->position + count - 1;
    short end_block_pos = end / BLOCK_SIZE;
    if(end_block_pos >= MAX_FILE_BLOCKS) {
        printf("Write error: exceeding the max limit of logic blocks used by a single file\n");
        return 1;
    }
    short start_block_pos = file->position / BLOCK_SIZE;

    // Step 2: 检查结尾字节是否超出了文件大小，执行必要的位图分配
    short new_block_num = (end + 1) / BLOCK_SIZE - file->fd.size / BLOCK_SIZE;
    if(new_block_num > 0) {
        uchar *bitmap = (uchar*)malloc(BLOCK_SIZE);
        read_block(BITMAP_BASE, bitmap);
        short *block_indices = bitmap_search_and_acquire(bitmap, 0, new_block_num);
        for(short i = file->fd.size / BLOCK_SIZE + 1, j = 0; j < new_block_num; i++, j++) {
            file->fd.blocks[i] = block_indices[j];
        }
        write_block(BITMAP_BASE, bitmap);
    }

    // Step 3: 将读取内容涉及的逻辑块全部读出，并将内存中的内容写入涉及的逻辑块
    uchar *blocks = (uchar*)malloc((end_block_pos - start_block_pos + 1) * BLOCK_SIZE);
    for(short i = start_block_pos, j = 0; i <= end_block_pos; i++, j++) {
        read_block(DATA_BLOCKS_BASE + file->fd.blocks[i], blocks + j * BLOCK_SIZE);
    }

    void *start = blocks + (file->position - start_block_pos * BLOCK_SIZE);
    memcpy(start, mem_area, count);
    for(short i = start_block_pos, j = 0; i <= end_block_pos; i++, j++) {
        write_block(DATA_BLOCKS_BASE + file->fd.blocks[i], blocks + j * BLOCK_SIZE);
    }

    // Step 4: 调整文件操作符(操作位置、文件描述符)
    file->position += count;
    if(end + 1 > file->fd.size) {
        file->fd.size = end + 1;
    }
    write_fd(&(file->fd), file->fd_index);

    printf("%d bytes written\n", count);

    return 0;
}


/**
 * @brief 移动文件的当前操作位置
 * @param pos 目标位置
 */
int lseek (fo *file, int pos) {
    if(file == NULL || !fo_exists(file, 0)) {
        printf("Lseek error: invalid file operator printer\n");
        return 1;
    }

    if(pos < 0 || pos > file->fd.size) {
        printf("Pos error: invalid position\n");
        return 1;
    }
    file->position = pos;

    printf("Position set to %d\n", pos);

    return 0;
}


/**
 * @brief 列出所有文件的名字及其长度
 */
void directory () {
    printf("%-34s | %s\n", "- Name", "- Size");
    fd dir_fd = read_fd(0);
    fr *dir = (fr*)(&dir_fd);
    int total_size = 0;     // 文件占用总字节数
    for(short i = 0, j = 0; j < file_count; i++) {
        if(dir[i].fd_index != -1) {
            fd fd_ = read_fd(dir[i].fd_index);
            total_size += fd_.size;
            // 进行长度单位转换以便于识读
            char size_str[41];
            transform(fd_.size, size_str);
            printf("  - %-30s | %7s\n", dir[i].filename, size_str);
            j++;
        }
        
    }
    // 打印占用总空间
    char total_size_str[41];
    transform(total_size, total_size_str);
    printf("%-34s | %7s\n\n", "-- Total", total_size_str);
}


/**
 * @brief 对磁盘进行格式化，每次启动文件系统时调用
 */
void format() {
    ldisk = (block*)malloc(BLOCK_NUM * (BLOCK_SIZE + 1));
    memset(ldisk, -1, BLOCK_NUM * (BLOCK_SIZE + 1));
    uchar *bitmap = (uchar*)ldisk;
    bitmap[FD_BITMAP_BASE - 1] = 0xe0;
    
    file_count = 0;
    open_files = 0;
    for(int i = 0; i < FILE_CAPACITY; i++) {
        current_fos[i] = NULL;
    }

    printf("Disk formatted\n\n");
}
