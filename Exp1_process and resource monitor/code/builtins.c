#include "builtins.h"


char *builtin_strs[] = {
    "create",
    "destroy",
    "activate",
    "request",
    "release",
    "request_io",
    "io_completion",
    "timeout",
    "list_p",
    "list_r",
    "help",
    "exit_shell",
    "clear_cmd"
};

int (*builtin_func[]) (char **) = {
    &create,
    &destroy,
    &activate,
    &request,
    &release,
    &request_io,
    &io_completion,
    &timeout,
    &list_p,
    &list_r,
    &help,
    &exit_shell,
    &clear_cmd
};

int argc;

int num_builtins () {
    return sizeof(builtin_strs) / sizeof(char *);
}

bool param_error (int num) {
    if (argc != num) {
        fprintf(stderr, "Error: Args Count Not Match.\n");
        return true;
    }
    return false;
}

/**
 * @brief 创建进程
 * @param args args[1]: name  args[2]: pri
 * @return 0代表shell继续运行，1代表终止
 */
int create (char **args) {
    if (param_error(3)) return 0;
    char* name = args[1];
    if(strlen(name) > PS_NAME_SIZE) {
        fprintf(stderr, "Error: Process Name Too Long.\n");
        return 0;
    }
    int pri = atoi(args[2]);
    ps *_ps = make_process(name, pri);
    if (_ps != NULL) {
        for(int i = 0; i < PS_LIST_SIZE; i++) {
            if(ps_list[i] == NULL) {
                ps_list[i] = _ps;
                break;
            }
        }
        printf("Process %s created with PID: %d  Priority: %s\n", name, ps_cnt, pspri_names[pri]);
        // 加入就绪队列
        psqe *_psqe = make_psqe(_ps);
        psq_push(&ready_q[pri], _psqe);
        ps_cnt++;
    }
    return 0;
}

/**
 * @brief 结束进程
 * @param args args[1]: pid
 * @return 0代表shell继续运行，1代表终止
 */
int destroy (char **args) {
    if (param_error(2)) return 0;

    int pid = atoi(args[1]);
    ps *_ps = NULL;
    int idx = -1;
    for(int i = 0; i < PS_LIST_SIZE; i++) {
        if(ps_list[i] != NULL && ps_list[i]->pid == pid) {
            _ps = ps_list[i];
            idx = i;
            break;
        }
    }
    // 检查进程是否存在
    if (_ps == NULL) {
        fprintf(stderr, "Error: Process Not Exist.\n");
        return 0;
    }
    
    // 检查进程的状态（只有就绪的进程才能被结束）
    if (_ps->state != READY) {
        fprintf(stderr, "Error: Illegal Destruction.\n");
        return 0;
    }

    printf("Process %s (PID: %d) is being destroyed.\n", _ps->name, pid);
    ps_list[idx] = NULL; 
    psqe *_psqe = psq_pop(&ready_q[_ps->pri], pid);
    SAFE_DELETE(_ps);
    SAFE_DELETE(_psqe);

    return 0;
}

/**
 * @brief 调度进程
 * @param args args[1]: pid
 */
int activate (char **args) {
    if (param_error(2)) return 0;
    // 检查并发进程数
    if (cc_ps == MAX_CC_PS) {
        fprintf(stderr, "Error: Too Many Processes Running.\n");
        return 0;
    }
    
    int pid = atoi(args[1]);
    ps *_ps = get_process(pid);
    // 检查进程是否存在
    if (_ps == NULL) {
        fprintf(stderr, "Error: Process Not Exist.\n");
        return 0;
    }
    // 检查进程的状态（只有就绪的进程才能被调度）
    if (_ps->state != READY) {
        fprintf(stderr, "Error: Illegal Activation.\n");
        return 0;
    }

    printf("Activating process %s (PID: %d)\n", _ps->name, pid);
    _ps->state = RUNNING;     // 修改状态为就绪
    // 从就绪队列中移除
    psqe *_psqe = psq_pop(&ready_q[_ps->pri], pid);
    // 加入运行队列
    psq_push(&running_q, _psqe);
    cc_ps++;
    return 0;
}

