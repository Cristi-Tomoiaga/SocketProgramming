#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
    if(argc != 3){
        printf("Not enough parameters.\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    char* ipAddr = argv[1];
    unsigned short port;
    sscanf(argv[2], "%hu", &port);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    printf("Socket created.\n");
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ipAddr);
    int res = connect(sock, (struct sockaddr*) &serverAddr, sizeof(serverAddr));

    if(res < 0){
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    printf("Connected successfully to ip address %s and port %hu.\n", ipAddr, port);

    printf("Enter a string(maximum 255 characters): ");
    
    char buf[256];
    fgets(buf, 256, stdin);
    buf[strlen(buf) - 1] = '\0';
    unsigned int len = strlen(buf);
    
    len = htonl(len);
    res = send(sock, &len, sizeof(len), 0);

    if(res < 0){
        perror("send()");
        exit(EXIT_FAILURE);
    }

    len = ntohl(len);
    res = send(sock, buf, len + 1, 0);

    if(res < 0){
        perror("send()");
        exit(EXIT_FAILURE);
    }

    printf("Sent the string to the server.\n");
    close(sock);

    return 0;
}
