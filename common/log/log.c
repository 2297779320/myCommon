#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "log.h"
#include "debugtrace.h"

#define MAX_LOG_FILE_SIZE_2MB        2 * 1024 * 1024
#define MAX_LOG_FILE_TIME_7DAYS      7 * 24 * 3600

// 全局变量
static LogLevel current_level = LOG_INFO;
static FILE *log_file = NULL;
static char current_log_path[256] = {0};
static const char *log_dir = "/data/vendor/appautorun/";
static const char *log_prefix = "tsdecoder";
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;


static  LJDebugtraceCb tracecb = NULL;
void *traceCbarg = NULL;


// 检查并执行日志轮转
static void check_rotate_log() {
    if (!log_file) return;
    
    // 检查文件大小
    fflush(log_file);
    struct stat st;
    if (stat(current_log_path, &st) == 0 && st.st_size > MAX_LOG_FILE_SIZE_2MB) {
        // 关闭当前文件
        fclose(log_file);
        
        // 确保old目录存在
        char old_dir_path[512];
        snprintf(old_dir_path, sizeof(old_dir_path), "%s/old", log_dir);
        mkdir(old_dir_path, 0755); // 如果目录已存在，mkdir会失败，但这没关系
        
        // 生成带时间戳的新文件名（在old目录下）
        time_t now = time(NULL);
        struct tm tm_buf;
        struct tm *tm = localtime_r(&now, &tm_buf);
        char new_log_path[512];
        snprintf(new_log_path, sizeof(new_log_path), 
                "%s/old/%s_%04d%02d%02d_%02d%02d%02d_%03d.log", // 增加了毫秒精度
                log_dir, log_prefix,
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(now % 1000)); // 简易毫秒
        
        // 移动当前日志文件到old目录
        if (rename(current_log_path, new_log_path) != 0) {
            // 如果移动失败，尝试在原地重新打开日志文件
            perror("Failed to move log file to old directory");
            log_file = fopen(current_log_path, "a");
            if (!log_file) {
                perror("Also failed to reopen original log file");
            }
            return;
        }
        
        // 在原位置重新创建新的日志文件
        log_file = fopen(current_log_path, "a");
        if (!log_file) {
            perror("Failed to create new log file after rotation");
            // 在此可以尝试使用备用路径或采取其他恢复措施
        }
    }
}

// 清理旧日志文件
static void log_cleanup_old_files() {
    time_t threshold = time(NULL) - MAX_LOG_FILE_TIME_7DAYS;
    
    // 清理old目录下的旧文件
    char old_dir_path[512];
    snprintf(old_dir_path, sizeof(old_dir_path), "%s/old", log_dir);
    
    DIR *dir = opendir(old_dir_path);
    if (!dir) {
        if (errno != ENOENT) { // 如果old目录不存在，无需清理
            perror("Failed to open old log directory for cleanup");
        }
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过"."和".."条目
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 检查是否为日志文件且匹配前缀
        if (strstr(entry->d_name, ".log") != NULL && 
            strstr(entry->d_name, log_prefix) != NULL) {
            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s/%s", old_dir_path, entry->d_name);
            
            struct stat st;
            if (stat(filepath, &st) == 0) {
                // 同时检查修改时间和文件名中的时间戳（更可靠）
                time_t file_time = st.st_mtime;
                
                // 可选：从文件名中解析时间戳作为备用判断
                if (file_time < threshold) {
                    if (remove(filepath) != 0) {
                        fprintf(stderr, "Failed to remove old log file %s: %s\n", 
                                filepath, strerror(errno));
                    }
                }
            }
        }
    }
    
    closedir(dir);
}

// 设置日志级别
void log_set_level(LogLevel level) {
    pthread_mutex_lock(&log_mutex);
    current_level = level;
    pthread_mutex_unlock(&log_mutex);
}