/**
 * @brief 请求资源
 * @param args args[1]: pid  args[2]: resource name
 */
int request (char **args) {
    if (param_error(3)) return 0;
    // 确定进程
    int pid = atoi(args[1]);
    ps *_ps = get_process(pid);
    if (_ps == NULL) {
        fprintf(stderr, "Error: Process Not Exist.\n");
        return 0;
    }

    // 检查进程的状态（只有运行中的进程才能请求资源）
    if (_ps->state != RUNNING) {
        fprintf(stderr, "Error: Illegal request.\n");
        return 0;
    }

    // 确定要请求的资源
    char *rsc_name = args[2];
    int rsc_id = -1;
    for(int i = 0; i < RSC_TYPES; i++) {
        if(rsc_name[0] == resource_names[i][0]) {
            rsc_id = i;
            break;
        }
    }
    if(rsc_id == -1) {
        fprintf(stderr, "Error: Resource Not Exist.\n");
        return 0;
    }

    // 检查资源是否已请求过
    if(_ps->rsc[rsc_id]) {
        fprintf(stderr, "Error: Process Already Requested The Resource.\n");
        return 0;
    }

    _ps->rsc[rsc_id] = true;
    
    // 检查资源的占用情况
    int rsc_pid = rsc_ps_pids[rsc_id];
    if(rsc_pid != -1) {
        // 资源已被占用
        ps *_ps_rsc = get_process(rsc_pid);
        if(_ps->pri >= _ps_rsc->pri || _ps_rsc->state == BLOCKED) {
            // 当前占用资源进程优先级较高或与申请进程相等，又或者占用资源进程处于阻塞状态，则阻塞
            printf("Process %s (PID: %d) is blocked by Process %s (PID: %d) for resource %s.\n",
                _ps->name, pid, _ps_rsc->name, rsc_pid, resource_names[rsc_id]);
            // 加入资源等待队列
            _ps->state = BLOCKED;
            psqe *_psqe = psq_pop(&running_q, pid);
            psq_push(&rsc_q[rsc_id][_ps->pri], _psqe);
            cc_ps--;
        } else {
            // 申请进程优先级较高，则抢占
            printf("Process %s (PID: %d) preempted Process %s (PID: %d) for resource %s.\n",
                _ps->name, pid, _ps_rsc->name, rsc_pid, resource_names[rsc_id]);
            // 被抢占的进程阻塞，加入资源等待队列
            _ps_rsc->state = BLOCKED;
            psqe *_psqe = psq_pop(&running_q, rsc_pid);
            psq_push(&rsc_q[rsc_id][_ps_rsc->pri], _psqe);
            // 重新分配资源
            rsc_ps_pids[rsc_id] = pid;
        }
    } else {
        // 资源未被占用
        rsc_ps_pids[rsc_id] = pid;
        printf("Process %s (PID: %d) acquired resource %s.\n", _ps->name, pid, resource_names[rsc_id]);
    }

    return 0;
    
}

/**
 * @brief 主动释放资源
 * @param args args[1]: pid  args[2]: resource name
 */
