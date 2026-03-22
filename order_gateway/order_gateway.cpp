#pragma once 

#include "../header/lock_free_queue.h"      
// #include "../header/macros.h"       
#include "../header/order_gateway_struct.h"
#include "../header/logger.h"   
#include <unordered_map>
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 1)

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
        
         inline void busy_spin_throttle () noexcept {
            if (LIKELY(throttle_cycles > 0)) {
                uint64_t start = 
            }
         }

         public: 
         OrderGateway( Common::LFQueue<internal_lib::LOBOrder>* laq,
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
         
        int AssignSystemId(long long orderId) {
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
                UserOrder* readOrder = SniperOrderQueue->getNextToRead();

                if (LIKELY(readOrder != nullptr)) {
                    
                    uint64_t arrival_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    
                    auto write_log = Logger->getNextToWriteTo();

                    if (write_log != nullptr) {
                        write_log->log_id = 1;
                        write_log->timestamp =  arrival_time;
                        write_log->log_data = *readOrder;
                    }
                }
            }

        }



        




    

    };
};