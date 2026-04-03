#ifndef __UART_H__
#define __UART_H__
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

#endif