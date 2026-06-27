#pragma once
#include "order_gateway_struct.h"
#include <cstddef>
#include <vector>
#include <utility>
#include <cstdint>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define LUT_SIZE 10000000

namespace internal_lib
{

    template <bool isBuy>
    class limitedOrderBook
    {

        size_t best_price; // this points to the optimal/best price of the order

        // we are having price-based queue (vectors) in which when the order arrives with the price x it stores that in x price list in FIFO manner
        // have all the price table from min_price to max price at gap of 0.1 price difference, book size is number of ticks 
        std::vector<std::vector<internal_lib::LOBOrder>> book; 
        std::vector<std::pair<size_t, size_t>> LUT;            // this is to look up price and col no based on system_id   systemId -> {price, colno in }
        std::vector<int> active_counts;                        // count number of live orders for a specific price
        size_t max_price_possible;                             // max_price_possible is max_price * 10 

        void glide_best_prize() noexcept
        {
            if (isBuy)
            { // this is template added to every order
                while (best_price > 0 && active_counts[best_price] == 0)
                    best_price--; // this is buyer book the person that bid  higher price will be  more aggresssive  so pointer will be on the highest price buyer  
                    // can  offer 
            }
            else
            {
                while (best_price < max_price_possible && active_counts[best_price] == 0)
                    best_price++; // if sellbook the seller selling at lowest  price  will be the most aggressive  buyer
            }
        }

    public:
        limitedOrderBook() = delete; // remove default constructor
        limitedOrderBook(size_t max_price, size_t max_number_of_entries_of_same_price) : max_price_possible(max_price)
        {
            book.resize(max_price);

            for (auto &t : book)
            {
                t.reserev(max_number_of_entries_of_same_price);
            }

            active_counts.resize(max_price, 0);

            LUT.resize(LUT_SIZE, {-1, -1}); // at most we can have 10 million different orders means 10 million different system_ids

            if (isBuy)
                best_price = 0;
            else
                best_price = max_price;     
        }

        internal_lib::LOBOrder *peekOrder(uint32_t systemid)
        {
            if (UNLIKELY(systemid >= LUT_SIZE || LUT[systemid].first == -1 || LUT[systemid].second == -1))
                return nullptr;
            return &book[LUT[systemid].first][LUT[systemid].second];
        }

        std::vector<internal_lib::LOBOrder> *get_order_at_prize(size_t best_ask_price)
        {
            if (UNLIKELY(best_ask_price >= max_price_possible))
            {
                return nullptr;
            }
            return &(book[best_ask_price]);
        }

        void createOrder(LOBOrder &order)
        {

            size_t price_index = static_cast<size_t>(order.price * 10);

            if (isBuy)
            {   // for seller 
                if (best_price < price_index)
                    best_price = price_index;
            }
            else
            {
                if (best_price > price_index)
                    best_price = price_index;
            }
            // push back order into respective price_index
            book[price_index].push_back(order);

            uint32_t system_id = order.system_id;
            LUT[system_id] = {price_index, book[price_index].size() - 1};

            active_counts[price_index]++;
        }

        void UpdateOrderQuantity(LOBOrder &order) noexcept
        {
            uint32_t system_id = order.system_id;

            if (UNLIKELY(system_id >= LUT_SIZE))
                return;

            std::pair<size_t, size_t> p = LUT[system_id];

            if (LIKELY(p.first != -1 && p.second != -1))
            {
                size_t price_index = p.first;
                size_t col_no = p.second;

                uint32_t newQuantity = order.quantity;
                uint32_t oldQuantity = book[price_index][col_no].quantity;

                if (UNLIKELY(newQuantity == oldQuantity))
                    return;

                if (newQuantity < oldQuantity)
                {
                    book[price_index][col_no].quantity = newQuantity;
                } else
                {
                    book[price_index][col_no].quantity = 0;
                    book[price_index].push_back(order);

                    LUT[order.system_id] = {price_index, book[price_index].size() - 1};
                }
            }
        }

        void deleteOrder(int system_id) noexcept
        {
            if (UNLIKELY(system_id > LUT_SIZE))
            {
                return;
            }

            std::pair<int, int> order = LUT[system_id];
            int price_index = order.first;
            int col_no = order.second;

            book[price_index][col_no].quantity = 0;
            LUT[system_id] = {-1, -1};

            active_counts[price_index]--;

            if (UNLIKELY(active_counts[price_index] == 0 && price_index == (int)best_price))
            {
                glide_best_prize();
            }
        }

        size_t get_optimal_price()
        {
            return best_price;
        }
    };
};