int release (char **args) {
    if(param_error(3)) return 0;
    // 确定进程
    int pid = atoi(args[1]);
    ps *_ps = get_process(pid);
    if(_ps == NULL) {
        fprintf(stderr, "Error: Process Not Exist.\n");
        return 0;
    }

    // 确定要释放的资源
    char *rsc_name = args[2];
    int rsc_id = -1;
    for(int i = 0; i < RSC_TYPES; i++) {
        if(rsc_name[0] == resource_names[i][0]) {
            rsc_id = i;
            break;
        }
    }
    if(rsc_id == -1) {
        fprintf(stderr, "Error: Resource Not Exist.\n");
        return 0;
    }

    // 检查当前进程是否在使用该资源
    if(rsc_ps_pids[rsc_id] != pid) {
        fprintf(stderr, "Error: Process Not Using Current Resource.\n");
        return 0;
    }

    // 检查资源是否已请求过
    if(!_ps->rsc[rsc_id]) {
        fprintf(stderr, "Error: Process Did Not Requested The Resource.\n");
        return 0;
    }

    // 释放资源
    _ps->rsc[rsc_id] = false;
    rsc_ps_pids[rsc_id] = -1;
    printf("Process %s (PID: %d) released resource %s.\n", _ps->name, pid, resource_names[rsc_id]);

    // 如果有并发空间，从最高优先级开始，检查资源等待队列
    if(cc_ps < MAX_CC_PS) {
        psqe **rsc_q_i = rsc_q[rsc_id];
        for(int i = 0; i < PS_PRIORITY_NUM; i++) {
            if(rsc_q_i[i] != NULL) {
                ps *_ps_rsc = rsc_q_i[i]->data;
                // 调度第一个等待的进程
                printf("Activating process %s (PID: %d) from resource waiting queue.\n", _ps_rsc->name, _ps_rsc->pid);
                _ps_rsc->state = RUNNING;
                psqe *_psqe = psq_pop(&rsc_q_i[i], _ps_rsc->pid);
                psq_push(&running_q, _psqe);
                cc_ps++;
                // 重新分配资源
                rsc_ps_pids[rsc_id] = _ps_rsc->pid;
                break;
            }
        }
    }
    return 0;
}

/**
 * @brief 请求I/O资源
 * @param args args[1]: pid
 */
int request_io (char **args) {
    if(param_error(2)) return 0;
    // 确定进程
    int pid = atoi(args[1]);
    ps *_ps = get_process(pid);
    if(_ps == NULL) {
        fprintf(stderr, "Error: Process Not Exist.\n");
        return 0;
    }

    // 检查进程的状态（只有运行中的进程才能请求I/O资源）
    if(_ps->state != RUNNING) {
        fprintf(stderr, "Error: Illegal request.\n");
        return 0;
    }

    // 检查I/O资源是否已请求过
    if(_ps->io) {
        fprintf(stderr, "Error: Process Already Requested I/O Resource.\n");
        return 0;
    }

    _ps->io = true;
    _ps->state = BLOCKED;
    psqe *_psqe = psq_pop(&running_q, pid);
    cc_ps--;

    // 检查I/O资源的占用情况
    if(io_ps != NULL) {
        // I/O资源已被占用，不抢占，等待I/O资源释放
        ps *_ps_io = io_ps->data;
        printf("Process %s (PID: %d) is blocked by Process %s (PID: %d) for I/O resource.\n",
            _ps->name, pid, _ps_io->name, _ps_io->pid);
        psq_push(&io_q[_ps->pri], _psqe);
    } else {
        // I/O资源未被占用
        io_ps = _psqe;
        printf("Process %s (PID: %d) acquired I/O resource.\n", _ps->name, pid);
    }

    return 0;
}

/**
 * @brief I/O操作完成
 * @param args args[1]: pid
 */
int io_completion(char **args) {
    if(cc_ps == MAX_CC_PS) {
        fprintf(stderr, "Error: Too Many Processes Running.\n");
        return 0;
    }

    if(param_error(2)) return 0;
    // 确定进程
    int pid = atoi(args[1]);
    ps *_ps = get_process(pid);
    if(_ps == NULL) {
        fprintf(stderr, "Error: Process Not Exist.\n");
        return 0;
    }

    // 检查进程当前是否正在进行I/O操作
    if(io_ps->data->pid != pid) {
        fprintf(stderr, "Error: Process Not In I/O Operation.\n");
        return 0;
    }

    // 释放I/O资源
    _ps->io = false;
    _ps->state = RUNNING;
    psq_push(&running_q, io_ps);
    cc_ps++;    
    io_ps = NULL;
    printf("Process %s (PID: %d) completed I/O operation.\n", _ps->name, pid);

    // 从最高优先级开始，检查I/O资源等待队列
    for(int i = 0; i < PS_PRIORITY_NUM; i++) {
        if(io_q[i] != NULL) {
            ps *_ps_io = io_q[i]->data;
            // 调度第一个等待的进程
            printf("Allocate I/O resource to process %s (PID: %d) from I/O waiting queue.\n", _ps_io->name, _ps_io->pid);
            _ps_io->state = BLOCKED;
            psqe *_psqe = psq_pop(&io_q[i], _ps_io->pid);
            io_ps = _psqe;
            break;
        }
    }

    return 0;
}

