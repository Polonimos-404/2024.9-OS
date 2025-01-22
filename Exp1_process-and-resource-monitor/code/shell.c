#include <unistd.h>

#include "shell.h"


void init_shell () {
    printf("Initializing......\n");
    // 初始化资源占用状况
    for(int i = 0; i < RSC_TYPES; i++) {
        rsc_ps_pids[i] = -1;
    }
    io_ps = NULL;

    // 初始化进程列表
    ps_cnt = 0;
    cc_ps = 0;
    for (int i = 0; i < PS_LIST_SIZE; i++) {
        ps_list[i]= NULL;
    }
    // 初始化进程队列
    for (int i = 0; i < PS_PRIORITY_NUM; i++) {
        ready_q[i] = NULL;
        io_q[i] = NULL;
    }
    running_q = NULL;
    for (int i = 0; i < RSC_TYPES; i++) {
        for (int j = 0; j < PS_PRIORITY_NUM; j++) {
            rsc_q[i][j] = NULL;
        }
    }
    io_ps = NULL;

    clear();
    printf(
        "\t ________________________________________\n"
        "\t|                                        |\n"
        "\t|        Welcome To 404's Shell ~        |\n"
        "\t|________________________________________|\n"
        "\t|                                        |\n"
        "\t| Functions:                             |\n"
        "\t|    - Process Management Simulation     |\n"
        "\t|    - Resource Management Simulation    |\n"
        "\t|________________________________________|\n"
        "\t|                                        |\n"
        "\t|        ! USE AT YOUR OWN RISK !        |\n"
        "\t|________________________________________|\n"
    );
    char* username = getenv("USER");
    printf("\n - CURRENT USER: @%s", username);
    printf("\n");
    sleep(3);
    clear();
}

char *read_cmd () {
    int buf_size = LINE_BUFF_SIZE, pos = 0;
    char *buffer = malloc(sizeof(char) * buf_size);

    if (!buffer) {
        fprintf(stderr, "Error: Allocation Error.\n");
        exit(EXIT_FAILURE);
    }

    int c;
    while(1) {
        c = getchar();
        if (c == EOF) {
            // 读到文件终止符
            exit(EXIT_SUCCESS);
        } else if (c == '\n') {
            // 读完一行，返回
            buffer[pos] = '\0';
            return buffer;
        } else {
            buffer[pos] = c;
        }
        pos++;
        // 超出缓冲区大小，重新分配
        if (pos == buf_size) {
            buf_size += LINE_BUFF_SIZE;
            buffer = realloc(buffer, buf_size);
            if (!buffer) {
                fprintf(stderr, "Error: Allocation Error.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **split_cmd (char *line) {
    char **args = malloc(sizeof(char*) * ARGS_BUFF_SIZE);
    if (!args) {
        fprintf(stderr, "Error: Allocation Error.\n");
        exit(EXIT_FAILURE);
    };
    char **args_backup;
    int buf_size = ARGS_BUFF_SIZE;
    argc = 0;
    
    char* arg = strtok(line, TOK_MARK);
    while (arg) {
        args[argc++] = arg;

        if (argc == buf_size) {
            buf_size += ARGS_BUFF_SIZE;
            args_backup = args;
            args = realloc(args, sizeof(char*) * buf_size);
            if (!args) {
                SAFE_DELETE(args_backup);
                fprintf(stderr, "Error: Allocation Error.\n");
                exit(EXIT_FAILURE);
            }
        }

        arg = strtok(NULL, TOK_MARK);
    }
    args[argc] = NULL;
    return args;
}


int execute_cmd (char **args) {
    if (args[0] == NULL) return 0;     // 输入命令长度为0

    // 与内置命令逐个进行字符串匹配
    for (int i = 0; i < num_builtins(); i++) {
        if (strcmp(args[0], builtin_strs[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    // 未在内置命令中找到输入命令，输出错误信息
    fprintf(stderr, "Error: Command Not Exist.\n");
    return 0;
}

int main() {
    init_shell();

    char* cmd;
    char** args;
    int status;

    do {
        printf("[404Shell]# ");
        cmd = read_cmd();
        args = split_cmd(cmd);
        status = execute_cmd(args);
        SAFE_DELETE(cmd);
        SAFE_DELETE(args);
    } while (!status);
}
