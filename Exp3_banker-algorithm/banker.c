# include <stdio.h>
# include <stdlib.h>


// 全局变量
int PS_NUM;			// 进程数
int RSC_NUM;		// 资源种类数
int* Available;     // 各种资源的可用数量
int** Max;          // 每个进程运行所需的各类资源的最大数量
int** Allocation;   // 每个进程已获得的各类资源数量

/**
 * @brief 从文件中读取确定数量的数字，存储为一个int类型数组，打印为一行并返回
 * @param f 文件流指针
 * @param size 读取数字数量（数组大小）
 */
int* read_output_row(FILE** f, int size) {
    int* sub = (int*)malloc(RSC_NUM * sizeof(int));
    for (int i = 0; i < size; i++) {
        fscanf(*f, "%d ", &sub[i]);
        printf("%d\t", sub[i]);
    }
    printf("\n");
    return sub;
};

/**
 * @brief 读取文件输入，初始化分配状态
 * @param filename 输入文件名
 */
void state_initial(char* filename) {

    FILE* f = fopen(filename, "r");

    fscanf(f, "%d %d ", &PS_NUM, &RSC_NUM);
    printf("Process num: %d\nResource num: %d\n", PS_NUM, RSC_NUM);

    printf("Avaiable resouce:\n");
    Available = read_output_row(&f, RSC_NUM);

    Max = (int**)malloc(PS_NUM * sizeof(int*));
    printf("Max request num:\n");
    for (int i = 0; i < PS_NUM; i++) {
        Max[i] =read_output_row(&f, RSC_NUM);
    }
    Allocation = (int**)malloc(PS_NUM * sizeof(int*));
    printf("Allocated num:\n");
    for (int i = 0; i < PS_NUM; i++) {
        Allocation[i] = read_output_row(&f, RSC_NUM);
    }

    fclose(f);
    printf("\n");
}

/**
 * @brief 比较两个长度相同的整型数组的大小；定义：由前向后遍历，只要有一位x大于y则x <= y不成立；否则x <= y成立
 * @return
 *     - 0: x <= y不成立
 * @return
 *     - 1: x <= y成立
 */
int less_or_equal_than(int* x, int* y, int size) {
    for (int i = 0; i < size; i++) {
        if (x[i] > y[i]) return 0;
    }
    return 1;
}

/**
 * @brief 对int数组做逐位加减运算
 * @param type
 *  -1: 减法
 *  1: 加法
 */
void int_list_basic(int* x, int* y, int size, int type) {
    for(int i = 0; i < size; i++) {
        x[i] += type * y[i];
    }
}

/**
 * @brief 获得某一进程p[i]对应的need[i]
 * @param pidx 进程索引
 * @return need[i]
 */
int* get_need_idx(int pidx) {
    int* need_i = (int*)malloc(RSC_NUM * sizeof(int));
    for (int j = 0; j < RSC_NUM; j++) {
        need_i[j] = Max[pidx][j] - Allocation[pidx][j];
    }
    return need_i;
}

/**
 * @brief 资源预分配
 * @param pidx 进程索引
 * @param request 进程的资源请求数组
 * @return
 *     - -1: 分配失败
 * @return
 *     - 0: 分配成功
 */
int pre_allocate(int pidx, int* request) {
    // 判断预分配后进程占有资源是否超出系统资源上限
    if (!less_or_equal_than(request, Available, RSC_NUM)) {
        printf("Pre_allocation error: intend to allocate resource exceeding the system limit.\n\n");
        return -1;
    }

    // 初始化need数组
    int* need = get_need_idx(pidx);
    // 判断预分配后进程占有资源是否会超出声明的最大使用量
    if (!less_or_equal_than(request, need, RSC_NUM)) {
        printf("Pre_allocation error: intend to allocate resource exceeding the claimed limit.\n\n");
        return -1;
    }

    int_list_basic(Available, request, RSC_NUM, -1);
    int_list_basic(Allocation[pidx], request, RSC_NUM, 1);
    printf("Pre_allocation finished.\n\n");
    return 0;
}

/**
 * @brief 状态回退
 * @param pidx 进程索引
 * @param request 进程的资源请求数组
 */
void rollback(int pidx, int* request) {
    int_list_basic(Available, request, RSC_NUM, 1);
    int_list_basic(Allocation[pidx], request, RSC_NUM, -1);
    printf("Rollback executed.\n\n");
}

/**
 * @brief 安全状态检查函数
 * @return
 *     - 0 状态不安全
 * @return
 *     - 1 状态安全
 */
int state_safe() {
    // 构造finished、safe_seq（安全序列）、work、need数组
    int* finished = (int*)malloc(PS_NUM * sizeof(int*));
    for (int i = 0; i < PS_NUM; i++) {
        finished[i] = 0;
    }

    int* safe_seq = (int*)malloc(PS_NUM * sizeof(int*));
    for (int i = 0; i < PS_NUM; i++) {
        safe_seq[i] = -1;
    }

    int* work = (int*)malloc(RSC_NUM * sizeof(int));
    for (int i = 0; i < RSC_NUM; i++) {
        work[i] = Available[i];
    }

    int** need = (int**)malloc(PS_NUM * sizeof(int*));
    for (int i = 0; i < PS_NUM; i++) {
        need[i] = get_need_idx(i);
    }

    // 执行安全检查算法
    int order = 0;  // 安全序列中的索引
    for (int i = 0; i < PS_NUM; i++) {
        if (!finished[i] && less_or_equal_than(need[i], work, RSC_NUM)) {
            int_list_basic(work, Allocation[i], RSC_NUM, 1);
            finished[i] = 1;
            safe_seq[order++] = i;
            i = -1;
        }
    }
    // 判断状态是否安全，不安全则打印need中无法满足need[i] <= work的项
    if (order < PS_NUM) {	// 这一条件成立等价于并非所有finished[i] == 1
        printf("State NOT safe.\n  - Work:   \t");
        for (int j = 0; j < RSC_NUM; j++) {
            printf("%d\t", work[j]);
        }
        for (int i = 0; i < PS_NUM; i++) {
            if (!finished[i]) {
                printf("\n  - Need[%d]: \t", i);
                for (int j = 0; j < RSC_NUM; j++) {
                    printf("%d\t", need[i][j]);
                }
            }
        }
        printf("\n\n");
        return 0;
    }

    printf("State safe.\nSafe sequence: \t");
    for (int i = 0; i < PS_NUM; i++) {
        printf("%d\t", safe_seq[i]);
    }
    printf("\n\n");
    return 1;
}

/**
 * @brief 银行家算法 先尝试预分配资源，不成功则返回；而后检测状态安全性，不安全则回退
 * @param pidx 进程索引
 * @param request 进程的资源请求数组
 */
void banker(int pidx, int* request) {
    if (pre_allocate(pidx, request)) {
        return;
    }
    if (!state_safe()) {
        rollback(pidx, request);
    }
}


int main() {
    // 测试用例1基础上的测试
    printf("____TEST 1____\n");
    state_initial("example_1.txt");
    state_safe();

    printf("--Request 1--\n");
    int request_1[] = {2, 3, 2};
    banker(1, request_1);

    printf("--Request 2--\n");
    int request_2[] = {1, 4, 1};
    banker(2, request_2);

    printf("--Request 3--\n");
    int request_3[] = { 1, 3, 1 };
    banker(0, request_3);
    
    printf("--Request 4--\n");
    int request_4[] = { 1, 2, 1 };
    banker(4, request_4);

    // 测试用例2基础上的测试
    printf("____TEST 2____\n");
    state_initial("example_2.txt");
    state_safe();
}