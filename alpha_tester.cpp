// this is to test matching engine

#pragma once

#include <thread>
#include "header/lock_free_queue.h"
#include "header/order_gateway_struct.h"

#define order_num 100000

namespace internal_lib
{
    class AlphaServer
    {

    private:
        Common::LFQueue<internal_lib::UserOrder> *AlphaOrderQueue;
        Common::LFQueue<internal_lib::UserAck> *userAck;
        Common::LFQueue<internal_lib::BroadCast> *BroadCast;

        std::vector<internal_lib::UserOrder> TestStore;

    public:
        AlphaServer() = delete;
        AlphaServer(
            Common::LFQueue<internal_lib::UserOrder> *AlphaOrder,
            Common::LFQueue<internal_lib::UserAck> *UserAck,
            Common::LFQueue<internal_lib::BroadCast> *bq) : AlphaOrderQueue(AlphaOrder), userAck(UserAck), BroadCast(bq){
                TestStore.reserve(order_num);
            }

        void AlphaRun(std::atomic<bool> &start, std::atomic<bool> &terminate) noexcept
        {
            std::cout << "this is Alpha Server " << std::endl;
            int num_orders = static_cast<int>(order_num);
            
            // filling
            int count = 0;
            for (int i = 1; i < num_orders; ++i)
            {
                
                internal_lib::UserOrder order;
                // (150 - 110) / 0.1 = 400 steps.
                // rand() % 401 generates 0-400.
                // 110 + (result * 0.1) gives the price.
                order.price = 110.0 + (double)(rand() % 401) * 0.1;
                order.order_id = i;                               // unique order id per order
                order.req_type = 'c';                             // create order
                order.order_type = (rand() % 2 == 0) ? 'b' : 's'; // random buy/sell
                order.quantity = (rand() % 100) + 1;              // 1-100 lots
                order.trader_id = 1;                              // sniper trader
                order.arrived_cycle_count = 0;
                
                TestStore.push_back(order);

                               
            }

            // auto user = TestStore[0];
 
            // std::cout << "correcte " << user.order_id << std::endl;
             
         
            while (!start.load(std::memory_order_acquire))
            {
                if (terminate.load(std::memory_order_acquire))
                    return;
            }

       
            for (int i = 0; i < TestStore.size(); i++)
            {
                run(i);
            }
        }

        void run(int &i) noexcept 
        {
            // Debugging log to check TestStore values

            internal_lib::UserOrder *order = AlphaOrderQueue->getNextToWriteTo();
            while (order == nullptr)
            {
                std::this_thread::yield();
                order = AlphaOrderQueue->getNextToWriteTo();
            }

            // Debugging log to confirm order pointer is valid
          
            *order = TestStore[i];

          

            AlphaOrderQueue->updateWriteIndex();
        }
    };
};
