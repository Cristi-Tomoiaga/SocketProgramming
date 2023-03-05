#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "statistics.h"

std::unordered_map<uint32_t, std::list<suseconds_t>> logClients;
std::unordered_set<uint32_t> blackList;
std::mutex logMutex;
std::mutex printMutex;

void updateUserConnectionLog(uint32_t ipAddress){
    logMutex.lock();
    if(logClients.find(ipAddress) == logClients.end()){
        logClients[ipAddress] = std::list<suseconds_t>();
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);

    logClients[ipAddress].push_back(tv.tv_usec);
    logMutex.unlock();
}

void debugPrint(){
    struct in_addr ipAddr;
    
    printMutex.lock();
    printf("\n============================================================\n");
    
    for(auto item: logClients){
        ipAddr.s_addr = item.first;
        printf("Client %s:\n", inet_ntoa(ipAddr));
        for(auto timeStamp: item.second){
            printf("%ld ", timeStamp);
        }
        printf("\n");
    }
       
    printf("\n============================================================\n");
    printMutex.unlock();
}

void consistencyCheck(){
    logMutex.lock();

    double mean = 0.0;
    unsigned long sum = 0;
    unsigned long count = logClients.size();
    double deviation = 0.0;
    double sqDiffSum = 0.0;        

    for(auto item: logClients){
        sum += item.second.size();
    }
    mean = (double)sum/count;

    for(auto item: logClients){
        sqDiffSum += (item.second.size() - mean) * (item.second.size() - mean);
    }
    deviation = sqrt(sqDiffSum / count);

    printMutex.lock();
    printf("%lf %lf\n", mean, deviation);
    printMutex.unlock();

    logClients.clear();

    logMutex.unlock();
}
