#ifndef ERROR_H
#define ERROR_H
#include <stdio.h>
/* ================== 配置区 ================== */
// 1: 开启串口打印报错 (需要外部已实现 printf 重定向)
// 0: 关闭打印，仅返回错误码
#define ENABLE_PRINT 0
// 1: 报错后卡死在死循环 (用于开发阶段排查问题)
// 0: 报错后不卡死，让程序继续运行并返回错误码 (用于发布版本)
#define CONFIG_HALT_ON_ERROR 0
/* ============================================ */
typedef enum ERRCODE {
    ERR_OK = 0,
    ERR_PTR_NULL,  // 指针为空
    ERR_MEM,       // 内存分配失败
    ERR_OPT,       // 操作错误
    ERR_EMPTY,     // 参数为空
    ERR_INDEX,     // 索引错误
    ERR_ARGS,      // 参数错误
    // 野指针错误
    ERR_WILD,
    ERR_FREE_REPEAT,  // 重复释放
    ERR_BOUNDARY,     // 内存越界

} ERRCODE;

#if ENABLE_PRINT
extern const char* const ErrStr[];
#endif

// 修改为返回 ERRCODE 类型
static inline ERRCODE my_handler_error(ERRCODE code, const char* file,
                                       const char* func, const int line,
                                       const char* msg) {
#if ENABLE_PRINT
    printf("%s:%d: [ERROR] %s | reason %s", file, line, func, ErrStr[code]);
    if (msg) {
        printf(" | msg %s", msg);
    }
    printf("\r\n");
#else
    // 消除未使用参数的编译器警告
    (void)file;
    (void)func;
    (void)line;
    (void)msg;
#endif
#if CONFIG_HALT_ON_ERROR
    // 开发调试模式下，如果出错直接死循环，方便挂载调试器查看 Call Stack
    while (1) {
    }
#endif
    // 返回错误码给调用者
    return code;
}

// 宏定义不再使用 do...while(0)，而是直接展开为函数调用，以便获取返回值
#define _ERR_MODE1(code) \
    my_handler_error(code, __FILE__, __func__, __LINE__, NULL)

#define _ERR_MODE2(code, msg) \
    my_handler_error(code, __FILE__, __func__, __LINE__, msg)

#define _GET_MODE(_1, _2, NAME, ...) NAME

#define ERROR(...) _GET_MODE(__VA_ARGS__, _ERR_MODE2, _ERR_MODE1)(__VA_ARGS__)

#endif  // ERROR_H