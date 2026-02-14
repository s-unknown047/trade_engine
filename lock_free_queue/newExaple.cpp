#pragma once
#include "../header/Thread.h"
#include "../header/lock_free_queue.h"  
#include <iostream>
#include <chrono>
#include <atomic>
#include <vector>
#include <algorithm>

using namespace Common;


#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0) 

void showBenchmark(std::vector<std::chrono::duration<double,std::nano>>& producer_time, std::vector<std::chrono::duration<double,std::nano>>& consumer_time) {
    
    sort(producer_time.begin(), producer_time.end());
    sort(consumer_time.begin(), consumer_time.end());
    

    std::cout<< "Producer time median: " << producer_time[producer_time.size() / 2].count() << " ns" << std::endl;
    std::cout<< "Consumer time merdian: " << consumer_time[consumer_time.size() / 2].count() << " ns" << std::endl;
    std::cout<< "Producer time 0.75: " << producer_time[producer_time.size() * 0.75].count() << " ms" << std::endl;
    std::cout << "Consumer time 0.75: " << consumer_time[consumer_time.size() * 0.75].count() << " ms" << std::endl;
    std::cout << "Producer time 0.95: " << producer_time[producer_time.size() * 0.95].count() << " ms" << std::endl;
    std::cout << "Consumer time 0.95: " << consumer_time[consumer_time.size() * 0.95].count() << " ms" << std::endl;
    std::cout << "Producer time 0.99: " << producer_time[producer_time.size() * 0.99].count() << " ms" << std::endl;
    std::cout << "Consumer time 0.99: " << consumer_time[consumer_time.size() * 0.99].count() << " ms" << std::endl;

}

struct mystruct{
    int val;
    int val1;
    int val2;
};    




auto inline  producer(LFQueue<mystruct> &lfq, std::vector<std::chrono::duration<double,std::nano>>& producer_time) {
    
    
    for (int i = 0; i < 100000; ++i) {
      
        const mystruct d{i, i * 10, i * 100};

        auto start = std::chrono::high_resolution_clock::now();

        *(lfq.getNextToWriteTo()) = d;
        lfq.updateWriteIndex();

        std::cout << "Producer constructed elem: " << d.val << ", " << d.val1 << ", " << d.val2
                  << " lfq-size: " << lfq.size() << std::endl;

            


        // if (UNLIKELY(!flag.load())) {
        //     flag.store(true);
        // }

        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double,std::nano> interval = end - start;

        producer_time.push_back(interval);


    }

  

    std::cout << "************************************************Producer is done producing **********************************************************************************************" << std::endl;
    
} 

auto inline consumer(LFQueue<mystruct> &lfq, std::vector<std::chrono::duration<double,std::nano>>& consumer_time) {
     
    // while (!flag.load());

     while (lfq.size()) {
        auto st = std::chrono::high_resolution_clock::now();

         const auto d = lfq.getNextToRead();
         lfq.updateReadIndex();
         std::cout << "Consumed: " << d->val << ", " << d->val1 << ", " << d->val2
                   << " size: " << lfq.size() << "***********************************************************************" <<std::endl;

        auto en = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::nano> interval = en - st;
        consumer_time.push_back(interval);
     }

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Consumer is done consuming" << std::endl;
}

int main() {    

    // using mutex lock etc



    // buffer timing for producer and consumer and then show the benchmark results
    LFQueue<mystruct> lfq(5000000);
    // std::atomic<bool> flag(false);
    
    std::vector<std::chrono::duration<double,std::nano>> producer_time;
    std::vector<std::chrono::duration<double,std::nano>> consumer_time;
    auto pt = createAndStartThread(1, "producer thread", producer, lfq, producer_time);
    auto ct = createAndStartThread(2, "consumer thread", consumer, lfq, consumer_time); 

    showBenchmark(producer_time, consumer_time);


 
    pt->join();
    ct->join();


    return 0;
}