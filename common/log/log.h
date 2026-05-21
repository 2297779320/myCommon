/**
 * @file log.h
 * @brief 日志模块接口（叶子节点，无项目内依赖）
 *
 * @details
 * 提供分级日志输出功能（DEBUG/INFO/WARNING/ERROR），支持同时输出到控制台和文件。
 * 通过宏 syslog/syserr/syswarn/sysdebug 自动记录文件名、函数名和行号。
 * 支持注册调试跟踪回调函数（LJDebugtraceCb）用于外部日志转发。
 *
 * @note 线程安全: log_write 内部使用 log_mutex 保护。
 *
 * @see defs.h（通过 defs.h 被几乎所有模块间接依赖）
 */

#ifndef LOG_H
#define LOG_H

// 日志级别定义
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

#define OUTPUT_TO_CONSOLE   1
#define OUTPUT_TO_FILE      1

#define SHORT_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
                   (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

#define syslog(format, ...) do{ log_write(LOG_INFO, SHORT_FILE, __func__, __LINE__, format, ##__VA_ARGS__); } while(0)
#define syserr(format, ...) do{ log_write(LOG_ERROR, SHORT_FILE, __func__, __LINE__, format, ##__VA_ARGS__); } while(0)
#define syswarn(format, ...) do{ log_write(LOG_WARNING, SHORT_FILE, __func__, __LINE__, format, ##__VA_ARGS__); } while(0)
#define sysdebug(format, ...) do{ log_write(LOG_DEBUG, SHORT_FILE, __func__, __LINE__, format, ##__VA_ARGS__); } while(0)
#define LOG_DEBUG(format, ...) do{ log_write(LOG_DEBUG, SHORT_FILE, __func__, __LINE__, format, ##__VA_ARGS__); } while(0)
#define LOG_INFO(format, ...)  do{ log_write(LOG_INFO, SHORT_FILE, __func__, __LINE__, format, ##__VA_ARGS__); } while(0)
#define LOG_WARN(format, ...)  do{ log_write(LOG_WARNING, SHORT_FILE, __func__, __LINE__, format, ##__VA_ARGS__); } while(0)
#define LOG_ERR(format, ...) do{ log_write(LOG_ERROR, SHORT_FILE, __func__, __LINE__, format, ##__VA_ARGS__); } while(0)

void log_set_level(LogLevel level);
int log_init();
void log_write(LogLevel level, const char* file, const char* func, int line, const char *format, ...) __attribute__((format(printf, 5, 6)));


typedef void (*LJDebugtraceCb)(const char* message, int len);

int log_register_debug_trace(LJDebugtraceCb trace_log, void *arg);
void log_unregister_debug_trace(void);

#endif