#include "server.h"
#include <process.h>
int client_port;
int addr_len = sizeof(struct sockaddr_in);//ip��ַ����
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
        printf("���ܵ����Կͻ���%d�Ĳ�ѯ���󣬲�ѯ����Ϊ%s\n", client_sock, msg.questions->q_name);
    }
    is_found = query_cache(msg.questions->q_name, ip_addr);

    if (is_found == 0) {
        if (debug_mode == 1 || process_mode == 1)
        {
            printf("��cache�в�ѯʧ�ܣ������ڱ��ض��ձ��в�ѯ����\n");
        }
        is_found = query_node(list_trie, msg.questions->q_name, ip_addr);
        if (is_found == 0) {
            if (debug_mode == 1 || process_mode == 1)
            {
                printf("�ڱ��ض��ձ��в�ѯʧ�ܣ�������Զ�̷��������Ͳ�ѯ���󡭡�\n");
            }
            uint16_t newID = set_ID(msg.header->id, param->client_addr);
            memcpy(param->buffer, &newID, sizeof(uint16_t));
            if (newID == ID_LIST_SIZE) // ��ID�б����������޷�ת��
            {
                if (debug_mode == 1 || process_mode == 1)
                {
                    printf("ͬʱ��ѯ�������ർ������ʧ�ܣ������ԣ�\n");
                    message_type = -4;
                }
            }
            else
            {
                if (debug_mode == 1 || process_mode == 1)
                {
                    printf("��Զ�̷��������Ͳ�ѯ����ɹ�����ȴ�Զ�̷������Ļظ���\n");
                    message_type = -3;
                }
                is_listen = 1; // ���ü�����־λΪ1����ʾ���ڵȴ�Զ��DNS����������Ӧ
                sendto(server_sock, param->buffer, param->msg_size, 0, (struct sockaddr*)&server_addr, addr_len);
                // ��DNS��ѯ����ת����Զ��DNS������������1Ϊsocket������������2ΪҪ���͵����ݣ�����3Ϊ���ݳ��ȣ�
                //  ����4Ϊ��־λ������5Ϊ���ն˵ĵ�ַ��Ϣ������6Ϊ��ַ��Ϣ�Ĵ�С
                //  ����ֵΪʵ�ʷ��͵����ݳ��ȣ��������򷵻�-1
            }
            if (log_mode == 1)
            {
                write_log(msg.questions->q_name, ip_addr, message_type, NULL); // ��ӡ��־
            }
            if (debug_mode == 1 || process_mode == 1)
            {
                printf("�������߳�\n");
            }
            free(param);
            _endthreadex(0);
            return 0;
        }
        else if (debug_mode == 1 || process_mode == 1)
        {
            printf("�ڱ��ض��ձ��в�ѯ�ɹ���\n");
            message_type = -2;
        }
    }
    else if (debug_mode == 1 || process_mode == 1)
    {
        printf("��cache�в�ѯ�ɹ���\n");
        message_type = -1;
    }
    if (intercept_mode == 1) {
        if (prev_index != 0) {
            for (int i = 0; i < prev_index; i++)
            {
                if (strcmp(prev[i].q_name, msg.questions->q_name) == 0 && memcmp(prev[i].ip_addr, ip_addr, 4) == 0)
                {
                    printf("����IPv6�ɹ���\n");
                    prev_index = 0; // ���prev����
                    return; // �����ѯ����Ѿ��ڻ����У���ֱ�ӷ���
                }
            }
        }
        else
        {
            prev_index = 0; // ���prev����
                memcpy(prev[prev_index].ip_addr, ip_addr, sizeof(ip_addr));//��IP��ַ������prev[i]��
                free(prev[prev_index].q_name); //�ͷ�prev[i]�е�q_name
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
        printf("��ѯ����ѷ��ͻؿͻ��ˣ���鿴��\n");
    }
    if (log_mode == 1)
    {
        write_log(msg.questions->q_name, ip_addr, message_type, NULL); // ��ӡ��־
    }
    if (debug_mode == 1 || process_mode == 1)
    {
        printf("�������߳�\n");
    }
    free(param);
    _endthreadex(0);
    return 0;
}

