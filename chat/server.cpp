#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <cstdint>
#include <thread>
#include <mutex>

// CONSTANTS
#define BACKLOG 10

// STATE
int numClients = 0;

// SOCKETS
int sServer;
int sReceiver;
int sBroadcast;
struct sockaddr_in sServerAddr;
struct sockaddr_in sBroadcastAddr;

// FD_SETS
fd_set fds;
fd_set readFds;
int fdmax = 0;

// MUTEX
std::mutex mutexNumClients;

// Main functions
struct sockaddr_in getSocketName(int sock) {
    struct sockaddr_in addr;
    socklen_t addrSize = sizeof(addr);
    memset(&addr, 0, sizeof(addr));

    int res = getpeername(sock, (struct sockaddr *) &addr, &addrSize);
    if(res == -1) {
        perror("getpeername()");
    }

    return addr;
}

char* getIPSocket(int sock) {
    struct sockaddr_in addr = getSocketName(sock);

    return inet_ntoa(addr.sin_addr);
}

uint16_t getPortSocket(int sock) {
    struct sockaddr_in addr = getSocketName(sock);

    return ntohs(addr.sin_port);
}

void sendBroadcastMessage() {
    char msg[] = "[BROADCAST MESSAGE]\n";
    while(1) {
        sleep(50);
        int res = sendto(sBroadcast, msg, strlen(msg) + 1, 0, (struct sockaddr *) &sBroadcastAddr, sizeof(sBroadcastAddr));
        if(res == -1) {
            perror("sendto()");
            continue;
        }

        printf("Mesaj de broadcast trimis\n");
    }
}

void handleUDPConnection() {
    struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    socklen_t clientAddrSize = sizeof(clientAddr);
    
    char buf[256] = "";
    while(1) {
        buf[0] = '\0';
        memset(&clientAddr, 0, sizeof(clientAddr));
        clientAddrSize = sizeof(clientAddr);

        int res = recvfrom(sReceiver, buf, sizeof(buf), 0, (struct sockaddr *) &clientAddr, &clientAddrSize);
        if(res == -1) {
            perror("recvfrom()");
            continue;
        }

        printf("Cerere udp primita de la %s:%hu\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        if(strcmp(buf, "get") == 0) {
            mutexNumClients.lock();
            int numClientsCopy = htonl(numClients);
            mutexNumClients.unlock();

            res = sendto(sReceiver, &numClientsCopy, sizeof(numClientsCopy), 0, (struct sockaddr *) &clientAddr, clientAddrSize);
            
            if(res == -1) {
                perror("sendto()");
            }
        }
    }
}

void sendMessageToAll(char* msg, int len, int crt) {
    for(int fd = 0; fd <= fdmax; fd++) {
        if(FD_ISSET(fd, &fds)) {
            if(fd != sServer && fd != crt) {
                int res = send(fd, msg, len, 0);
                if(res == -1) {
                    perror("send()");
                }
            }
        }
    }
}

