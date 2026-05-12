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
                order.out_cycle_count = 0;
                TestStore.push_back(order);
            }

            while (!start.load(std::memory_order_acquire))
            {
                if (terminate.load(std::memory_order_acquire))
                    return;
            }

            for (int i = 0; i < TestStore.size(); i++)
                run(i);
        };

        void run(int &i) noexcept
        {
            internal_lib::UserOrder *order = AlphaOrderQueue->getNextToWriteTo();

            while (order != nullptr)
            {
                std::this_thread::yield();
                order = AlphaOrderQueue->getNextToWriteTo();
            }

            *order = TestStore[i];

            AlphaOrderQueue->updateWriteIndex();

            internal_lib::BroadCast *cast = BroadCast->getNextToRead();

            if (cast == nullptr)
            {
                cast = BroadCast->getNextToRead();
            }

            std::cout << "this is broadcast " << cast->system_id << std::endl;

            internal_lib::UserAck *ack = userAck->getNextToRead();

            if (ack == nullptr)
            {
                ack = userAck->getNextToRead();
            }

            std::cout << "this is User Ack " << ack->order_id << std::endl;
        }
    };
};
