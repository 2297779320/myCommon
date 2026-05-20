# myCommon + SampleApp Combined Makefile
CC = gcc
CFLAGS = -Wall -g -O0 -D_LINUX
LDFLAGS = -lpthread -lsqlite3

# Include paths
INCLUDES = \
    -I./SampleApp/inc \
    -I./AppPublic/inc \
    -I./AppPublic/inc/StbpClient \
    -I./common \
    -I./common/cjson

# Source files
SAMPLE_SRCS = \
    SampleApp/src/MainApp/MainApp.c \
    SampleApp/src/Global/global.c \
    SampleApp/src/SysConfig/SysConfig.c \
    SampleApp/src/SysManager/SysManager.c \
    SampleApp/src/SensorHub/SensorHub.c \
    SampleApp/src/DeviceCtrl/DeviceCtrl.c \
    SampleApp/src/UserAgent/UserAgent.c

COMMON_SRCS = \
    common/osal.c \
    common/tsk.c \
    common/comm_que.c \
    common/common.c \
    common/ctos.c \
    common/JsonEx.c \
    common/cjson/cJSON.c \
    common/cjson/cjson_extension.c \
    common/xlist.c \
    common/msgqueue.c \
    common/module_manager.c \
    common/framework_v2.c

APP_PUBLIC_SRCS = \
    AppPublic/src/StbpClient/StbpClient.c \
    AppPublic/src/StbpClient/JsonMsgDispatch.c

SRCS = $(SAMPLE_SRCS) $(COMMON_SRCS) $(APP_PUBLIC_SRCS)
OBJS = $(SRCS:.c=.o)

# Default target
all: SampleApp

SampleApp: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "========================================="
	@echo "Build complete: SampleApp"
	@echo "========================================="

%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	find . -name "*.o" -delete
	rm -f SampleApp
	@echo "Clean complete."

.PHONY: all clean
