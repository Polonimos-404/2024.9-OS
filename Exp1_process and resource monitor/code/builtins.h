#include "process_monitor.h"


/* 内置命令与函数 */
/* 函数声明 */

int create(char **args);
int destroy(char **args);
int activate(char **args);
int request(char **args);        
int release(char **args);
int request_io(char **args);
int io_completion(char **args);
int timeout(char **args);  
int list_p(char **args);
int list_r(char **args);
int help(char **args);
int exit_shell(char **args);
int clear_cmd(char **args);


/* 内置命令与函数对应关系 */
extern char *builtin_strs[];

/**
 * @brief 获得内置命令数量
 */
int num_builtins();

extern int (*builtin_func[]) (char **);

extern int argc;       // 命令行参数数量

/**
 * @brief 内置函数的参数量检查
 * @param num 某一内置函数应具有的参数量
 */
bool param_error(int num);
