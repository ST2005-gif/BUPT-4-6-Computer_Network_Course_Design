#pragma once

#include "header.h"
#include "server.h"
#include "data_struct.h"

char* host_path;					// HOST�ļ�Ŀ¼
char* LOG_PATH;						// ��־�ļ�Ŀ¼

int debug_mode;
int log_mode;
int process_mode;
int intercept_mode;
void init(int argc, char* argv[]);// ��ʼ��
void get_config(int argc, char* argv[]);// ��ȡ����
void print_help_info();// ��ӡ������Ϣ
void read_host();// ��ȡdnsrelay�ļ�
void write_log(char* domain, uint8_t* ip_addr,int type,uint8_t addr[][4]);// д����־