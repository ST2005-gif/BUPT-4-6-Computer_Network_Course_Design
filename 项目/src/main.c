#include "system.h"
#include "server.h"
int main(int argc, char* argv[]) {
    init(argc, argv);//初始化
    if (mode == 1) //mode为初始化输入的参数
    {
        nonblock();//以非阻塞模式运行
    }
    else if (mode == 0) 
    {
        block();    //以阻塞模式运行
    }
    close_server(); //关闭服务器
    return 0;
}