#include"httpserver/httpd.h""
#include<sys/socket.h>
#include<iostream>
#include<pthread.h>
#include<unistd.h>
#include<netinet/in.h>
using namespace std;

void test(){
    Httpd Test;
    __int32_t serverSock = -1;
    uint16_t port = 22122;//这个只是举例
    __int32_t clientSock = -1;
    struct sockaddr_in  clientName;
    socklen_t clientNameLen = sizeof(clientName);
    pthread_t newthread;

    serverSock = Test.startUp(&port);
    cout << "httpd running on port " << port << endl;

    while(1){
        clientSock = accept(serverSock,(struct sockaddr*)&clientName, &clientNameLen);
        if(clientSock == -1)
            Test.errorInfo("accept");

        //创建线程 accept_request(&client_sock);
        if(pthread_create(&newthread, nullptr,acceptRequest, (void*)(intptr_t)clientSock) != 0)
            Test.errorInfo("pthread_create failure");               
    }
    close(serverSock);
}

int main(int argc, char* argv[]){
    test();
    return 0;
}