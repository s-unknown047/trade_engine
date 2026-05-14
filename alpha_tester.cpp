// this is to test matching engine

#pragma once

#include <thread>
#include "header/lock_free_queue.h"
#include "header/order_gateway_struct.h"

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
            Common::LFQueue<internal_lib::BroadCast> *bq) : AlphaOrderQueue(AlphaOrder), userAck(UserAck), BroadCast(bq) {}

        void AlphaRun(std::atomic<bool> &start, std::atomic<bool> &terminate) noexcept
        {
            std::cout << "this is Alpha Server " << std::endl;
            int num_orders = 1;
            // filling
            for (int i = 0; i < num_orders; ++i)
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
                // order.out_cycle_count = 0;
                TestStore.push_back(order);
            }

            while (!start.load(std::memory_order_acquire))
            {
                if (terminate.load(std::memory_order_acquire))
                    return;
            }

            std::cout << "at run" << std:: endl;

           for (int i = 0 ; i < TestStore.size() && !terminate.load(std::memory_order_acquire); i++) {
					run(i);
				}
}

        void run(int &i) noexcept
        {
            std::cout<<"inside run \n ";

            std::cout << " Alppha order " << AlphaOrderQueue << std::endl;
            internal_lib::UserOrder *order = AlphaOrderQueue->getNextToWriteTo();
            std::cout << "Order " << order << std::endl;
            while (order == nullptr)
            {   
                std::cout << "in loop \n"; 
                std::this_thread::yield();
                order = AlphaOrderQueue->getNextToWriteTo();
            }

            std::cout << "i am here 1\n";;

            *order = TestStore[i];

            AlphaOrderQueue->updateWriteIndex();
             std::cout << "i am here 1\n";;

            // internal_lib::BroadCast *cast = BroadCast->getNextToRead();

            // while (cast == nullptr)
            // {
            //     std::this_thread::sleep_for(std::chrono::milliseconds(3));
            //     cast = BroadCast->getNextToRead();
            // }
            //  std::cout << "i am here 2\n";

            // std::cout << "this is broadcast " << cast->system_id << std::endl;
            //  std::cout << "i am here 2\n";

            // internal_lib::UserAck *ack = userAck->getNextToRead();

            // if (ack == nullptr)
            // {
            //     ack = userAck->getNextToRead();
            // }

            // std::cout << "this is User Ack " << ack->order_id << std::endl;
        }
    };
};
