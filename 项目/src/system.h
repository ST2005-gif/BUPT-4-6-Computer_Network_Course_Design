#pragma once

#include "header.h"
#include "server.h"
#include "data_struct.h"

char* host_path;					// HOST文件目录
char* LOG_PATH;						// 日志文件目录

int debug_mode;
int log_mode;
int process_mode;
int intercept_mode;
void init(int argc, char* argv[]);// 初始化
void get_config(int argc, char* argv[]);// 获取配置
void print_help_info();// 打印帮助信息
void read_host();// 读取dnsrelay文件
void write_log(char* domain, uint8_t* ip_addr,int type,uint8_t addr[][4]);// 写入日志