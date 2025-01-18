#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 内存块定义
typedef struct block {
    int size;           // 内存块大小，单位：KB
    int allocated;      // 是否分配
    struct block *next; // 下一内存块指针
} block;

const int FACTOR_BIT = 10;    // KB到Byte的换算因数
const int MEMORY_SIZE = 256, PAGE_SIZE = 4;     // 内存空间大小、页面大小（单位：KB）
const int BLOCK_SIZE = sizeof(block); 
block *block_list_head = NULL;      // 内存块链表头指针


// 设定内存块链表头参数
void set_head() {
    block_list_head->size = MEMORY_SIZE;
    block_list_head->allocated = 0;
    block_list_head->next = NULL;
}

// 程序运行基本配置
void set() {
    void *start = (void*)malloc(MEMORY_SIZE << FACTOR_BIT);
    memset(start, 0, MEMORY_SIZE << FACTOR_BIT);
    block_list_head = (block*)start;
    set_head();
}

// 将内存初始化为单一空闲块
void *init() {
    set_head();
    memset(block_list_head + 1, 0, (MEMORY_SIZE << FACTOR_BIT) - BLOCK_SIZE);
    return (void*)block_list_head;
}

// 最佳适应
block *best_fit(int size) {
    block *p = block_list_head, *best = NULL;
    while(p != NULL) {
        if(!p->allocated && p->size >= size && (best == NULL || p->size < best->size)) {
            best = p;
        }
        p = p->next;
    }
    return best;
}

// 最坏适应
block *worst_fit(int size) {
    block *p = block_list_head, *worst = NULL;
    while(p != NULL) {
        if(!p->allocated && p->size >= size && (worst == NULL || p->size > worst->size)) {
            worst = p;
        }
        p = p->next;
    }
    return worst;
}

// 首次适应
block *first_fit(int size) {
    block *p = block_list_head;
    while(p != NULL && (p->allocated || p->size < size)) {
        p = p->next;
    }
    return p;
}

// 申请内存
void *my_malloc(int size, int strategy) {
    // 判断申请合法性
    if(size < 1 || size > MEMORY_SIZE) {
        printf("Malloc error: Address space size invalid.\n");
        return NULL;
    }
    size = ((size - 1) / PAGE_SIZE + 1) * PAGE_SIZE;    // size化为大于size的最小的4的倍数
    block *target = NULL;
    // 选择分配策略
    switch(strategy) {
        case 0:
            target = best_fit(size);
            break;
        case 1:
            target = worst_fit(size);
            break;
        case 2:
            target = first_fit(size);
            break;
        default:
            printf("Malloc error: Strategy index invalid.\n");
            return NULL;
    }

    // 无法满足申请
    if(target == NULL) {
        printf("INFO: Could not allocate.\n");
        return NULL;
    }

    target->allocated = 1;
    // 如果内存块大小大于申请大小，进行分割
    if(target->size > size) {
        block *new = target + (size << FACTOR_BIT) / BLOCK_SIZE;
        new->size = target->size - size;
        new->allocated = 0;
        new->next = target->next;

        target->size = size;
        target->next = new;
    }
    return (void*)target;
}

// 合并空闲内存块
void merge_idle_blocks(block *block1, block *block2) {
    block1->size += block2->size;
    block1->allocated = 0;
    block1->next = block2->next;
    memset(block1 + 1, 0, (block1->size << FACTOR_BIT) - BLOCK_SIZE);
}

// 释放内存块
void my_free(block *sub) {
    block *cur = block_list_head, *last = NULL;
    while(cur != NULL && sub != cur) {
        last = cur;
        cur = cur->next;
    }
    if(cur == NULL) {
        printf("Free error: Illegal access.\n");
        return;
    } else if (!cur->allocated) {
        printf("INFO: Block is idle.\n");
        return;
    }

    if(last != NULL && !last->allocated) {
        merge_idle_blocks(last, sub);
        sub = last;
    }
    if(sub->next != NULL && !sub->next->allocated) {
        merge_idle_blocks(sub, sub->next);
    }
    printf("Block freed.\n");
}

// 打印内存块链表中的空闲块
void print_list() {
    block *cur = block_list_head;
    printf("Idle blocks:\nStart Address\tSize\n");
    while(cur != NULL) {
        if(!cur->allocated) {
            int start_addr = ((cur - block_list_head) * BLOCK_SIZE) >> FACTOR_BIT;
            printf("%d\t\t %d\n", start_addr, cur->size);
        }
        cur = cur->next;
    }
}

void get_cmd() {
    int run = 1;
    while(run) {
        printf("> ");
        char cmd;
        scanf("%c", &cmd);
        switch (cmd) {
        // 按照指定策略分配指定大小的内存块
        case 'a':
            int size, strategy;
            printf("Input the block size needed and the strategy which will be used: ");
            scanf("%d %d", &size, &strategy);
            my_malloc(size, strategy);
            printf("Block allocated.\n");
            break;
        
        // 回收第一个已分配的内存块
        case 'f':
            block *cur = block_list_head->allocated ? block_list_head : block_list_head->next;
            if(cur == NULL) {
                printf("ERROR: No allocated block.\n");
            }
            my_free(cur);
            printf("First allocated block in memory is freed.\n\n");
            break;
        // 初始化内存
        case 'i':
            init();
            printf("Memory recovered to initial state.\n");
            break;
        // 打印空闲块
        case 'p':
            print_list();
            break;
        // 退出程序
        case 'q':
            run = 0;
            break;
        default:
        }
    }
}

int main() {
    set();
    // 设置用例所对应的初始条件
    int block_count = 5;
    int sizes[] = {32, 32, 8, 56, 128}, allocated[] = {0, 1, 0, 1, 0};
    block *cur = block_list_head;
    for(int i = 0; i < block_count; i++) {
        cur->size = sizes[i];
        cur->allocated = allocated[i];
        if(i < block_count - 1) {
            cur->next = cur + (sizes[i] << FACTOR_BIT) / BLOCK_SIZE;
        } else {
            cur->next = NULL;
        }
        cur = cur->next;
    }

    printf("____Memory Management Stimulation System____\n");
    printf("Memory size: %d\nPage size: %d\n", MEMORY_SIZE, PAGE_SIZE);
    printf("- Built_in commands:\n"
           "-- a: allocate memory of given size(KB) param using the strategy(int 0 - 2) param\n"
           "-- f: free the first allocated block to memory\n"
           "-- i: recover memory to initial state, which contains only one idle block\n"
           "-- p: print information on current idle blocks\n"
           "-- q: quit\n\n");
    print_list();
    get_cmd();
    free(block_list_head);
}
