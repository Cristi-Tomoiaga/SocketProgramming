#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdio.h>
#include <sys/time.h>
#include <math.h>

#include <unordered_map>
#include <unordered_set>
#include <list>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#define DELAY 2000

extern std::unordered_map<uint32_t, std::list<suseconds_t>> logClients;
extern std::unordered_set<uint32_t> blackList;
extern std::mutex logMutex;
extern std::mutex printMutex;

void consistencyCheck();
void updateUserConnectionLog(uint32_t ipAddress);

struct caller {
    std::atomic_bool& stop;

    caller(std::atomic_bool& stop_) : stop(stop_) {}
    caller(caller const& other) : stop(other.stop) {}
    caller(caller&& other) : stop(other.stop) {}

    void operator() () {
        auto t_start = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds delay(DELAY);
        while(!stop) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto t_now = std::chrono::high_resolution_clock::now();
            std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_start);
            if(delay <= elapsed) {
                t_start = t_now;
                consistencyCheck();
            }
        }
    }
};

#endif
