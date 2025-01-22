/* 资源相关定义 */
/* 资源种类数 */
#define RSC_TYPES 4

/* 资源种类表 */
typedef enum resources {
    ALPHA, BETA, GAMMA, DELTA   // I_O资源单独考虑不在此列
} rsc;
extern const char *resource_names[];

extern int rsc_ps_pids[RSC_TYPES];  // 占用资源的进程pid
