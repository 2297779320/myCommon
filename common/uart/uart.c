#include "uart.h"
#include <termios.h> /* terminate interface */

#define DEFAULT_BAUDRATE    115200

static int GetLastError(void) { return errno; }

static char *GetLastErrorStr(void) { return strerror(errno); }

static INT32 uart_open(const char* file)
{
    INT32 fd = -1;
    LOG_INFO("open %s\n", file);
    fd = open(file, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        LOG_ERR("uart tty open failed.\n");
    }
    return fd;
}

static INT32 uart_read(uart_param_t* param, UINT8 *buff, UINT32 len)
{
    int nfds = 0;
    fd_set readset;
    struct timeval timeout;
    int ret = 0;

    FD_ZERO(&readset);
    FD_SET(param->fd, &readset);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    nfds = select(param->fd + 1, &readset, NULL, NULL, &timeout);
    if (nfds < 0)
    {
        if ((errno == EINTR) || (errno == EAGAIN))
        {
        goto exit;
        }

        syslog("Select Fail:[%d]%s", GetLastError(), GetLastErrorStr());
        return STATE_CODE_UNDEFINED_ERROR;
    }
    else if (nfds == 0)
    {
        goto exit;
    }
    else
    {
        OSAL_MutexLock(&param->tMutex);
        ret = read(param->fd, buff, len);
        OSAL_MutexUnlock(&param->tMutex);
        if (ret < 0) {
            return ret;
        }
        //LOG_INFO("UART READ %d\n", *buff);
    }
exit: 
    return ret;
}

static INT32 uart_write(UINT8 *buff, UINT32 len, void* arg)
{
    uart_param_t* param = (uart_param_t*)arg;
    int ret = 0;
    OSAL_MutexLock(&param->tMutex);
    ret = write(param->fd, buff, len);
    OSAL_MutexUnlock(&param->tMutex);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

static void *uart_recv_thread(void *args)
{
    uart_param_t* param = (uart_param_t*)args;

    LOG_INFO("uart_recv_thread start!\n");

    UINT8 data;
    while (!param->stop_thread) {

        INT32 err = uart_read(param, &data, 1);
        if(err <= 0) 
        {   
		    }
        else {
            param->read_cb(&data, 1, param->protocol_param);
        }

        usleep(1);
    }
    LOG_INFO("stopped uart recv, uart_recv_thread end\n");
    param->stop_thread = TRUE;

    return NULL;
}

static E_StateCode ConfigUart(int fd, int baudrate, int wordWidth, int stopBits,
                      ParityMode_e parity)
{
  E_StateCode eCode = STATE_CODE_NO_ERROR;
  unsigned int i;
  int speed_arr[] = {B576000, B500000, B460800, B230400, B115200,
                     B57600,  B38400,  B19200,  B9600,   B4800,
                     B2400,   B1800,   B1200,   B300};
  int name_arr[] = {576000, 500000, 460800, 230400, 115200, 57600, 38400,
                    19200,  9600,   4800,   2400,   1800,   1200,  300};
  struct termios options;

  if (tcgetattr(fd, &options) != 0)
  {
    return STATE_CODE_CONFIG_ERROR;
  }

  for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++)
  {
    if (baudrate == name_arr[i])
    {
      cfsetispeed(&options, speed_arr[i]);
      cfsetospeed(&options, speed_arr[i]);
    }
  }

  options.c_cflag |= CLOCAL;
  options.c_cflag |= CREAD;
  options.c_cflag &= ~CRTSCTS;
  options.c_cflag &= ~CSIZE;
  switch (wordWidth)
  {

  case 5:
    options.c_cflag |= CS5;
    break;

  case 6:
    options.c_cflag |= CS6;
    break;

  case 7:
    options.c_cflag |= CS7;
    break;

  case 8:
    options.c_cflag |= CS8;
    break;

  default:
    options.c_cflag |= CS8;
    break;
  }

  switch (parity)
  {

  case ParityMode_NONE:
    options.c_cflag &= ~PARENB;
    options.c_iflag &= ~INPCK;
    break;

  case ParityMode_ODD:
    options.c_cflag |= (PARODD | PARENB);
    options.c_iflag |= INPCK;
    break;

  case ParityMode_EVEN:
    options.c_cflag |= PARENB;
    options.c_cflag &= ~PARODD;
    options.c_iflag |= INPCK;

    break;
  case ParityMode_MARK:
    options.c_cflag |= (PARODD | PARENB | CMSPAR);
    options.c_iflag |= INPCK;

    break;

  case ParityMode_SPACE:
    options.c_cflag |= (PARENB | CMSPAR);
    options.c_cflag &= ~PARODD;
    options.c_iflag |= INPCK;

    break;

  default:
    options.c_cflag &= ~PARENB;
    options.c_iflag &= ~INPCK;
    break;
  }

  switch (stopBits)
  {

  case 1:
    options.c_cflag &= ~CSTOPB;
    break;

  case 2:
    options.c_cflag |= CSTOPB;
    break;

  default:
    options.c_cflag &= ~CSTOPB;
    break;
  }

  options.c_oflag &= ~OPOST;
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_iflag &= ~(ICRNL | IXON);
  // options.c_lflag &= ~(ISIG | ICANON);

  options.c_cc[VTIME] = 1; /* 等待超时:x*0.1s */
  options.c_cc[VMIN] = 1;  /* 读取字�?�的最少个数为1 */

  tcflush(fd, TCIFLUSH);

  if (tcsetattr(fd, TCSANOW, &options) != 0)
  {
    return STATE_CODE_CONFIG_ERROR;
  }
  return eCode;
}

