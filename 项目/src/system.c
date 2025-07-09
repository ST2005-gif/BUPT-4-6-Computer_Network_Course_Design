#include "system.h"
#include <time.h>

char* host_path = "./dnsrelay.txt";
char* LOG_PATH;
int debug_mode = 0;
int log_mode = 0;
int process_mode = 0;
int intercept_mode = 0;
void init(int argc, char* argv[]) {
    mode = 1;           // Ĭ�Ϸ�����ģʽ
    is_listen = 0;      // ��ʼ������
    get_config(argc, argv);    //��ȡһ��ʼ��������в���
    init_socket();    //��ʼ��socket
    init_ID_list();    //��ʼ��һ���ṹ�����飬������Զ�̷�������������ʱ����ԭ������Ϣ
    init_cache(); //��ʼ�������Ӧ������ṹ
    read_host();//��dnsrelay.txt�е�������ip��ַ���������һ���ݽṹ��
}

/* ��ȡ����������� */
void get_config(int argc, char* argv[]) {
    int index;//��������
    argc--;//ȥ����һ��������������
    print_help_info();//��ӡ������Ϣ
    if (argc == 0)
    {
        printf("Ĭ��ģʽ\n");
        return;
    }
    for (index = 1; index <= argc; index++) {
        if (strcmp(argv[index], "-p") == 0) {
            process_mode = 1;//�����򵥵���ģʽ������ӡÿһ������ʾ
            printf("�򵥵���ģʽ\n");
        }
        if (strcmp(argv[index], "-d") == 0) {
            debug_mode = 1;//��������ģʽ������ӡÿһ���ĸ�����Ϣ
            printf("���ӵ���ģʽ\n");
        }
        if (strcmp(argv[index], "-l") == 0) {
            log_mode = 1;//������־ģʽ������ӡ�ͻ��˵�ÿ�β�ѯ����
            printf("��־ģʽ\n");
        }
        if (strcmp(argv[index], "-t") == 0) {
            intercept_mode = 1;//��������IPv6ģʽ����������ͬ��IPv6��ַ������
            printf("����IPv6ģʽ\n");
        }
        else if (strcmp(argv[index], "-s") == 0)         //����Զ��DNS������
        {
            char* addr = malloc(16);
            memset(addr, 0, 16);
            index++;
            memcpy(addr, argv[index], strlen(argv[index]) + 1);
            remote_dns = addr;
        }  
        else if (strcmp(argv[index], "-i") == 0) //��ӡ������Ϣ
        {
            printf("���ض��ձ�·��: %s\n", host_path);//dnsrelay.txt��·��
            printf("Զ��DNS��������ַ: %s (Ĭ�ϵ�ַ: 10.3.9.6, BUPT DNS) \n", remote_dns);//Զ��DNS��������ַ
        }

        else if (strcmp(argv[index], "-m") == 0)        //����ģʽ
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
    printf("����ģʽ: ");//����ģʽ���Ƿ�����ģʽ
    printf(mode == 1 ? "������ģʽ\n" : "����ģʽ\n");
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
    printf("��ǰģʽΪ\n");
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
        transfer_IP(this_ip, IPAddr);//��IPAddrת������
        add_node(list_trie, this_ip, domain);//���ӽڵ�
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
        // ��ȡ��ǰʱ��
        time_t currentTime = time(NULL);
        // ���� struct tm ����
        struct tm localTime;
        // ʹ�� localtime_s ��ȡ����ʱ��
        if (localtime_s(&localTime, &currentTime) != 0) {
            fprintf(stderr, "��ȡ����ʱ��ʧ�ܡ�\n");
            fclose(fp);
            return;
        }
        // ��ʽ������ӡʱ��
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
        // ˢ�»��������ر��ļ�
        fflush(fp);
        fclose(fp);
    }
}