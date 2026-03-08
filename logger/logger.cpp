#include "logger_imp.h"
#include "../header/Thread.h"
#include <chrono>
#include <vector>
#include <thread>
#include <iostream>     
#include <string>   
#define SIZE 100


int main() {
    Common::LFQueue<internal_lib::LogElement> order(SIZE);
    Common::LFQueue<internal_lib::LogElement> matching(SIZE);
    Common::LFQueue<internal_lib::LogElement> broadcast(SIZE);

    std::vector<internal_lib::LogElement> order_test;
    std::vector<internal_lib::LogElement> matching_test;
    std::vector<internal_lib::LogElement> broadcast_test;

    order_test.reserve(100);
    matching_test.reserve(100);
    broadcast_test.reserve(100);

     

    for (uint64_t i = 0; i < 100; i++) {

        uint64_t log_id_x = i;
        uint64_t log_id_y = i + 1;
        uint64_t log_id_z = i + 2;
        internal_lib::order x;
        internal_lib::match y;
        internal_lib::broadcast z; 

        x = {i};
        y = {i+i};
        z = {i + i + i};

        uint64_t time_x = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        uint64_t time_y =  std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        uint64_t time_z =  std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        
        order_test.push_back({log_id_x , time_x, x});
        matching_test.push_back({log_id_y, time_y, y});
        broadcast_test.push_back({log_id_z, time_z, z});

    }


    std::string fileLocation = "../file.log";

    internal_lib::Async_Logger logger(fileLocation, &order, &matching, &broadcast);
    
    auto producer_order = [](Common::LFQueue<internal_lib::LogElement>& order, std::vector<internal_lib::LogElement> order_test){
       
      int cnt = 0;
      while (cnt < SIZE) {
        auto p = order.getNextToWriteTo();

        while (p == nullptr) {
            std::this_thread::yield();
            p = order.getNextToWriteTo();    
        }

        *(p) = order_test[cnt];
        order.updateWriteIndex();
        cnt++;
      }

    };

    auto producer_matching = [](Common::LFQueue<internal_lib::LogElement>& match, std::vector<internal_lib::LogElement> match_test){
       
      int cnt = 0;
      while (cnt < SIZE) {
        auto p = match.getNextToWriteTo();

        while (p == nullptr) {
            std::this_thread::yield();
            p = match.getNextToWriteTo();    
        }

        *(p) = match_test[cnt];
        match.updateWriteIndex();
        cnt++;
      }

    };

    auto producer_broadcast = [](Common::LFQueue<internal_lib::LogElement>& broadcast, std::vector<internal_lib::LogElement> broadcast_test){
       
      int cnt = 0;
      while (cnt < SIZE) {
        auto p = broadcast.getNextToWriteTo();

        while (p == nullptr) {
            std::this_thread::yield();
            p = broadcast.getNextToWriteTo();    
        }

        *(p) = broadcast_test[cnt];
        broadcast.updateWriteIndex();
        cnt++;
      }
    };

     auto t0 = createAndStartThread(0, "this is logger thread", [&logger](){ logger.run(); });
     auto t1 = createAndStartThread(1, "this is Order thread", producer_order, order, order_test);
     auto t2 = createAndStartThread(2, "this is Matching thread", producer_matching, matching, matching_test);
     auto t3 = createAndStartThread(3, "this is Broadcast thread", producer_broadcast, broadcast, broadcast_test);

    t1->join();
    t2->join();
    t3->join();

    std::cout << "Producer done producing" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    logger.stop();

    t0->join();

    std::cout << "Test complete" << std::endl;

    return 0;
}

