#include <stdbool.h>
#include <string.h>

#include "resource.h"
#include "utils.h"


/* 进程相关定义 */
#define PS_LIST_SIZE 127    // 进程列表大小
#define PS_PRIORITY_NUM 3   // 进程优先级数量
#define PS_NAME_SIZE 31     // 进程名长度

/* 进程优先级 */
typedef enum process_priority {
    HIGH, COMMON, LOW        // 高，普通，低
} pspri;
extern const char *pspri_names[];

/* 进程状态 */
typedef enum process_state {
    READY, RUNNING, BLOCKED   // 就绪，运行，阻塞
} pst;
extern const char *pst_names[];

/* 进程 */
typedef struct process {
    int pid;                    // 进程ID
    char name[PS_NAME_SIZE + 1];// 进程名
    pspri pri;                  // 进程优先级
    pst state;                  // 进程状态
    bool rsc[RSC_TYPES];        // 进程请求的资源（未必处在占用状态，是否占用要通过rsc_ps判断）
    bool io;                    // 是否正在请求I/O操作
} ps;

/* 进程队列（链表）元素 */
typedef struct process_queue_element {
    ps *data;                              // 进程
    struct process_queue_element *next;    // 下一个元素
} psqe;

extern int ps_cnt;                  // 累计进程数目，用于生成PID
extern ps *ps_list[PS_LIST_SIZE];   // 进程列表

/* 进程队列
    - 结构：进程队列元素的链表头数组，三个链表对应三个不同优先级 */
extern const int MAX_CC_PS;                        // 最大并发进程数
extern int cc_ps;                                  // 当前并发进程数
extern psqe *ready_q[PS_PRIORITY_NUM];             // 就绪队列
extern psqe *running_q;                            // 运行队列（此处用队列来模拟多核CPU上运行的进程）
extern psqe *rsc_q[RSC_TYPES][PS_PRIORITY_NUM];    // 资源等待队列（4个资源），因而又是链表头数组的数组
extern psqe *io_q[PS_PRIORITY_NUM];                // I/O资源等待队列
extern psqe *io_ps;                                // 正在进行I/O操作的进程


/**
 * @brief 初始化一个进程指针及其所指对象
 * @param _name 进程名
 * @param _pri 优先级
 * @return 进程指针
 */
ps *make_process (char *_name, int _pri);

/** 
 * @brief 初始化一个进程队列元素指针及其所指对象
 * @param _data 进程指针
 * @return 队列元素指针
*/
psqe *make_psqe (ps *_data);

/**
 * @brief 在进程列表中查找进程
 * @param pid 进程PID
 * @param delete 是否从列表删除
 * @return 进程指针
 * @note 若进程不存在，返回NULL
 * @note 由于进程数目较少，直接遍历查找
 */
ps *get_process (int pid);

/**
 * @brief 进程队列元素入队
 * @param q 队列
 * @param _psqe 队列元素
 */
void psq_push (psqe **q, psqe *_psqe);

/**
 * @brief 进程队列元素出队
 * @param q 队列
 * @param pid 进程PID
 * @return 出队元素
 */
psqe *psq_pop (psqe **q, int pid);
