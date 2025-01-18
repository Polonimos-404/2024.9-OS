#include "builtins.h"


/* 命令行相关定义 */
#define LINE_BUFF_SIZE 10         // 命令行缓冲区大小
#define TOK_MARK " \t\r\n\a"      // 分隔符
#define ARGS_BUFF_SIZE 3          // 参数缓冲区大小


/**
 * @brief 初始化全局变量，展示命令行的欢迎界面
 */
void init_shell ();

/**
 * @brief 从stdin中读取一行命令
 * @return 返回命令行
 */
char *read_cmd ();

/**
 * @brief 将命令行拆解为参数
 * @param line 输入的命令行
 * @return 拆解得到的参数列表，以NULL表示结尾
 */
char **split_cmd (char *line);

/**
 * @brief 执行内置命令
 * @param args 参数列表，以NULL代表结尾
 * @return 1代表shell继续运行，0代表终止
 */
int execute_cmd (char **args);