void init_socket() {
    /* ��ʼ���������޷�����socket */
    WORD wVersion = MAKEWORD(2, 2);//���ð汾��
    WSADATA wsadata;//����һ��WSADATA�ṹ��
    if (WSAStartup(wVersion, &wsadata) != 0) //����ʼ��ʧ�ܣ��򷵻ش�����
    {
        return;
    }
    client_sock = socket(AF_INET, SOCK_DGRAM, 0);//����һ��UDP�׽���
    //��������ʹ��ipv4Э�飬ʹ��UDPЭ�飬Э��Ϊ0��ʾʹ��Ĭ��Э��
    //����Ϊ����һ���׽��֣�����һ���׽���������
    server_sock = socket(AF_INET, SOCK_DGRAM, 0);//����һ��UDP�׽���

    memset(&client_addr, 0, sizeof(client_addr));//��client_addr����
    memset(&server_addr, 0, sizeof(server_addr));//��server_addr����

    client_addr.sin_family = AF_INET;//���õ�ַ��Ϊipv4
    client_addr.sin_addr.s_addr = INADDR_ANY;//���õ�ַΪ���ص�ַ
    client_addr.sin_port = htons(PORT);//���ö˿�Ϊ53��53ΪDNS�����Ĭ�϶˿�

    server_addr.sin_family = AF_INET;//���õ�ַ��Ϊipv4
    server_addr.sin_addr.s_addr = inet_addr(remote_dns);//���õ�ַΪԶ��DNS��������ַ
    server_addr.sin_port = htons(PORT);//���ö˿�Ϊ53��53ΪDNS�����Ĭ�϶˿�
    const int REUSE = 1;//
    setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&REUSE, sizeof(REUSE));
    //�������׽��ִ��� TIME_WAIT ״̬ʱ���°󶨵����׽���
    if (bind(client_sock, (SOCKADDR*)&client_addr, addr_len) < 0)
    //��client_sock��client_addr�󶨣�����client_sock����client_addr�Ķ˿�
    {
        printf("ERROR: Could not bind: %s\n", strerror(errno));//�����ʧ�ܣ����������Ϣ
        exit(-1);
    }

    char* DNS_server = remote_dns;
    printf("\nDNS server: %s\n", DNS_server);//���DNS��������ַ
    printf("Listening on port 53\n\n");
}

/* ������ģʽ */
void nonblock() {
    int server_result = ioctlsocket(server_sock, FIONBIO, &mode);// ���÷�����ģʽ
    int client_result = ioctlsocket(client_sock, FIONBIO, &mode);// ���÷�����ģʽ
    if (server_result != 0 || client_result != 0) {
        // ����ʧ�ܾͷ���
        printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
        closesocket(server_sock);
        closesocket(client_sock);
        return ;
    }

    while (1) {
        receive_client(); // �������Կͻ��˵�����
        receive_server(); // �������Է�����������
    }
}

