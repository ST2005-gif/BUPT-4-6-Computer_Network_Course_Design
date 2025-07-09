#include "server.h"
#include <process.h>
int client_port;
int addr_len = sizeof(struct sockaddr_in);//ip地址长度
char* remote_dns = "10.3.9.6";
int is_listen;
int message_type;
typedef struct {
    char* q_name;
    uint8_t ip_addr[4];
}memory;
memory prev[10];
int prev_index=0;
typedef struct {
    uint8_t buffer[BUFFER_SIZE];
    int msg_size;
    struct sockaddr_in client_addr;
    int addr_len;
} thread_param_t;
//
unsigned __stdcall handle_client(void* arg) {
    thread_param_t* param = (thread_param_t*)arg;
    dns_message msg;
    uint8_t buffer_new[BUFFER_SIZE];
    uint8_t ip_addr[4] = { 0 };
    int is_found = 0;

    uint8_t* start = param->buffer;
    get_message(&msg, param->buffer, start);
    if (debug_mode == 1 || process_mode == 1)
    {
        printf("接受到来自客户端%d的查询请求，查询内容为%s\n", client_sock, msg.questions->q_name);
    }
    is_found = query_cache(msg.questions->q_name, ip_addr);

    if (is_found == 0) {
        if (debug_mode == 1 || process_mode == 1)
        {
            printf("在cache中查询失败！尝试在本地对照表中查询……\n");
        }
        is_found = query_node(list_trie, msg.questions->q_name, ip_addr);
        if (is_found == 0) {
            if (debug_mode == 1 || process_mode == 1)
            {
                printf("在本地对照表中查询失败！尝试向远程服务器发送查询请求……\n");
            }
            uint16_t newID = set_ID(msg.header->id, param->client_addr);
            memcpy(param->buffer, &newID, sizeof(uint16_t));
            if (newID == ID_LIST_SIZE) // 若ID列表已满，则无法转发
            {
                if (debug_mode == 1 || process_mode == 1)
                {
                    printf("同时查询人数过多导致请求失败！请重试！\n");
                    message_type = -4;
                }
            }
            else
            {
                if (debug_mode == 1 || process_mode == 1)
                {
                    printf("向远程服务器发送查询请求成功！请等待远程服务器的回复！\n");
                    message_type = -3;
                }
                is_listen = 1; // 设置监听标志位为1，表示正在等待远程DNS服务器的响应
                sendto(server_sock, param->buffer, param->msg_size, 0, (struct sockaddr*)&server_addr, addr_len);
                // 将DNS查询报文转发给远程DNS服务器，参数1为socket描述符，参数2为要发送的数据，参数3为数据长度，
                //  参数4为标志位，参数5为接收端的地址信息，参数6为地址信息的大小
                //  返回值为实际发送的数据长度，若出错则返回-1
            }
            if (log_mode == 1)
            {
                write_log(msg.questions->q_name, ip_addr, message_type, NULL); // 打印日志
            }
            if (debug_mode == 1 || process_mode == 1)
            {
                printf("结束多线程\n");
            }
            free(param);
            _endthreadex(0);
            return 0;
        }
        else if (debug_mode == 1 || process_mode == 1)
        {
            printf("在本地对照表中查询成功！\n");
            message_type = -2;
        }
    }
    else if (debug_mode == 1 || process_mode == 1)
    {
        printf("在cache中查询成功！\n");
        message_type = -1;
    }
    if (intercept_mode == 1) {
        if (prev_index != 0) {
            for (int i = 0; i < prev_index; i++)
            {
                if (strcmp(prev[i].q_name, msg.questions->q_name) == 0 && memcmp(prev[i].ip_addr, ip_addr, 4) == 0)
                {
                    printf("拦截IPv6成功！\n");
                    prev_index = 0; // 清空prev数组
                    return; // 如果查询结果已经在缓存中，则直接返回
                }
            }
        }
        else
        {
            prev_index = 0; // 清空prev数组
                memcpy(prev[prev_index].ip_addr, ip_addr, sizeof(ip_addr));//将IP地址保存在prev[i]中
                free(prev[prev_index].q_name); //释放prev[i]中的q_name
                prev[prev_index].q_name = (char*)malloc(strlen(msg.questions->q_name) + 1);
                memcpy(prev[prev_index].q_name, msg.questions->q_name, strlen(msg.questions->q_name) + 1);
                prev_index=1;
        }
    }
    uint8_t* end = set_message(&msg, buffer_new, ip_addr);
    int len = end - buffer_new;
    sendto(client_sock, buffer_new, len, 0, (struct sockaddr*)&param->client_addr, param->addr_len);
    if (debug_mode == 1 || process_mode == 1)
    {
        printf("查询结果已发送回客户端，请查看！\n");
    }
    if (log_mode == 1)
    {
        write_log(msg.questions->q_name, ip_addr, message_type, NULL); // 打印日志
    }
    if (debug_mode == 1 || process_mode == 1)
    {
        printf("结束该线程\n");
    }
    free(param);
    _endthreadex(0);
    return 0;
}

