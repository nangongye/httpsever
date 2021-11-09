#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<time.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<string.h>

const int bufferSize = 1023;

//设置非阻塞connect
int setnonblocking(int fd){
    int oldOption = fcntl(fd, F_GETFL);//获取fd的标志比如close-on-exec标志，返回值是fd的标志
    int newOption = oldOption | O_NONBLOCK;//设置非阻塞标志
    fcntl(fd, F_SETFL, newOption);//返回文件描述符旧的状态标志，以便日后恢复该状态标志
    return oldOption;
}

/*超时连接函数，参数分别是服务器IP地址、端口号和超时时间（毫秒）
    函数成功则返回已经处于连接状态的socket，失败则返回-1
*/
int unblock_connect(const char* ip, int port, int time){
    int ret = 0;
    struct sockaddr_in address;

    //等价于memset(&address, 0, sizeof(address);函数位于<string.h>
    bzero(&address, sizeof(address) );//将前sizeof(address)个字节数清0，
    address.sin_family =AF_INET;//ipv4

    //将ip地址转换成address.sin_addr用ipv4地址协议簇
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);//将端口转换成tcp/ip网络字节序

    //使用TCP协议，成功返回socket文件描述符
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    int fdopt = setnonblocking(sockfd);
    ret = connect(sockfd, (struct sockaddr*)& address, sizeof(address));
    if( ret == 0){
        /*如果连接成功，则恢复sockfd的属性，并立即返回*/
        printf("connect with server immediately\n");
        fcntl(sockfd, F_SETFL, fdopt);
        return sockfd;
    }else if(errno != EINPROGRESS){
        /*如果连接没有立即建立，那么只有当errno是EINPROGRESS时才表示连接还在进行，否则出错返回*/
        printf("unblock connect not support \n");
        return -1;
    }

    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;

    FD_ZERO(&readfds);//将文件描述符清空
    FD_SET(sockfd, &writefds); // 用于在文件描述符集合中增加一个新的文件描述符。

    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    //当套接字中有信号时，就会提示
    ret = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
    if(ret <= 0){
        /*select超时或者出错，立即返回*/
        printf("connection time out\n");
        close(sockfd);
        return -1;
    }

    //测试sockfd是否在读写文件描述符集合中
    if(!FD_ISSET(sockfd, &writefds)){
        printf("no events on sockfd found\n");
        close(sockfd);
        return -1;
    }

    int error = 0;
    socklen_t length = sizeof(error);
    /*调用getsockopt来获取并清除sockfd上的错误*/
    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0){
        printf("get socket option failed\n");
        close(sockfd);
        return -1;
    }

    /*错误号不为0表示连接出错*/
    if(error != 0){
        printf("connection failed after select with the error: %d \n",error);
        close(sockfd);
        return -1;
    }

    /*连接成功*/
    printf("connection ready after select with the socket : %d\n", sockfd);
    fcntl(sockfd, F_SETFL, fdopt);
    return sockfd;
}

int main(int argc, char* argv[]){
    if(argc <= 2){
        printf("usage: %s ip_address port_number \n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = unblock_connect(ip, port, 10);
    if(sockfd < 0)
        return 1;
    close(sockfd);
    return 0;
}