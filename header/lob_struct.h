#pragma once
#include "order_gateway_struct.h"

#define LIKELY(x) __builtin_except(!!(x), 1)
#define UNLIKELY(x) __builtin_except(!!(x), 0)
#define LUT_SIZE 10000000

namespace internal_lib {
    
    template <bool isBuy>
    class limitedOrderBook {
    
        size_t best_price; // this points to the optimal/best prize of the order 
        std::vector<std::vector<internal_lib::LOBOrder>> book; // have all the price table from min_price to max prize at gap of 0.1 prize difference  
        std::vector<pair<int, int>> LUT; // this is to look up price and col number based on system_id
        std::vector<int> active_counts; // count number of live order for a specific price
        size_t max_price_possible;
        std::vector<internal_lib::LOBOrder> emptyOrderList;

        this piece of code is used when order creation to get best_price
        //  const float price = Order->price;
        //     if (Order->order_type == 'b')  {
        //         if (best_price > price) {
        //            best_price = price; 
        //         }
        //     } else if (Order->order_type == 's'){
        //         if (best_price > price) {
        //             best_price = price;
        //         }
        //     }

        // rather then using linked list for having the prices that are active and deleting it we use a vector approch
        // a vector which store active index (the prices that have quantity ) and we linarly seach so this is cache friendly method
        // as cu already calculates all the next index to visisted and in linked list approach we have to jump from address to address 
        // for which cache have to flush and reflush every time branch prediction and thus add overheads 
         void glide_best_prize() noexcept {
           if (isBuy) {  // this is templeate added to every order 
               while (best_price > 0 && active_counts[best_price] == 0 ) best_price--; // if someone want to buy then the price should be as low as possible and there should be some active  order present 
           } else {
            while (best_price < max_price_possible &&  active_counts[best_price] == 0) best_price++; // if sell, try to sell at amx price possible 
           }
        }
        public : 

        limitedOrderBook() = delete; // remove default constructor 
        limitedOrderBook(size_t max_price, size_t max_number_of_entries_of_same_prize) : max_prize_possible(max_prize) {
                book.resize(max_price);

                for (std::vector<internal_lib::LOBOrder> t: book) {
                    t.reserve(max_number_of_entries_of_same_prize);
                }

                active_count.resize(max_price, 0);
                LUT.resize(LUT_SIZE, {-1, -1}); // at most we have can have 10 millions different order means 10 millions different system_id

                if(IsBuy)  best_price = 0;
                else best_price = max_price;
        
        }

        internal_lib::LOBOrder* peekOrder(uint32_t systemid) {
            if (UNLIKELY(systemid >= LUT_SIZE)) return nullptr;
            return &book[LUT[systemid].first][LUT[systemid].second];
        } 

        std::vector<internal_lib::LOBOrder>* get_order_at_prize(size_t best_ask_price) {
            if (UNLIKELY( best_ask_price >= max_price_possible)) {
               return nullptr;
            }
            return &(book[best_ask_price]);
        }

        void createOrder(LOBOrder& order) {
            size_t price_index = static_cast<size_t>(order.price * 10);
            
        }

        

    };
};