void block() {
    struct pollfd fds[2];//�����������ڼ��ӵĽṹ��

    while (1)
    {
        fds[0].fd = client_sock;//���õ�һ���ṹ���fdΪ�ͻ��˵�socket
        fds[0].events = POLLIN;//���õ�һ���ṹ��ļ����¼�ΪPOLLIN���������ݿɶ�
        fds[1].fd = server_sock;//���õڶ����ṹ���fdΪ��������socket
        fds[1].events = POLLIN;//���õڶ����ṹ��ļ����¼�ΪPOLLIN���������ݿɶ�
        int ret = WSAPoll(fds, 2, 1);//��������socket����ʱʱ��Ϊ1ms�������ȴ�ֱ�������ݿɶ���ʱ
        if (ret == SOCKET_ERROR)
        {
            printf("ERROR WSAPoll: %d.\n", WSAGetLastError());//��������������������
        }
        else if (ret > 0)
        {
            if (fds[0].revents & POLLIN)//����ͻ���socket�����ݿɶ�
            {
                receive_client();//�������Կͻ��˵�����
            }
            if (fds[1].revents & POLLIN)//���������socket�����ݿɶ�
            {
                receive_server();//�������Է�����������
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
            printf("�������߳�\n");
        }
        _beginthreadex(NULL, 0, handle_client, param, 0, NULL);
    }
    else {
        free(param);
    }
}

void receive_server()//��ΪDNS�м������ã�Ϊ��Զ��DNS��������������
{
    uint8_t buffer[BUFFER_SIZE];        // ���յı���
    dns_message msg;
    int msg_size = -1;                  // ���Ĵ�С
    uint8_t addr[10][4] = { 0 };                //���ص�ip��ַ���
    int addr_index=0;
    /* ����Զ��DNS������������DNSӦ���� */
    if (is_listen == 1) //�����ڵȴ�Զ��DNS����������Ӧ
    {
        msg_size = recvfrom(server_sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, &addr_len);
        //˵����recvfrom�������ڽ������ݣ�����1Ϊsocket������������2Ϊ�������ݵĻ�������
        // ����3Ϊ��������С������4Ϊ��־λ������5Ϊ���Ͷ˵ĵ�ַ��Ϣ������6Ϊ��ַ��Ϣ�Ĵ�С
        // ����ֵΪʵ�ʽ��յ������ݳ��ȣ��������򷵻�-1�����ܵ������ݷ���buffer��
    }
    if (msg_size > 0 && is_listen == 1)//���ɹ����ձ��������ڵȴ�Զ��DNS����������Ӧ
    {
        get_message(&msg, buffer, buffer);//����Զ��DNS������������DNS���ģ����䱣�浽msg�ṹ����
        if (process_mode == 1 || debug_mode == 1)
        {
            printf("�������ѽ���Զ�̷�������ѯ����%s�Ľ����\n", msg.questions->q_name);
        }
        dns_rr* current = msg.answers;
        prev_index = 0;
        for (int i = 0; i < msg.header->anCount; i++)
        {
            if (current->type == 1) { // ����Ƿ�ΪA��¼ (type 1 ��ʾA��¼)
                update_cache(current->rd_data.a_record.IP_addr, msg.questions->q_name);
                memcpy(addr[addr_index], current->rd_data.a_record.IP_addr, sizeof(current->rd_data.a_record.IP_addr));
                 addr_index++;
                if (intercept_mode == 1)
                {
                    memcpy(prev[prev_index].ip_addr, current->rd_data.a_record.IP_addr, sizeof(current->rd_data.a_record.IP_addr));//��IP��ַ������prev[i]��
                    free(prev[prev_index].q_name); //�ͷ�prev[i]�е�q_name
                prev[prev_index].q_name = (char*)malloc(strlen(msg.questions->q_name) + 1);
                memcpy(prev[prev_index].q_name, msg.questions->q_name, strlen(msg.questions->q_name) + 1);
                prev_index++;
                }
            }
            current = current->next;
        }
        /* IDת�� */
        uint16_t ID = msg.header->id;//��ȡ���Ľṹ���е�ID
        uint16_t old_ID = htons(ID_list[ID].client_ID);//��ȡID�б��ж�ӦID�Ŀͻ���ID
        memcpy(buffer, &old_ID, sizeof(uint16_t));        //�Ѵ����ؿͻ��˵İ�ID�Ļ�ԭID

        struct sockaddr_in ca = ID_list[ID].client_addr;//��ȡԭID�Ŀͻ��˵�ַ
        ID_list[ID].expire_time = 0;//��id�б��ж�ӦID��expire_time����Ϊ0����ʾ��λ���ѿ�ȱ�����Է�����ID

        sendto(client_sock, buffer, msg_size, 0, (struct sockaddr*)&ca, addr_len);
        //��DNSӦ���ķ��͸��ͻ��ˣ�����1Ϊsocket������������2ΪҪ���͵����ݣ�����3Ϊ���ݳ��ȣ�
        // ����4Ϊ��־λ������5Ϊ���ն˵ĵ�ַ��Ϣ������6Ϊ��ַ��Ϣ�Ĵ�С
        is_listen = 0;//���ü�����־λΪ0����ʾ�ѷ�����DNSӦ���ģ����ٵȴ�Զ��DNS����������Ӧ
        if (debug_mode == 1 || process_mode == 1) {
            printf("��ѯ����ѷ��ͻؿͻ��ˣ���鿴��\n");
            message_type = 2;
        }
        if (log_mode == 1) {
            write_log(msg.questions->q_name, NULL,addr_index,addr);//��ӡ��־
        }
    }
}
//������ID���߼���
//1������������û�ڱ��غͻ������ҵ�������Ӧ��IP��ַʱ������Զ��DNS����������DNS��ѯ����
//2����ʱ��Ҫ���ð���ID���и��ģ��������£�
//2.1����i=0��ʼ����ID�б��ҵ���һ��expire_timeΪ0��ID����i��ֵ����ID
//2.2����ԭID������list[i].id�У����ͻ��˵�ַ������list[i].client_addr�У����õ�ǰʱ��Ϊlist[i].expire_time
//��þ�ID���߼���
//3�����������˽��յ�Զ��DNS��������DNSӦ����ʱ������ݱ����е�ID�ҵ���Ӧ�Ŀͻ���ID���������£�
//ԭidΪlist[����id].id