#include "server.h"

void initServer(int argc, char* argv[]){
    unsigned short int port;
    char* ipAddr = argv[1];
    sscanf(argv[2], "%hu", &port);    

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipAddr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock == -1){
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    printf("Socket created.\n");

    int res = bind(sock, (struct sockaddr*) &addr, sizeof(addr));

    if(res == -1){
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    printf("Successfully bound to address %s and port %hu.\n", ipAddr, port);

    res = listen(sock, BACKLOG);

    if(res == -1){
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    printf("Began listening on port %hu.\n", port);

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientSock;
    char* clientIpAddr;

    std::atomic_bool stop(false);
    caller delayCaller(stop);
    std::thread delayThread(delayCaller);

    while(1){
        clientSock = accept(sock, (struct sockaddr*) &clientAddr, &clientAddrLen);            
        if(clientSock == -1){
            perror("accept()");
            continue;
        }

        updateUserConnectionLog(clientAddr.sin_addr.s_addr); 

        clientIpAddr = inet_ntoa(clientAddr.sin_addr);

        printMutex.lock();
        printf("The client with ip %s has connected.\n", clientIpAddr);
        printMutex.unlock();

        int pid = fork();

        if(pid == -1){
            perror("fork()");
            continue;
        }

        if(pid == 0){
            serveClient(clientSock);
        }
    }    
}

void serveClient(int clientSock){
    unsigned int len;
    int res = recv(clientSock, &len, sizeof(len), MSG_WAITALL);

    if(res == -1){
        perror("recv()");

        close(clientSock);
        exit(EXIT_FAILURE);
    }

    len = ntohl(len);

    // The problem was here
    char buf[256];
    res = recv(clientSock, buf, len + 1, MSG_WAITALL);

    if(res == -1){
        perror("recv()");

        close(clientSock);
        exit(EXIT_FAILURE);
    }

    printMutex.lock();
    printf("Client said: %s\n", buf);
    printMutex.unlock();

    close(clientSock);
    exit(EXIT_SUCCESS);
}
