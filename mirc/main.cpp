#include "server.h"

int main(int argc, char* argv[]){
    signal(SIGCHLD, SIG_IGN);

    if(argc != 3){
        printf("Not enough parameters.\n");
        exit(EXIT_FAILURE);
    }

    initServer(argc, argv);        

    return 0;
}
