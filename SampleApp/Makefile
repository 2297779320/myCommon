# SampleApp Makefile
# Minimal build system demonstrating media-like build pattern

APP_NAME = SampleApp
CC = gcc
CFLAGS = -Wall -g -O0
LDFLAGS =

# Include paths
INCLUDES = \
    -I./inc \
    -I../AppPublic/inc \
    -I../AppPublic/inc/StbpClient \
    -I../AppPublic/inc/jsonEx \
    -I../AppPublic/inc/listOpt \
    -I../media/inc

# Source files
SRCS = \
    src/MainApp/MainApp.c \
    src/Global/global.c \
    src/SysConfig/SysConfig.c \
    src/SysManager/SysManager.c \
    src/SensorHub/SensorHub.c \
    src/DeviceCtrl/DeviceCtrl.c \
    src/UserAgent/UserAgent.c \
    src/UserAgent/UserAgentMsg.c

OBJS = $(SRCS:.c=.o)

# Library paths
LIB_DIRS = \
    -L../AppPublic/lib \
    -L../media/lib

# Libraries (link against shared infrastructure)
LIBS = \
    -lstbpc \
    -ljsonEx \
    -llistOpt \
    -lsqlite3

# Default target
all: $(APP_NAME)

$(APP_NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIB_DIRS) $(LIBS)
	@echo "Build complete: $(APP_NAME)"

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(APP_NAME)
	@echo "Clean complete."

.PHONY: all clean
