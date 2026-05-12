#pragma once 

#include "../header/lock_free_queue.h"      
// #include "../header/macros.h"       
#include "../header/order_gateway_struct.h"
#include "../header/logger.h"   
#include <unordered_map>


namespace internal_lib {
    
    class OrderGateway {
 
        private: 
        // LOB Communication 
        Common::LFQueue<internal_lib::LOBOrder>* LobOrderQueue;
        Common::LFQueue<internal_lib::LOBAck>* LobAckQueue;

        // sniper Communication
        Common::LFQueue<internal_lib::UserOrder>* SniperOrderQueue;
        Common::LFQueue<internal_lib::UserAck>* SniperOrderAck;

        // input queue For Market Maker
        Common::LFQueue<internal_lib::UserOrder>* MMorderQueue;
        
        // for logging data
        Common::LFQueue<internal_lib::LogElement>* Logger;
        
        // for converting order id to system id
        std::unordered_map<long long, int> map;

        // for converting system id -> order id 
        std::vector<long long> LUT; 

        int next_system_id = 0; //  this is system id val st from zere
        
        int orders_received = 0;
        int LOB__orders_sent = 0;

        std::vector<uint64_t> order_gateway_time;
        

        uint64_t throttle_cycles = 400;
        
        //  inline void busy_spin_throttle () noexcept {
        //     if (LIKELY(throttle_cycles > 0)) {
        //         uint64_t start = 
        //     }
        //  }

    public: 
        OrderGateway(Common::LFQueue<internal_lib::LOBOrder>* laq,
        Common::LFQueue<internal_lib::LOBAck>* LOBAck,
        Common::LFQueue<internal_lib::UserOrder>* uo,
        Common::LFQueue<internal_lib::UserAck>* uack,
        Common::LFQueue<internal_lib::LogElement>* log,
        Common::LFQueue<internal_lib::UserOrder>* mm) : LobOrderQueue(laq), LobAckQueue(LOBAck), SniperOrderAck(uack), SniperOrderQueue(uo), MMorderQueue(mm), Logger(log) {
            order_gateway_time.reserve(10000);   
            map.reserve(10000);
            LUT.resize(10000);
        }  

        long long SystemToOrderId(int sysId) noexcept {
            if (LIKELY(sysId < LUT.size())) return LUT[sysId];
            return -1;
        }
         
        int AssignSystemId(uint64_t orderId) {
            int sysId = next_system_id++;
            map.insert({orderId, sysId});
            LUT[sysId] = orderId;
            return sysId; 
        }

        int GetSystemId(long long orderId) {
            return map.find(orderId) != map.end() ? map[orderId] : -1;
        }

        void run (std::atomic<bool>& start_order_gateway,
        std::atomic<bool>& terminate_order_gateway) noexcept {

            std::cout << "this is order Gateway " << std::endl;

            // it means we are getting the value of start_order_gateway with memomry 
            // odering of acquire/release which means that no memory read and write after this
            // can be reordered  

            while (!start_order_gateway.load(std::memory_order_acquire)) {
                   if (terminate_order_gateway.load(std::memory_order_acquire)) return;                
            }

            while (!terminate_order_gateway.load(std::memory_order_acquire)){
               // order received at order Gateway from Sniper               
                UserOrder* readOrder = SniperOrderQueue->getNextToRead();

                if (LIKELY(readOrder != nullptr)) {
                    
                    uint64_t arrival_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    

                    auto write_log = Logger->getNextToWriteTo();

                    if (write_log != nullptr) {
                        write_log->log_id = 1;
                        write_log->timestamp =  arrival_time;
                        write_log->log_data = *readOrder;
                        Logger->updateWriteIndex();
                    }
                    
                    int sys_id = AssignSystemId(readOrder->order_id);

                    uint64_t  end_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    
                    order_gateway_time.push_back(end_time - arrival_time);

                    // creating LOBOrder* request for the SniperOder (userOrder)
                    
                    internal_lib::LOBOrder* write =  LobOrderQueue->getNextToWriteTo();

                    if (LIKELY(write != nullptr)) {

                        write->arrival_cycle_count = readOrder->arrived_cycle_count;
                        write->system_id = sys_id;
                        write->order_type = readOrder->order_type;
                        write->price = readOrder->price;
                        write->quantity = readOrder->quantity;
                        write->trade_id = readOrder->trader_id;     
                        write->req_type = readOrder->req_type;
                        write->out_cycle_count = readOrder->out_cycle_count; 

                        auto lobLog = Logger->getNextToWriteTo();

                        if (LIKELY(write !=  nullptr)) {
                            
                            lobLog->log_id = 2;
                            lobLog->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                            lobLog->log_data = *write;

                            Logger->updateWriteIndex();
                        }

                        LobOrderQueue->updateWriteIndex();
                        SniperOrderQueue->updateReadIndex();

                    }
                }

                UserOrder* MMreadOrder = MMorderQueue->getNextToRead();

                
                if (LIKELY(MMreadOrder != nullptr)) {
                    LogElement* log = Logger->getNextToWriteTo();
                    if (LIKELY(log != nullptr)) {
                        log->log_id = 1;
                        log->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                        log->log_data = *MMreadOrder;
                        Logger->updateReadIndex();
                    }
                    
                    LOBOrder* write = LobOrderQueue->getNextToWriteTo();

                    if (LIKELY(write != nullptr)) {

                        write->arrival_cycle_count = MMreadOrder->arrived_cycle_count;
                        write->system_id = AssignSystemId(MMreadOrder->order_id);
                        write->order_type = MMreadOrder->order_type;
                        write->price = MMreadOrder->price;
                        write->quantity = MMreadOrder->quantity;
                        write->trade_id = MMreadOrder->trader_id;     
                        write->req_type = MMreadOrder->req_type;
                        write->out_cycle_count = MMreadOrder->out_cycle_count;
                        
                        // order is sent from order Gateway to Matching Engine
                        LogElement* log = Logger->getNextToWriteTo();
                        
                        if (LIKELY(log != nullptr)) {
                            log->log_id = 2;
                            log->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                            log->log_data = *write;
                            Logger->updateWriteIndex();
                        }

                        LobOrderQueue->updateWriteIndex();
                        MMorderQueue->updateReadIndex();
                    }
                }

                // for processing ack  

                // ack received from Matching engine 

                LOBAck* readAck = LobAckQueue->getNextToRead();

                if (LIKELY(readAck != nullptr)) {
                   
                    LogElement* logger = Logger->getNextToWriteTo();
                    if (LIKELY(logger != nullptr)) {
                        logger->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock().now().time_since_epoch()).count();
                        logger->log_id = 9;
                        logger->log_data = *readAck;
                    }

                    UserAck* writeAck = SniperOrderAck->getNextToWriteTo();

                    if (LIKELY(writeAck  != nullptr)) {
                        writeAck->order_id = SystemToOrderId(readAck->system_id);
                        writeAck->quantity = readAck->quantity;
                        writeAck->price = readAck->price;
                        writeAck->side = readAck->side; // Side is buy or sell
                        writeAck->status = readAck->status;

                        LogElement* logger = Logger->getNextToWriteTo();
                        if (LIKELY(logger != nullptr)) {
                            logger->log_id = 10;
                            logger->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock().now().time_since_epoch()).count();
                            logger->log_data = *writeAck;
                            Logger->updateWriteIndex();
                        }
                        
                        SniperOrderAck->updateWriteIndex();
                        LobAckQueue->updateReadIndex();
                    }
                }
            }

            // std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    };
};