#pragma once 

#include "../header/lock_free_queue.h"      
// #include "../header/macros.h"       
#include "../header/order_gateway_struct.h"
#include "../header/logger.h"   
#include <unordered_map>
#include "../header/bench.h"

#define size_ 1000005

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
        

        // uint64_t throttle_cycles = 400;
        
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
            order_gateway_time.reserve(size_);   
            map.reserve(size_);
            LUT.resize(size_);
        }  

        long long SystemToOrderId(int sysId) noexcept {
            if (sysId < LUT.size()) return LUT[sysId];
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


            // it means we are getting the value of start_order_gateway with memomry 
            // odering of acquire/release which means that no memory read and write after this
            // can be reordered  

            while (!start_order_gateway.load(std::memory_order_acquire)) {
                   if (terminate_order_gateway.load(std::memory_order_acquire)) return;                
            }

            while (!terminate_order_gateway.load(std::memory_order_acquire)){
                // this is order gateway processing function 
                auto gatewayOrderProcess = [&] (UserOrder* order) {
                    
                    internal_lib::compiler_barrier();
                    uint64_t arrivalCycle = internal_lib::now_cycle();
                    internal_lib::compiler_barrier();

                    auto write_log = Logger->getNextToWriteTo();

                    if (write_log != nullptr) {
                        write_log->log_id = internal_lib::GATEWAY_RECEIVE_USER_ORDER;
                        write_log->timestamp =  arrivalCycle;
                        write_log->log_data = *order;
                        Logger->updateWriteIndex();
                    }
                    
                    // std::cout<< "log id " << write_log->log_id << std::endl;

                    int sys_id = AssignSystemId(order->order_id);

                    // creating LOBOrder* request for the SniperOder (userOrder)
                    
                    internal_lib::LOBOrder* write =  LobOrderQueue->getNextToWriteTo();
                    
                    // std::cout <<"in Sam inside hell"<< std::endl;

                    if (LIKELY(write != nullptr)) {
                        //   std::cout <<"in am inside hell"<< std::endl;
                        write->arrival_cycle_count = arrivalCycle;
                        write->system_id = sys_id;
                        write->order_type = order->order_type;
                        write->price = order->price;
                        write->quantity = order->quantity;
                        write->trade_id = order->trader_id;     
                        write->req_type = order->req_type;
                        internal_lib::compiler_barrier();
                        write->out_cycle_count = internal_lib::now_cycle(); 
                        internal_lib::compiler_barrier();
                        
                        LobOrderQueue->updateWriteIndex();
                        
                        internal_lib::compiler_barrier();
                        uint64_t endCycle = internal_lib::now_cycle();
                        internal_lib::compiler_barrier();

                        order_gateway_time.push_back(endCycle - arrivalCycle);

                        auto lobLog = Logger->getNextToWriteTo();

                        if (LIKELY(lobLog !=  nullptr) ) {
                            lobLog->log_id = internal_lib::GATEWAY_SEND_TO_ME;
                            internal_lib::compiler_barrier();
                            lobLog->timestamp = internal_lib::now_cycle();
                            internal_lib::compiler_barrier();
                            lobLog->log_data = *write;
                            Logger->updateWriteIndex();
                        }
                    }  
                };
               
                // order received at order Gateway from Sniper       
                  
                UserOrder* readOrder = SniperOrderQueue->getNextToRead();
                
                if (LIKELY(readOrder != nullptr)) {                  
                    gatewayOrderProcess(readOrder);
                    SniperOrderQueue->updateReadIndex();
                }

                
 
                UserOrder* MMreadOrder = MMorderQueue->getNextToRead();
                
                if (LIKELY(MMreadOrder != nullptr)) {
                    gatewayOrderProcess(MMreadOrder);
                    MMorderQueue->updateReadIndex();
                }

                // for processing ack  

                // ack received from Matching engine 

                LOBAck* readAck = LobAckQueue->getNextToRead();
                
                if (LIKELY(readAck != nullptr)) {
                 // log that we  received the ack  from me (matching engine)
                     
                    LogElement* logger = Logger->getNextToWriteTo();
                    if (LIKELY(logger != nullptr)) {
                        logger->timestamp = internal_lib::now_cycle();
                        logger->log_id = internal_lib::GATEWAY_RECEIVE_ME_ACK;
                        logger->log_data = *readAck;
                        Logger->updateWriteIndex();
                    }

                    //  create ack for sniper order 
                    UserAck* writeAck = SniperOrderAck->getNextToWriteTo();
                    
                    if (LIKELY(writeAck  != nullptr)) {
                        writeAck->order_id = SystemToOrderId(readAck->system_id);
                        writeAck->quantity = readAck->quantity;
                        writeAck->price = readAck->price;
                        writeAck->side = readAck->side; // Side is buy or sell
                        writeAck->status = readAck->status;

                        LogElement* logger = Logger->getNextToWriteTo();
                    
                        //   log that gateway  send  user ack
                        if (LIKELY(logger != nullptr)) {
                            logger->log_id = internal_lib::GATEWAY_SEND_USER_ACK;
                            logger->timestamp = internal_lib::now_cycle();
                            logger->log_data = *writeAck;
                            Logger->updateWriteIndex();
                        }
                        
                        SniperOrderAck->updateWriteIndex();
                        LobAckQueue->updateReadIndex();
                    }
                }
             
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            std::string msg = "This is OrderGateway BenchMark";
            double cpc =  internal_lib::get_cycle_per_ns();
            internal_lib::showBenchmark(order_gateway_time, cpc, msg);
        
        }
    };
};