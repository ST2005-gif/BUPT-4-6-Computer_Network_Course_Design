#include "system.h"
#include "server.h"
int main(int argc, char* argv[]) {
    init(argc, argv);//��ʼ��
    if (mode == 1) //modeΪ��ʼ������Ĳ���
    {
        nonblock();//�Է�����ģʽ����
    }
    else if (mode == 0) 
    {
        block();    //������ģʽ����
    }
    close_server(); //�رշ�����
    return 0;
}