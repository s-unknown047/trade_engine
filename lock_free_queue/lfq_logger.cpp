#pragma once
#include "../header/order_gateway_struct.h"
#include "../header/Thread.h"
#include "../header/lock_free_queue.h"
#include "../header/logger.h"
#include <thread>
#include <functional>
#define SIZE 10

inline uint64_t time() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock().now().time_since_epoch()).count();
}
void producer(Common::LFQueue<internal_lib::LogElement>* userOrder, std::vector<std::chrono::duration<double,std::nano>> produce_time) {
    std::atomic<int> cnt = {0};
    std::cout<< "in producer"<<std::endl;
    while (cnt < SIZE) {
        uint64_t t = time();
        auto arrival_time = std::chrono::high_resolution_clock::now();
        internal_lib::LogElement* writea = userOrder->getNextToWriteTo();

        while (writea == nullptr) {
            std::this_thread::yield();
            writea = userOrder->getNextToWriteTo();
        }
        writea->log_id = cnt;
        writea->log_data = internal_lib::UserOrder{};
        internal_lib::UserOrder* write = &std::get<internal_lib::UserOrder>(writea->log_data);
      
    
        write->arrived_cycle_count = t;
        write->order_id = cnt;
        write->trader_id = 0;
        write->order_type = 'b';
        write->req_type = 'r';
        write->price = cnt*100*0.1;
        write->quantity = 10;
        write->out_cycle_count = t;
        // std::cout << write->order_id << std::endl;
        userOrder->updateWriteIndex();
        auto finish_time = std::chrono::high_resolution_clock::now();

        produce_time.push_back(arrival_time-finish_time);
        
        cnt++;
    }    
}

void printVal(Common::LFQueue<internal_lib::LogElement>* userOrder) {
    for (size_t i = 0; i < userOrder->size(); ++i) {
        internal_lib::LogElement* element = userOrder->getNextToRead();
        if (element != nullptr) {
            std::cout << "Element " << i << ": " << element->log_id << std::endl;
        }
        userOrder->getNextToRead();
        }
    std::cout << "Queue contents printed" << std::endl;
}

int main() {
    size_t size = SIZE;
    Common::LFQueue<internal_lib::LogElement> Order(size*5);
        Common::LFQueue<internal_lib::LogElement>  match(size);

    std::vector<std::chrono::duration<double, std::nano>> producer_time(SIZE);
    std::vector<std::chrono::duration<double, std::nano>> consumer_time(SIZE);
    std::string path = "../file.log";

    internal_lib::Async_Logger logger(path, &match, &Order);
    
    auto t1 = createAndStartThread(0, "Logger Thread", [&logger] () { logger.run();});
    auto t2 = createAndStartThread(2 ,"this is Producer thread", producer, &Order, producer_time);
    
        t2->join();

    std::cout<<"Producer done Producing" << std::endl;
        // printVal(&Order);

        // std::this_thread::sleep_for(std::chrono::seconds(2));

    std::this_thread::sleep_for(std::chrono::seconds(2));

    logger.stop();

    t1->join();
    std::cout << "done logger" << std::endl;

}