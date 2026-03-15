#include "logger_imp.h"
#include "../header/Thread.h"
#include <chrono>
#include <vector>
#include <thread>
#include <iostream>     
#include <string>   
#include <algorithm> 
#define SIZE 10000000


void benchMark(std::vector<std::chrono::duration<double, std::nano>> consumer, std::vector<std::chrono::duration<double, std::nano>> producer_order, std::vector<std::chrono::duration<double, std::nano>> producer_match, std::vector<std::chrono::duration<double, std::nano>> producer_broaddcast) {

  std::sort(consumer.begin(), consumer.end());
  std::sort(producer_order.begin(), producer_order.end());
  std::sort(producer_broaddcast.begin(), producer_broaddcast.end());
  std::sort(producer_match.begin(), producer_match.end());

 
    std::cout <<" time median: " << consumer[consumer.size() / 2].count() << " ns" << std::endl;
    std::cout <<" time 0.75: " << consumer[consumer.size() * 0.75].count() << " ns" << std::endl;
    std::cout  << " time 0.95: " << consumer[consumer.size() * 0.95].count() << " ns" << std::endl;
    std::cout  << " time 0.99: " << consumer[consumer.size() * 0.99].count() << " ns" << std::endl;

    int size = producer_order.size();
    std::cout <<" time median: " << ( producer_order[size / 2].count() + producer_match[size / 2].count() + producer_broaddcast[size / 2].count() )/3 << " ns" << std::endl;
    std::cout <<" time 0.75: " << ( producer_order[size * 0.75].count() + producer_match[size * 0.75].count() + producer_broaddcast[size * 0.75].count() )/3 << " ns" << std::endl;
    std::cout  << " time 0.95: " << ( producer_order[size * 0.95].count() + producer_match[size * 0.95].count() + producer_broaddcast[size * 0.95].count() )/3 << " ns" << std::endl;
    std::cout  << " time 0.99: " << ( producer_order[size * 0.99].count() + producer_match[size * 0.99].count() + producer_broaddcast[size * 0.99].count() )/3<< " ns" << std::endl;
}


int main() {
    Common::LFQueue<internal_lib::LogElement> order(SIZE*5);
    Common::LFQueue<internal_lib::LogElement> matching(SIZE*5);
    Common::LFQueue<internal_lib::LogElement> broadcast(SIZE*5);

    std::vector<internal_lib::LogElement> order_test;
    std::vector<internal_lib::LogElement> matching_test;
    std::vector<internal_lib::LogElement> broadcast_test;

    order_test.reserve(SIZE);
    matching_test.reserve(SIZE);
    broadcast_test.reserve(SIZE);

     

    for (uint64_t i = 0; i < SIZE; i++) {

        uint64_t log_id_x = i;
        uint64_t log_id_y = i;
        uint64_t log_id_z = i;
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

   
    
    auto producer_order = [](Common::LFQueue<internal_lib::LogElement>& order, std::vector<internal_lib::LogElement>& order_test, std::vector<std::chrono::duration<double,std::nano>>& bench){
      int cnt = 0;
      while (cnt < SIZE) {
        auto t_st = std::chrono::high_resolution_clock::now();
        auto p = order.getNextToWriteTo();
        while (p == nullptr) {
            std::this_thread::sleep_for(std::chrono::microseconds(3));
            p = order.getNextToWriteTo();    
        }

        *(p) = order_test[cnt];
        order.updateWriteIndex();
        auto end_t = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::nano> interval = end_t - t_st;
        bench.push_back(interval);
        cnt++;
      }

    };

    auto producer_matching = [](Common::LFQueue<internal_lib::LogElement>& match, std::vector<internal_lib::LogElement>& match_test, std::vector<std::chrono::duration<double,std::nano>>& bench){
       
      int cnt = 0;
      while (cnt < SIZE) {
        auto t_st = std::chrono::high_resolution_clock::now();
        auto p = match.getNextToWriteTo();

        while (p == nullptr) {
            std::this_thread::sleep_for(std::chrono::microseconds(3));
            p = match.getNextToWriteTo();    
        }

        *(p) = match_test[cnt];

         match.updateWriteIndex();
        auto end_t = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::nano> interval = end_t - t_st;
        bench.push_back(interval);
        cnt++;
      }

    };

    auto producer_broadcast = [](Common::LFQueue<internal_lib::LogElement>& broadcast, std::vector<internal_lib::LogElement>& broadcast_test, std::vector<std::chrono::duration<double,std::nano>>& bench){
       
      int cnt = 0;
      while (cnt < SIZE) {
        auto t_st = std::chrono::high_resolution_clock::now();
        auto p = broadcast.getNextToWriteTo();

        while (p == nullptr) {
            std::this_thread::sleep_for(std::chrono::microseconds(3));
            p = broadcast.getNextToWriteTo();    
        }

        *(p) = broadcast_test[cnt];
        broadcast.updateWriteIndex();
        auto end_t = std::chrono::high_resolution_clock::now();  
        std::chrono::duration<double, std::nano> interval = end_t - t_st;
        bench.push_back(interval);

        cnt++;
      }
    };

    std::vector<std::chrono::duration<double, std::nano>> v_order;
    std::vector<std::chrono::duration<double, std::nano>> v_match;
    std::vector<std::chrono::duration<double, std::nano>> v_broadcast;
    std::vector<std::chrono::duration<double, std::nano>> consumer;

     auto t1 = createAndStartThread(1, "this is Order thread", producer_order, order, order_test, v_order);
     auto t2 = createAndStartThread(2, "this is Matching thread", producer_matching, matching, matching_test, v_match);
     auto t3 = createAndStartThread(3, "this is Broadcast thread", producer_broadcast, broadcast, broadcast_test, v_broadcast);
     auto t0 = createAndStartThread(0, "this is logger thread", [&logger, &consumer](){ logger.run(consumer); });

    t1->join();
    t2->join();
    t3->join();

    std::cout << "Producer done producing" << std::endl;

    // std::this_thread::sleep_for(std::chrono::seconds(2));

    logger.stop();

    t0->join();

    benchMark(consumer, v_order, v_match, v_broadcast );

    std::cout << "Test complete" << std::endl;

    return 0;
}