E_StateCode init_uart(uart_param_t* param)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;
    if (!param || !param->read_cb)
    {
        return STATE_CODE_INVALID_PARAM;
    }

    if (param->port == UART_USB0)
    {
        param->fd = uart_open("/dev/ttyUSB0");
        if (param->fd != -1)
        {
            LOG_INFO("open ttyUSB0 success\n");
        }
        else
        {
            return STATE_CODE_OBJECT_BUSY;
        }

        if (fcntl(param->fd, F_SETFL, 0) < 0)
        {
            printf("fcntl failed!\n");
            close(param->fd);
            param->fd = -1;
            return STATE_CODE_OBJECT_BUSY;
        }

        eCode = ConfigUart(param->fd, DEFAULT_BAUDRATE, 8, 1, ParityMode_NONE);
        if (!STATE_OK(eCode))
        {
            LOG_WARN("uart_set_speed failed\n");
            close(param->fd);
            param->fd = -1;
            return STATE_CODE_CONFIG_ERROR;
        }
    }
    else
    {
        /* 非 UART_USB0 端口：必须由调用方预先设置 param->fd */
        if (param->fd <= 0)
        {
            LOG_ERR("init_uart: non-USB port requires valid fd\n");
            return STATE_CODE_INVALID_PARAM;
        }
    }
    OSAL_MutexInit(&param->tMutex);
    param->write_func = &uart_write;
    param->stop_thread = FALSE;
    if (pthread_create(&param->recv_thread_handle, NULL, uart_recv_thread, param) != 0) {
        LOG_ERR("uart pthread_create failed: %s\n", strerror(errno));
        OSAL_MutexDestroy(&param->tMutex);
        if (param->fd > 0) {
            close(param->fd);
            param->fd = -1;
        }
        return STATE_CODE_UNDEFINED_ERROR;
    }

    return eCode;
}

E_StateCode deinit_uart(uart_param_t* param)
{
    E_StateCode eCode = STATE_CODE_NO_ERROR;   
    param->stop_thread = TRUE;
    pthread_join(param->recv_thread_handle, NULL);
    OSAL_MutexDestroy(&param->tMutex);
    if (param->fd <= 0) {
        return STATE_CODE_OBJECT_NOT_EXIST;
    }
    if (close(param->fd)) {
        return STATE_CODE_OBJECT_BUSY;
    }
    return eCode;
}
