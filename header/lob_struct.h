#pragma once
#include "order_gateway_struct.h"

#define LIKELY(x) __builtin_except(!!(x), 1)
#define UNLIKELY(x) __builtin_except(!!(x), 0)
#define LUT_SIZE 10000000

namespace internal_lib
{

    template <bool isBuy>
    class limitedOrderBook
    {

        size_t best_price; // this points to the optimal/best prize of the order

        // we are having price based queue (vectors) in which when the order arrives with the price x it store that int x price list in fifo manner
        std::vector<std::vector<internal_lib::LOBOrder>> book; // have all the price table from min_price to max prize at gap of 0.1 prize difference
        std::vector<pair<size_t, size_t>> LUT;                 // this is to look up price and col no  based on system_id   systemId -> {price, colno in }
        std::vector<int> active_counts;                        // count number of live order for a specific price
        size_t max_price_possible;
        std::vector<internal_lib::LOBOrder> emptyOrderList;

        // rather then using linked list for having the prices that are active and deleting it we use a vector approch
        // a vector which store active index (the prices that have quantity ) and we linarly seach so this is cache friendly method
        // as cpu already calculates (predicted) all the next index to visisted whereas in linked list approach we have to jump from address to address
        // for which cache have to flush and reflush every time branch prediction and thus add overheads
       
       
        void glide_best_prize() noexcept
        {
            if (isBuy)
            { // this is templeate added to every order
                while (best_price > 0 && active_counts[best_price] == 0)
                    best_price--; // if someone want to buy then the price should be as low as possible and there should be some active  order present
            }
            else
            {
                while (best_price < max_price_possible && active_counts[best_price] == 0)
                    best_price++; // if sell, try to sell at amx price possible
            }
        }


    public:
        limitedOrderBook() = delete; // remove default constructor
        limitedOrderBook(size_t max_price, size_t max_number_of_entries_of_same_prize) : max_prize_possible(max_prize)
        {
            book.resize(max_price);

            for (std::vector<internal_lib::LOBOrder> t : book)
            {
                t.reserve(max_number_of_entries_of_same_prize);
            }

            active_count.resize(max_price, 0);

            LUT.resize(LUT_SIZE, {-1, -1}); // at most we have can have 10 millions different order means 10 millions different system_id

            if (IsBuy)
                best_price = 0;
            else
                best_price = max_price;
        }

        internal_lib::LOBOrder *peekOrder(uint32_t systemid)
        {
            if (UNLIKELY(systemid >= LUT_SIZE)) return nullptr;
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

            //  when the order arrived we first create order first get it's price * 10 to get price index then update the best_price based on the price_index
            //  if order is for buying then we make sure best_price is as max as possible so that that price index entries will be evaluated first (aggresive order)and
            //  and if order is sell  then  we go for best_price as low  (aggresive selling) (when selling price of order is less then the price for buy) then best_price

            if (isBuy)
            {
                if (best_price < price_index)
                    best_price = price_index;
            }
            else
            {
                if (best_price > price)
                    best_price = price;
            }
            // push back order into respective price_index
            book[price_index].push_back(order);

            uint32_t system_id = order.system_id;
            // create lookup for that price if need to process first use system id to get price_index and length of that column have it column no 
            LUT[system_id] = {price_index, book[price_index].size() - 1};

            active_count[price_index]++;

        }

        // order already present only change the quantity we have pop the order then update new Quantity and then push back
        void UpdateOrderQuantity(LOBOrder &order) noexcept {
            uint32_t system_id = order.system_id;

            if (UNLIKELY(system_id >= LUT_SIZE)) return;
            
                
            pair<size_t, size_t> p = LUT[system_id];

            if (LIKELY(p.first != -1 && p.second != -1)) {
                
                size_t price_index = p.first;
                size_t col_no = p.second;
                
                // if quantity decrease just update the order if quantity increase remove update and push_back
                uint32_t newQuantity = order.quantity;
                uint32_t oldQuantity = book[price_index][col_no].quantity;
                if (UNLIKELY(newQuantity == oldQuantity )) return;
                
                if (newQuantity < oldQuantity) {
                    book[price_index][col_no].quantity = newQuantity;
                } else {
                    book[price_index][col_no].quantity = 0;
                    book[price_index].push_back(order);

                    LUT[order.system_id] = {price_index, book[price_index].size() - 1};
                }
            }
            
        }
        // delete order 

        void deleteOrder (int system_id) noexcept{
            if (unlikely(system_id > LUT_SIZE)) {
                return nullptr;
            }

            pair<int, int>  order = LUT[system_id];

            int price_index = order.first;
            int col_no = order.second;

            book[price_index][col_no].quantity = 0;
            LUT[system_is] = {-1, -1};

            active_order[price_index]--;

            if (UNLIKELY(active_order[price_index] == 0 && price_index == (int)best_price)) {
                glide_best_prize();
            }
        }
        size_t get_optimal_price() {
            return best_price;
        }
    };
};