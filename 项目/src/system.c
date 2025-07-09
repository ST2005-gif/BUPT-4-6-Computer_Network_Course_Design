#include "system.h"
#include <time.h>

char* host_path = "./dnsrelay.txt";
char* LOG_PATH;
int debug_mode = 0;
int log_mode = 0;
int process_mode = 0;
int intercept_mode = 0;
void init(int argc, char* argv[]) {
    mode = 1;           // 默认非阻塞模式
    is_listen = 0;      // 起始不监听
    get_config(argc, argv);    //获取一开始输入的运行参数
    init_socket();    //初始化socket
    init_ID_list();    //初始化一个结构体数组，用于向远程服务器发报是暂时保存原本的信息
    init_cache(); //初始化缓存对应的链表结构
    read_host();//将dnsrelay.txt中的域名和ip地址存入键树这一数据结构中
}

/* 读取程序命令参数 */
void get_config(int argc, char* argv[]) {
    int index;//参数索引
    argc--;//去掉第一个参数即程序名
    print_help_info();//打印帮助信息
    if (argc == 0)
    {
        printf("默认模式\n");
        return;
    }
    for (index = 1; index <= argc; index++) {
        if (strcmp(argv[index], "-p") == 0) {
            process_mode = 1;//开启简单调试模式，即打印每一步的提示
            printf("简单调试模式\n");
        }
        if (strcmp(argv[index], "-d") == 0) {
            debug_mode = 1;//开启调试模式，即打印每一步的各种信息
            printf("复杂调试模式\n");
        }
        if (strcmp(argv[index], "-l") == 0) {
            log_mode = 1;//开启日志模式，即打印客户端的每次查询操作
            printf("日志模式\n");
        }
        if (strcmp(argv[index], "-t") == 0) {
            intercept_mode = 1;//开启拦截IPv6模式，即拦截相同的IPv6地址的请求
            printf("拦截IPv6模式\n");
        }
        else if (strcmp(argv[index], "-s") == 0)         //设置远程DNS服务器
        {
            char* addr = malloc(16);
            memset(addr, 0, 16);
            index++;
            memcpy(addr, argv[index], strlen(argv[index]) + 1);
            remote_dns = addr;
        }  
        else if (strcmp(argv[index], "-i") == 0) //打印基本信息
        {
            printf("本地对照表路径: %s\n", host_path);//dnsrelay.txt的路径
            printf("远程DNS服务器地址: %s (默认地址: 10.3.9.6, BUPT DNS) \n", remote_dns);//远程DNS服务器地址
        }

        else if (strcmp(argv[index], "-m") == 0)        //设置模式
        {
            index++;
            if (strcmp(argv[index], "0") == 0) {
                mode = 0;
            }
            else if (strcmp(argv[index], "1") == 0) {
                mode = 1;
            }
        }
    }
    printf("运行模式: ");//阻塞模式还是非阻塞模式
    printf(mode == 1 ? "非阻塞模式\n" : "阻塞模式\n");
}

void print_help_info() {

    printf("-------------------------------------------------------------------------------\n");
    printf("|                    Welcome to use DNS relay server!                         |\n");
    printf("|    Please enter your query by cmd terminal, and receive the answer in it.   |\n");
    printf("|                  Example: nslookup www.baidu.com 127.0.0.1                  |\n");
    printf("|     Arguments: -i:                  print basic information                 |\n");
    printf("|                -p:                  print simple debug information          |\n");
    printf("|                -d:                  print debug information                 |\n");
    printf("|                -l:                  print log                               |\n");
    printf("|                -t:                  intercept IPv6                          |\n");
    printf("|                -s [server_address]: set remote DNS server                   |\n");
    printf("|                -m [mode]: set mode, 0: nonblock, 1: block                   |\n");
    printf("-------------------------------------------------------------------------------\n");
    printf("当前模式为\n");
}

void read_host() {
    FILE* host_ptr = fopen(host_path, "r");

    if (!host_ptr) {
        printf("Error! Can not open hosts file!\n");
        exit(1);
    }
    int num = 0;
    while (!feof(host_ptr)) {
        uint8_t this_ip[4];
        fscanf(host_ptr, "%s", IPAddr);
        fscanf(host_ptr, "%s", domain);
        num++;
        transfer_IP(this_ip, IPAddr);//把IPAddr转成数组
        add_node(list_trie, this_ip, domain);//增加节点
    }
    if (debug_mode == 1) {
        printf("%d domain name address info has been loaded.\n\n", num);
    }
}


void write_log(char* domain, uint8_t* ip_addr,int type, uint8_t addr[][4])
{
    FILE* fp = fopen("./log.txt", "a");
    if (fp == NULL)
    {
        if (debug_mode == 1) {
            printf("File open failed.\n");
        }
    }
    else
    {        
        if (debug_mode == 1) {
            printf("File open succeed.\n");
        }
        // 获取当前时间
        time_t currentTime = time(NULL);
        // 定义 struct tm 变量
        struct tm localTime;
        // 使用 localtime_s 获取本地时间
        if (localtime_s(&localTime, &currentTime) != 0) {
            fprintf(stderr, "获取本地时间失败。\n");
            fclose(fp);
            return;
        }
        // 格式化并打印时间
        char timeString[100];
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &localTime);
        fprintf(fp, "%s  ", timeString);
        fprintf(fp, "%s  ", domain);
        if (type == -1)
        {
            fprintf(fp, "Found in cache, IP address:%d.%d.%d.%d\n", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
        }
        else if (type == -2)
        {
            fprintf(fp, "Not found in cache, but in table, IP address:%d.%d.%d.%d\n", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
        }
        else if (type == -3)
        {
            fprintf(fp, "Not found in cache and table, searching from remote DNS server\n");
        }
        else if (type == -4)
        {
            fprintf(fp, "Failed because of too many requests simultaneously\n");
        }
        else
        {
            fprintf(fp, "Found in remote DNS server, IP address:");
            for (int i = 0; i < type; i++) {
                fprintf(fp, " %d.%d.%d.%d", addr[i][0], addr[i][1], addr[i][2], addr[i][3]);
            }
            fprintf(fp, "\n");
        }
        // 刷新缓冲区并关闭文件
        fflush(fp);
        fclose(fp);
    }
}