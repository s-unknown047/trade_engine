#pragma once

#include "header/lock_free_queue.h"
#include "header/order_gateway_struct.h"    
#include "header/logger.h"

#include "matching/matching.cpp"
#include "order_gateway/order_gateway.cpp"
#include "alpha_tester.cpp"
#include "header/Thread.h"
#include "header/prewarmer.h"

#define max_price_tick 10000
#define max_entries_per_price 400
#define file_path "eventLog.log"

int main () {

    Common::LFQueue<internal_lib::UserOrder>* sniperOrder;
    Common::LFQueue<internal_lib ::UserAck>* sniperAck;
    Common::LFQueue<internal_lib::LOBOrder>* lob;
    Common::LFQueue<internal_lib::LOBAck>* lobAck;
    Common::LFQueue<internal_lib::BroadCast>* broadCast;
    Common::LFQueue<internal_lib::LogElement>* match_to_log;
    Common::LFQueue<internal_lib::LogElement>* Order_Gateway_to_Logger;
    Common::LFQueue<internal_lib::UserOrder>* marketMaker; 
    size_t max_price = static_cast<size_t>(max_price_tick);
    size_t max_entries = static_cast<size_t>(max_entries_per_price);
    
    
    internal_lib::MatchingEngine matchEngine(max_price, max_entries, lob);
    internal_lib::OrderGateway orderGateway(lob, lobAck, sniperOrder, sniperAck,Order_Gateway_to_Logger, marketMaker );
    
    std::string file_name = static_cast<std::string>(file_path);
    internal_lib::Async_Logger logger(file_name, match_to_log, Order_Gateway_to_Logger);
    internal_lib::AlphaServer alphaServer(sniperOrder, sniperAck, broadCast);

    std::atomic<bool> start_aplha_server;
    start_aplha_server.store(false, std::memory_order_release);
    std::atomic<bool> terminate_alpha_server;
    terminate_alpha_server.store(false, std::memory_order_release);
    std::atomic<bool> start_orderGateway; 
    start_orderGateway.store(false, std::memory_order_release);
    std::atomic<bool> terminate_orderGateway;
    terminate_orderGateway.store(false, std::memory_order_release);
    std::atomic<bool> start_matchingEngine;
    start_matchingEngine.store(false, std::memory_order_release);
    std::atomic<bool> terminate_matchingEngine;
    terminate_matchingEngine.store(false, std::memory_order_release);
    

    // thread creation 

    auto matching_engine_thread = Common::createAndStartThread(1, "Matching Engine", [&]() {
        matchEngine.matchingEngine(start_aplha_server, terminate_alpha_server);
    });    

    auto orderGateway_thread = Common::createAndStartThread(2, "Order Gateway", [&]() {
        orderGateway.run(start_orderGateway, terminate_orderGateway);
    });

    auto alphaServer_thread = Common::createAndStartThread(3, "Alpha Server", [&]() {
        alphaServer.AlphaRun(start_aplha_server, terminate_alpha_server);
    });


    // internal_lib::prewarm(50);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    auto logger_thread =  Common::createAndStartThread(4, "Logger", [&]() {
        logger.run();
    });
    
    std::cout << "I am in main nigga" << std::endl;
    start_matchingEngine.store(true, std::memory_order_release);
    start_orderGateway.store(true, std::memory_order_release);
    start_aplha_server.store(true, std::memory_order_release);
    
    std::cout << "this is try" << std::endl;

    // stop now
    // wait for 5 second  
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "this is try" << std::endl;
  
    terminate_matchingEngine.store(true, std::memory_order_release);
    terminate_orderGateway.store(true, std::memory_order_release);
    terminate_alpha_server.store(true, std::memory_order_release);
    std::cout << "this is try" << std::endl;

    matching_engine_thread->join();
    orderGateway_thread->join();
    alphaServer_thread->join();

    delete matching_engine_thread;
    delete orderGateway_thread;
    delete  alphaServer_thread;
    
        
    logger.stop();
    logger_thread->join();

    delete logger_thread;
    
    return 0;
} 