void init_socket() {
    /* 初始化，否则无法运行socket */
    WORD wVersion = MAKEWORD(2, 2);//设置版本号
    WSADATA wsadata;//定义一个WSADATA结构体
    if (WSAStartup(wVersion, &wsadata) != 0) //若初始化失败，则返回错误码
    {
        return;
    }
    client_sock = socket(AF_INET, SOCK_DGRAM, 0);//创建一个UDP套接字
    //参数代表使用ipv4协议，使用UDP协议，协议为0表示使用默认协议
    //作用为创建一个套接字，返回一个套接字描述符
    server_sock = socket(AF_INET, SOCK_DGRAM, 0);//创建一个UDP套接字

    memset(&client_addr, 0, sizeof(client_addr));//将client_addr清零
    memset(&server_addr, 0, sizeof(server_addr));//将server_addr清零

    client_addr.sin_family = AF_INET;//设置地址族为ipv4
    client_addr.sin_addr.s_addr = INADDR_ANY;//设置地址为本地地址
    client_addr.sin_port = htons(PORT);//设置端口为53，53为DNS服务的默认端口

    server_addr.sin_family = AF_INET;//设置地址族为ipv4
    server_addr.sin_addr.s_addr = inet_addr(remote_dns);//设置地址为远程DNS服务器地址
    server_addr.sin_port = htons(PORT);//设置端口为53，53为DNS服务的默认端口
    const int REUSE = 1;//
    setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&REUSE, sizeof(REUSE));
    //允许在套接字处于 TIME_WAIT 状态时重新绑定到该套接字
    if (bind(client_sock, (SOCKADDR*)&client_addr, addr_len) < 0)
    //将client_sock与client_addr绑定，即让client_sock监听client_addr的端口
    {
        printf("ERROR: Could not bind: %s\n", strerror(errno));//如果绑定失败，输出错误信息
        exit(-1);
    }

    char* DNS_server = remote_dns;
    printf("\nDNS server: %s\n", DNS_server);//输出DNS服务器地址
    printf("Listening on port 53\n\n");
}

/* 非阻塞模式 */
void nonblock() {
    int server_result = ioctlsocket(server_sock, FIONBIO, &mode);// 设置非阻塞模式
    int client_result = ioctlsocket(client_sock, FIONBIO, &mode);// 设置非阻塞模式
    if (server_result != 0 || client_result != 0) {
        // 设置失败就返回
        printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
        closesocket(server_sock);
        closesocket(client_sock);
        return ;
    }

    while (1) {
        receive_client(); // 接收来自客户端的数据
        receive_server(); // 接收来自服务器的数据
    }
}

void block() {
    struct pollfd fds[2];//设置两个用于监视的结构体

    while (1)
    {
        fds[0].fd = client_sock;//设置第一个结构体的fd为客户端的socket
        fds[0].events = POLLIN;//设置第一个结构体的监视事件为POLLIN，即有数据可读
        fds[1].fd = server_sock;//设置第二个结构体的fd为服务器的socket
        fds[1].events = POLLIN;//设置第二个结构体的监视事件为POLLIN，即有数据可读
        int ret = WSAPoll(fds, 2, 1);//监视两个socket，超时时间为1ms，阻塞等待直到有数据可读或超时
        if (ret == SOCKET_ERROR)
        {
            printf("ERROR WSAPoll: %d.\n", WSAGetLastError());//如果发生错误，输出错误码
        }
        else if (ret > 0)
        {
            if (fds[0].revents & POLLIN)//如果客户端socket有数据可读
            {
                receive_client();//接收来自客户端的数据
            }
            if (fds[1].revents & POLLIN)//如果服务器socket有数据可读
            {
                receive_server();//接收来自服务器的数据
            }
        }
    }
}

void close_server() {
    closesocket(client_sock);
    closesocket(server_sock);
    WSACleanup();
}

void receive_client() {
    thread_param_t* param = (thread_param_t*)malloc(sizeof(thread_param_t));
    param->addr_len = sizeof(struct sockaddr_in);
    param->msg_size = recvfrom(client_sock, param->buffer, sizeof(param->buffer), 0, (struct sockaddr*)&param->client_addr, &param->addr_len);
    if (param->msg_size >= 0) {
        if (process_mode == 1 || debug_mode == 1)
        {
            printf("启动多线程\n");
        }
        _beginthreadex(NULL, 0, handle_client, param, 0, NULL);
    }
    else {
        free(param);
    }
}