void handleTCPConnections() {
    char buf[256] = "";
    char msg[276] = "";
    int res;
    struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    socklen_t clientAddrSize = sizeof(clientAddr);

    while(1) {
        readFds = fds;
        res = select(fdmax + 1, &readFds, NULL, NULL, NULL);
        if(res == -1) {
            perror("select()");
            continue;
        }

        for(int fd = 0; fd <= fdmax; fd++) {
            if(FD_ISSET(fd, &readFds)) {
                if(fd == sServer) {
                    memset(&clientAddr, 0, sizeof(clientAddr));
                    clientAddrSize = sizeof(clientAddr);

                    int clientfd = accept(sServer, (struct sockaddr *) &clientAddr, &clientAddrSize);
                    if(clientfd == -1) {
                        perror("accept()");
                        continue;
                    } 

                    FD_SET(clientfd, &fds);
                    if(clientfd > fdmax) {
                        fdmax = clientfd;
                    }
                    mutexNumClients.lock();
                    numClients++;       
                    mutexNumClients.unlock();

                    printf("Conexiune noua de la %s:%hu\n", getIPSocket(clientfd), getPortSocket(clientfd));
                } else {
                    buf[0] = '\0';
                    int res = recv(fd, buf, sizeof(buf), 0);
                    
                    if(res <= 0) {
                        if(res == 0) {
                            printf("Clientul %s:%hu s-a deconectat\n", getIPSocket(fd), getPortSocket(fd));
                        } else {
                            printf("Clientul s-a deconectat\n");
                            perror("recv()");
                        }
                    
                        FD_CLR(fd, &fds);

                        mutexNumClients.lock();
                        numClients--;
                        mutexNumClients.unlock();

                        close(fd);
                    } else {
                        msg[0] = '\0';
                        int len = sprintf(msg, "[%s:%hu] %s\n", getIPSocket(fd), getPortSocket(fd), buf);
                        printf("%s", msg);
                        if(len == -1) {
                            perror("sprintf()");
                            continue;
                        }
                        
                        sendMessageToAll(msg, len + 1, fd);
                    }
                }  
            }
        }
    }
}

// Setup functions
void setupSockets(char* ipStr, uint16_t port) {
    sServer = socket(AF_INET, SOCK_STREAM, 0);
    if(sServer == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    int yes = 1;
    int res = setsockopt(sServer, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if(res == -1) {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    printf("Socket tcp server creat cu succes\n");
    
    sReceiver = socket(AF_INET, SOCK_DGRAM, 0);
    if(sReceiver == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    res = setsockopt(sReceiver, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if(res == -1) {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    printf("Socket udp receptor creat cu succes\n");

    memset(&sServerAddr, 0, sizeof(sServerAddr));
    sServerAddr.sin_family = AF_INET;
    sServerAddr.sin_port = htons(port);
    sServerAddr.sin_addr.s_addr = inet_addr(ipStr);

    res = bind(sServer, (struct sockaddr *) &sServerAddr, sizeof(sServerAddr));
    if(res == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    printf("Bind realizat cu succes pe socket tcp server\n");

    res = bind(sReceiver, (struct sockaddr *) &sServerAddr, sizeof(sServerAddr));
    if(res == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    printf("Bind realizat cu succes pe socket udp receptor\n");

    res = listen(sServer, BACKLOG);
    if(res == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }
    printf("Listen realizat cu succes\n");

    FD_ZERO(&fds);
    FD_ZERO(&readFds);
    FD_SET(sServer, &fds);
    fdmax = sServer;
}

void setupBroadcast(char* ipBcastStr, uint16_t port) {
    sBroadcast = socket(AF_INET, SOCK_DGRAM, 0);
    if(sBroadcast == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    
    int yes = 1;
    int res = setsockopt(sBroadcast, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
    if(res == -1) {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }    

    printf("Socket udp broadcast creat cu succes\n");

    memset(&sBroadcastAddr, 0, sizeof(sBroadcastAddr));
    sBroadcastAddr.sin_family = AF_INET;
    sBroadcastAddr.sin_port = htons(port);
    sBroadcastAddr.sin_addr.s_addr = inet_addr(ipBcastStr);
}

int main(int argc, char* argv[]) {
    if(argc != 5) {
        printf("Utilizare: ./server <ip> <ip bcast> <port> <port bcast>\n");
        exit(EXIT_FAILURE);
    }

    uint16_t port;
    uint16_t portBcast;
    sscanf(argv[3], "%hu", &port);
    sscanf(argv[4], "%hu", &portBcast);
    char* ipStr = argv[1];
    char* ipBcastStr = argv[2];
    
    setupSockets(ipStr, port);
    setupBroadcast(ipBcastStr, portBcast);

    std::thread tcpThread(handleTCPConnections);
    std::thread broadcastThread(sendBroadcastMessage);
    std::thread udpThread(handleUDPConnection);
    tcpThread.join();
    broadcastThread.join();
    udpThread.join();
    
    exit(EXIT_SUCCESS);
}
