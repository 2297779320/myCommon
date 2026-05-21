/**
 * @file uart.h
 * @brief 串口通信模块 -- UART 端口初始化、读写回调
 *
 * @details
 * 提供串口（UART）的初始化和反初始化接口，支持异步读取回调。
 * 内部使用 pthread 线程进行数据接收。
 *
 * @see defs.h（依赖 E_StateCode, INT32, UINT8）
 * @see common.h（依赖 T_MutexObj）
 */

#ifndef UART_H
#define UART_H
#include "common/defs.h"
#include "common/common.h"

typedef enum {
    UART0,
    UART1,
    UART2,
    UART3,
    UART4,
    UART5,
    UART_USB0,
} uart_port;

typedef enum tagParityMode_e
{
    ParityMode_NONE,
    ParityMode_ODD,
    ParityMode_EVEN,
    ParityMode_MARK,
    ParityMode_SPACE,
    ParityMode_Butt,
} ParityMode_e;

typedef INT32(*uart_read_cb) (UINT8 * /*buff */, UINT32 /*buflen */, void * /*param */ );
typedef INT32(*uart_write_func) (UINT8 * /*buff */, UINT32 /*buflen */, void * /*param */ );
typedef struct
{
    uart_port port;
    INT32 fd;                
    pthread_t recv_thread_handle;
    BOOL stop_thread;
    uart_read_cb read_cb;
    void* protocol_param;
    uart_write_func write_func;
    T_MutexObj tMutex; 
} uart_param_t;

E_StateCode init_uart(uart_param_t* param);
E_StateCode deinit_uart(uart_param_t* param);

#endif // UART_H