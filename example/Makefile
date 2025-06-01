# RTNLite Demo Makefile

# 项目路径定义
CURRENT_DIR := $(shell pwd)
SDK_DIR := $(CURRENT_DIR)/../rtnlite_sdk
UTILITY_DIR := 3rd/utility
OBJ_DIR := obj

# 编译器和参数
CC := clang
CFLAGS := -Wall -g -O2

# 包含路径
INCLUDE := -I$(SDK_DIR)/include \
           -I$(CURRENT_DIR) \
           -I$(CURRENT_DIR)/3rd/utility \
           -I$(CURRENT_DIR)/3rd/file_parser/include

# 链接库
LIBS := -L$(SDK_DIR)/lib -lrtnlite -Wl,-rpath,$(SDK_DIR)/lib -lpthread

# 源文件和目标文件定义
HELLO_SRC := hello_rtnlite.c
UTILITY_SRC := $(UTILITY_DIR)/pacer.c $(UTILITY_DIR)/utility.c
FP_SRC := $(wildcard 3rd/file_parser/src/*.c)

# 应用程序目标
TARGET := hello_rtnlite

all: $(TARGET)

# 创建目标文件夹
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 直接编译链接应用
$(TARGET): $(HELLO_SRC) $(UTILITY_SRC) $(FP_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)
	@if [ -d $(OBJ_DIR) ]; then rm -rf $(OBJ_DIR); fi

.PHONY: all clean
