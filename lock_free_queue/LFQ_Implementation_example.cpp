#pragma once
#include "../header/bench.h"
#include "../header/Thread.h"
#include "../header/lock_free_queue.h"  
#include <iostream>
#include <chrono>
#include <atomic>
#include <vector>
#include <algorithm>
#include <thread>  

using namespace Common;

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0) 


struct mystruct{
    uint64_t val;
    uint64_t val1;
    uint64_t val2;
};    


auto inline  producer(LFQueue<mystruct> &lfq, std::vector<std::chrono::duration<double,std::nano>>& producer_time) {
    
    // this is literals in cpp which help us to write time durations in a more readable way, like 1s for 1 second, 500ms for 500 milliseconds etc.
    using namespace std::literals::chrono_literals;
    // std::this_thread::sleep_for(1s);  // 1std::chrono::seconds(1) is the same as 1s, it makes the code more readable and easier to understand that we are sleeping for 1 second.
    uint64_t cnt = {0};
    while (cnt < 1000000) {
        auto start = std::chrono::high_resolution_clock::now();
        const mystruct d{cnt, cnt + 1, cnt + 2};
       
        auto write_ptr = lfq.getNextToWriteTo();
        
        while (write_ptr == nullptr) {

            std::this_thread::yield();
            write_ptr = lfq.getNextToWriteTo();
        
        }

        *write_ptr = d;
        lfq.updateWriteIndex();
        cnt++;

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::nano> interval = end - start;
        producer_time.push_back(interval);
    
        std::cout << "Producer constructed elem: " << d.val << ", " << d.val1 << ", " << d.val2 << " lfq-size: " << lfq.size() << std::endl;

        // if (UNLIKELY(!flag.load())) {
        //     flag.store(true);
        // }
    }

    std::cout << "---------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "---------------------------------- producer done producing ----------------------------------" << std::endl;
    std::cout << "---------------------------------------------------------------------------------------------" << std::endl;    

} 

auto inline consumer(LFQueue<mystruct> &lfq, std::vector<std::chrono::duration<double,std::nano>>& consumer_time) {
     
    // while (!flag.load());
    // using namespace std::literals::chrono_literals;
    // std::this_thread::sleep_for(0s);
    uint64_t cnt = {0}; 
    while (cnt < 1000000) {

        auto st = std::chrono::high_resolution_clock::now();
        auto read = lfq.getNextToRead();

        while (read == nullptr) {
            std::this_thread::yield();
            read = lfq.getNextToRead();
        }  

        lfq.updateReadIndex();
        cnt++;

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::nano> interval = end - st;
        consumer_time.push_back(interval);

        std::cout << "Consumed: " << read->val << ", " << read->val1 << ", " << read->val2 << " size: " << lfq.size() << std::endl;
    }

    std::cout << "Consumer is done consuming" << std::endl;
}

int main() {    

    // buffer timing for producer and consumer and then show the benchmark results
    LFQueue<mystruct> lfq(5000000);
    // std::atomic<bool> flag(false);
    
    std::vector<std::chrono::duration<double,std::nano>> producer_time;
    std::vector<std::chrono::duration<double,std::nano>> consumer_time;

    auto pt = createAndStartThread(1, "producer thread", producer, lfq, producer_time);
    auto ct = createAndStartThread(2, "consumer thread", consumer, lfq, consumer_time); 


    

    pt->join();
    ct->join();
    std::cout<<"*************************************************main is done consuming ************************************************************" << std::endl;
    
    std::cout<<"*************************************************consumer Benchmark*****************************************************************" << std::endl;
    showBenchmark(producer_time, "Consumer");
    std::cout<<"*************************************************producer Benchmark*****************************************************************" << std::endl;
    showBenchmark(consumer_time, "Producer");

    return 0;
}