void receive_server()//仅为DNS中继器调用，为从远程DNS服务器接收数据
{
    uint8_t buffer[BUFFER_SIZE];        // 接收的报文
    dns_message msg;
    int msg_size = -1;                  // 报文大小
    uint8_t addr[10][4] = { 0 };                //传回的ip地址结果
    int addr_index=0;
    /* 接受远程DNS服务器发来的DNS应答报文 */
    if (is_listen == 1) //若正在等待远程DNS服务器的响应
    {
        msg_size = recvfrom(server_sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, &addr_len);
        //说明：recvfrom函数用于接收数据，参数1为socket描述符，参数2为接收数据的缓冲区，
        // 参数3为缓冲区大小，参数4为标志位，参数5为发送端的地址信息，参数6为地址信息的大小
        // 返回值为实际接收到的数据长度，若出错则返回-1，接受到的数据放在buffer里
    }
    if (msg_size > 0 && is_listen == 1)//若成功接收报文且正在等待远程DNS服务器的响应
    {
        get_message(&msg, buffer, buffer);//解析远程DNS服务器发来的DNS报文，将其保存到msg结构体内
        if (process_mode == 1 || debug_mode == 1)
        {
            printf("服务器已接收远程服务器查询内容%s的结果！\n", msg.questions->q_name);
        }
        dns_rr* current = msg.answers;
        prev_index = 0;
        for (int i = 0; i < msg.header->anCount; i++)
        {
            if (current->type == 1) { // 检查是否为A记录 (type 1 表示A记录)
                update_cache(current->rd_data.a_record.IP_addr, msg.questions->q_name);
                memcpy(addr[addr_index], current->rd_data.a_record.IP_addr, sizeof(current->rd_data.a_record.IP_addr));
                 addr_index++;
                if (intercept_mode == 1)
                {
                    memcpy(prev[prev_index].ip_addr, current->rd_data.a_record.IP_addr, sizeof(current->rd_data.a_record.IP_addr));//将IP地址保存在prev[i]中
                    free(prev[prev_index].q_name); //释放prev[i]中的q_name
                prev[prev_index].q_name = (char*)malloc(strlen(msg.questions->q_name) + 1);
                memcpy(prev[prev_index].q_name, msg.questions->q_name, strlen(msg.questions->q_name) + 1);
                prev_index++;
                }
            }
            current = current->next;
        }
        /* ID转换 */
        uint16_t ID = msg.header->id;//获取报文结构体中的ID
        uint16_t old_ID = htons(ID_list[ID].client_ID);//获取ID列表中对应ID的客户端ID
        memcpy(buffer, &old_ID, sizeof(uint16_t));        //把待发回客户端的包ID改回原ID

        struct sockaddr_in ca = ID_list[ID].client_addr;//获取原ID的客户端地址
        ID_list[ID].expire_time = 0;//把id列表中对应ID的expire_time设置为0，表示该位置已空缺，可以分配新ID

        sendto(client_sock, buffer, msg_size, 0, (struct sockaddr*)&ca, addr_len);
        //将DNS应答报文发送给客户端，参数1为socket描述符，参数2为要发送的数据，参数3为数据长度，
        // 参数4为标志位，参数5为接收端的地址信息，参数6为地址信息的大小
        is_listen = 0;//设置监听标志位为0，表示已发送完DNS应答报文，不再等待远程DNS服务器的响应
        if (debug_mode == 1 || process_mode == 1) {
            printf("查询结果已发送回客户端，请查看！\n");
            message_type = 2;
        }
        if (log_mode == 1) {
            write_log(msg.questions->q_name, NULL,addr_index,addr);//打印日志
        }
    }
}
//分配新ID的逻辑：
//1、当服务器端没在本地和缓存中找到域名对应的IP地址时，会向远程DNS服务器发送DNS查询报文
//2、此时需要将该包的ID进行更改，操作如下：
//2.1、从i=0开始遍历ID列表，找到第一个expire_time为0的ID，将i赋值给新ID
//2.2、将原ID保存在list[i].id中，将客户端地址保存在list[i].client_addr中，设置当前时间为list[i].expire_time
//获得就ID的逻辑：
//3、当服务器端接收到远程DNS服务器的DNS应答报文时，会根据报文中的ID找到对应的客户端ID，操作如下：
//原id为list[报文id].id