/**
 * @brief 进程进行计时器中断，释放所有资源，转为就绪态
 * @param args args[1]: pid
 */
int timeout(char **args) {
    if(param_error(2)) return 0;
    // 确定进程
    int pid = atoi(args[1]);
    ps *_ps = get_process(pid);
    if(_ps == NULL) {
        fprintf(stderr, "Error: Process Not Exist.\n");
        return 0;
    }

    // 只有运行中的进程才能进行计时器中断
    if (_ps->state != RUNNING) {
        fprintf(stderr, "Error: Illegal Timeout.\n");
        return 0;
    }

    // 恢复进程初始状态
    printf("Process %s (PID: %d) timeout and releasing all resources.\n", _ps->name, pid);
    psqe *_psqe = psq_pop(&running_q, pid);
    _ps->state = READY;
    psq_push(&ready_q[_ps->pri], _psqe);
    cc_ps--;

    for(int i = 0; i < RSC_TYPES; i++) {
        _ps->rsc[i] = false;
        rsc_ps_pids[i] = -1;
        if(cc_ps < MAX_CC_PS) {
            psqe **rsc_q_i = rsc_q[i];
            for(int j = _ps->pri; j < PS_PRIORITY_NUM; j++) {
                if(rsc_q_i[j] != NULL) {
                    ps *_ps_rsc = rsc_q_i[j]->data;
                    // 调度第一个等待的进程
                    printf("Activating process %s (PID: %d) from resource waiting queue.\n", _ps_rsc->name, _ps_rsc->pid);
                    _ps_rsc->state = RUNNING;
                    psqe *_psqe = psq_pop(&rsc_q_i[j], _ps_rsc->pid);
                    psq_push(&running_q, _psqe);
                    cc_ps++;
                    // 重新分配资源
                    rsc_ps_pids[i] = _ps_rsc->pid;
                    break;
                }
            }
        }
    }

    return 0;
}

/**
 * @brief 列出当前进程信息
 */
int list_p (char **args) {
    if(param_error(1)) return 0;
    printf(" ______________________________________________________________\n");
    printf("|__PID__|__Name_________________________|_Priority_|___State___|\n");
    for(int i = 0; i < PS_LIST_SIZE; i++) {
        ps *_ps = ps_list[i];
        if(_ps != NULL) {
            printf("|  %3d  |%-31s|  %6s  |  %7s  |\n", 
                _ps->pid, _ps->name, pspri_names[_ps->pri], pst_names[_ps->state]);
        }
    }
    printf("|_______|_______________________________|__________|___________|\n");
    return 0;
}

/**
 * @brief 列出当前资源信息
 */
int list_r (char **args) {
    if(param_error(1)) return 0;
    printf(" _____________________________\n");
    printf("|_Resource_|_Occupied_Process_|\n");
    for(int i = 0; i < RSC_TYPES; i++) {
        printf("| %5s    |", resource_names[i]);
        if(rsc_ps_pids[i] == -1) {
            printf("  -               |\n");
        } else {
            printf("  %3d             |\n", rsc_ps_pids[i]);
        }
    }
    printf("| %5s    |", "I/O");
    if(io_ps == NULL) {
        printf("  -               |\n");
    } else {
        printf("  %3d             |\n", io_ps->data->pid);
    }
    printf("|__________|__________________|\n");
    return 0;
}

/**
 * @brief 帮助；打印所有的内置函数信息
 */
int help (char **args) {
    if(param_error(1)) return 0;
    printf("--------404's Shell--------\n"
           "Type arguments and hit Enter.\n"
           "Built-in Functions:\n"
    );
    for (int i = 0; i < num_builtins(); i++) {
        printf(" - %s\n", builtin_strs[i]);
    }
    return 0;
}

/**
 * @brief 退出shell
 */
int exit_shell (char **args) {
    if (param_error(1)) return 0;
    return 1;
}

/**
 * @brief 清空shell显示内容
 */
int clear_cmd (char **args) {
    if (param_error(1)) return 0;
    clear();
    return 0;
}
