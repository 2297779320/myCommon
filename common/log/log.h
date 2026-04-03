#ifndef __LOG_H__
#define __LOG_H__

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