// 初始化日志系统
int log_init() {
    
    log_cleanup_old_files();
    log_set_level(LOG_DEBUG);

    // 确保日志主目录存在
    if (mkdir(log_dir, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create log directory");
        return -1;
    }

    // 构建old目录路径
    char old_dir_path[512];
    snprintf(old_dir_path, sizeof(old_dir_path), "%s/old", log_dir);

     // 创建old目录（如果不存在）
    if (mkdir(old_dir_path, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create old log directory");
        return -1;
    }
    
    // 迁移log_dir下的现有日志文件到old目录
    DIR *dir_ptr = opendir(log_dir);
    if (!dir_ptr) {
        perror("Failed to open log directory for migration");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir_ptr)) != NULL) {
        // 跳过"."和".."条目，以及old目录本身
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, "old") == 0) {
            continue;
        }
        
        // 检查是否为与前缀匹配的日志文件
        if (strstr(entry->d_name, ".log") != NULL && 
            (log_prefix[0] == '\0' || strstr(entry->d_name, log_prefix) != NULL)) {
            char old_file_path[512];
            char new_file_path[512];
            
            snprintf(old_file_path, sizeof(old_file_path), "%s/%s", log_dir, entry->d_name);
            snprintf(new_file_path, sizeof(new_file_path), "%s/old/%s", log_dir, entry->d_name);
            
            // 移动文件
            if (rename(old_file_path, new_file_path) != 0) {
                fprintf(stderr, "Warning: Failed to move file %s to old directory: %s\n", 
                        entry->d_name, strerror(errno));
                // 可以选择继续迁移其他文件而非直接返回错误
            }
        }
    }
    closedir(dir_ptr);
    
    // 初始化当前日志路径并打开日志文件
    time_t now = time(NULL);
    struct tm tm_buf;
    struct tm *tm = localtime_r(&now, &tm_buf);
    snprintf(current_log_path, sizeof(current_log_path), 
            "%s/%s_%04d%02d%02d_%02d%02d%02d.log",
            log_dir, log_prefix,
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    log_file = fopen(current_log_path, "a");
    if (!log_file) {
        perror("Failed to open log file");
        return -1;
    }
    
    return 0;
}

// 写入日志
void log_write(LogLevel level, const char* file, const char* func, int line, const char *format, ...) {
    (void)file;
    (void)func;
    pthread_mutex_lock(&log_mutex);
    if (level < current_level) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    // 获取当前时间
    // time_t now = time(NULL);
    
    char timestamp[32];

    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm tm_buf;
    struct tm *tm = localtime_r(&tv.tv_sec, &tm_buf);

    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

    snprintf(timestamp + strlen(timestamp), sizeof(timestamp) - strlen(timestamp), ":%06ld", tv.tv_usec);
    // 日志级别字符串
    const char *level_str;
    switch (level) {
        case LOG_DEBUG: level_str = "DEBUG"; break;
        case LOG_INFO: level_str = "INFO"; break;
        case LOG_WARNING: level_str = "WARN"; break;
        case LOG_ERROR: level_str = "ERROR"; break;
        default: level_str = "UNKNOWN"; break;
    }
    
    // 格式化日志消息
    va_list args;
    va_start(args, format);
    char message[1024] = {0};
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
// 输出到控制台
#if OUTPUT_TO_CONSOLE
    // fprintf(stdout, "[%s][%s][%-16.16s][%05d] %s", timestamp, level_str, file, line, message);
    fprintf(stdout, "[%s][%s][%s][%05d] %s", timestamp, level_str, file, line, message);
    fflush(stdout);
#endif
    
// 输出到文件
#if OUTPUT_TO_FILE
    if (log_file) {
        fprintf(log_file, "[%s][%s][%-16.16s][%05d] %s", timestamp, level_str, file, line, message);
        fflush(log_file);
        check_rotate_log();
    }
#endif
    // 调用注册的调试跟踪处理函数
    if (tracecb) {
        char newmessage[1024] = {0};
        snprintf(newmessage, sizeof(newmessage), "[%s][%s][%s][%05d] %s", timestamp, level_str, file, line, message);
        tracecb(newmessage, strlen(newmessage));
    }
    pthread_mutex_unlock(&log_mutex);
}

int log_register_debug_trace(LJDebugtraceCb  trace_log, void *arg)
{
    if (trace_log) {
        pthread_mutex_lock(&log_mutex);
        tracecb = trace_log;
        traceCbarg = arg;
        pthread_mutex_unlock(&log_mutex);
        syslog("Debug trace handler registered\n");
        return 0;
    }
    return -1;
}

void log_unregister_debug_trace(void)
{
    pthread_mutex_lock(&log_mutex);
    tracecb = NULL;
    traceCbarg = NULL;
    pthread_mutex_unlock(&log_mutex);
}
