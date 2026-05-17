#include "../header/lob_struct.h"
#include "../header/lock_free_queue.h"
#include "../header/order_gateway_struct.h"
#include "../header/logger.h"

namespace internal_lib
{
    class MatchingEngine
    {
    private:
        Common::LFQueue<internal_lib::LOBOrder> *LobOrderQueue;
        Common::LFQueue<internal_lib::LOBAck> *LobAck;
        Common::LFQueue<internal_lib::BroadCast> *BroadCast; 
        Common::LFQueue<internal_lib::LogElement> *matchLog; 

        internal_lib::limitedOrderBook<true> BuyOrderBook;
        internal_lib::limitedOrderBook<false> SellOrderBook;

    public:
        MatchingEngine() = delete;

        MatchingEngine(
            size_t max_price_ticks,
            size_t max_entries_per_price,
            Common::LFQueue<internal_lib::LOBAck> *lobAck ,Common::LFQueue<internal_lib::LOBOrder> *req_order, Common::LFQueue<internal_lib::BroadCast> *broadcast, Common::LFQueue<internal_lib::LogElement> *match) :LobAck(lobAck), LobOrderQueue(req_order), BuyOrderBook(max_price_ticks, max_entries_per_price), SellOrderBook(max_price_ticks, max_entries_per_price), BroadCast(broadcast), matchLog(match){}

        void matchingEngine(std::atomic<bool> &start_Engine, std::atomic<bool> &terminate_engine)
        {
            while (!start_Engine.load(std::memory_order_acquire))
            {
                if (terminate_engine.load(std::memory_order_acquire)) return;
            }

            while (!terminate_engine.load(std::memory_order_acquire))
            { 
                readOrder();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }

        void readOrder() noexcept
        {
            internal_lib::LOBOrder *order = LobOrderQueue->getNextToRead();
                       
            if (UNLIKELY(order == nullptr))
            {
                return;
            }

            auto write_log = matchLog->getNextToWriteTo();
            
            if (LIKELY(write_log != nullptr))
            {
                write_log->log_id = 3;
                write_log->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                write_log->log_data = *order;

                matchLog->updateWriteIndex();
            }
          
            bool is_buy = ((order->order_type == 'b') ? true : false);

            if (order->req_type == 'c')
            {   
                create_order(*order, is_buy);
            }
            else if (order->req_type == 'u')
            {
                updateOrder(*order, is_buy);
            }
            else if (order->req_type == 'd')
            {
                deleteOrder(*order, is_buy);
            }

            LobOrderQueue->updateReadIndex();
        };

        uint64_t create_order(LOBOrder &order, bool is_buy)  noexcept
        {       
            aggresiveMatch(order, is_buy);
          
            if (order.quantity > 0)
            { 
                if (is_buy)
                    BuyOrderBook.createOrder(order);
                else
                    SellOrderBook.createOrder(order);

                auto write_log = matchLog->getNextToWriteTo();
                
                if (UNLIKELY(write_log != nullptr))
                {
                    write_log->log_id = 7;
                    write_log->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    write_log->log_data = order;

                    matchLog->updateWriteIndex();
                }

                // Fixed the ternary char comparison bug here
                changeInQuantity(order.system_id, order.price, order.quantity, 'N', is_buy ? 'B' : 'S');
                 
                if (order.trade_id == 1)
                {
                    Ack(order, static_cast<size_t>(order.price * 10), 'N', order.quantity);
                }
            }
            uint64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            return time;
        }

        uint64_t updateOrder (LOBOrder &order, bool is_buy) noexcept {
            uint64_t done_time = 0;
            LOBOrder* previous_entry;
            if (is_buy) {
                previous_entry = BuyOrderBook.peekOrder(order.system_id);
            } else {
                 previous_entry = SellOrderBook.peekOrder(order.system_id);
            }

            if (UNLIKELY(previous_entry == nullptr)) {
                return 0;
            }

            bool quantity_change = order.quantity != previous_entry->quantity ? true : false; 
            bool price_change = order.price != previous_entry->price ? true : false;

            if (price_change)
            {
                deleteOrder(*previous_entry, is_buy);
                create_order(order, is_buy);
            }
            else if (quantity_change)
            {
                if (is_buy)
                {
                    BuyOrderBook.UpdateOrderQuantity(order);
                }
                else
                {
                    SellOrderBook.UpdateOrderQuantity(order);
                }

                internal_lib::LogElement *write_log = matchLog->getNextToWriteTo();

                if (write_log != nullptr)
                {
                    write_log->log_id = 7;
                    write_log->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                    write_log->log_data = order;

                    matchLog->updateWriteIndex();
                }

                changeInQuantity(order.system_id, order.price, order.quantity, 'U', is_buy ? 'B' : 'S');

                if (order.trade_id == 1)
                {
                    Ack(order, static_cast<size_t>(order.price * 10), 'U', order.quantity);
                }
            }
            
            done_time =  std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            return done_time;
        }

        uint64_t deleteOrder(LOBOrder &order, bool is_buy) {

            if (is_buy) {
                BuyOrderBook.deleteOrder(order.system_id);     
            } else {
                SellOrderBook.deleteOrder(order.system_id);
            }
            
            auto write_log = matchLog->getNextToWriteTo();
            
            if (UNLIKELY(write_log != nullptr)) {
                write_log->log_id = 6;
                write_log->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                write_log->log_data = order;

                matchLog->updateWriteIndex();
            }
            
            changeInQuantity(order.system_id, order.price, order.quantity, 'D', is_buy? 'B' : 'S' );

            if (order.trade_id == 1) {
                Ack(order, static_cast<size_t>(order.price * 10), 'D', order.quantity);
            }
            
            uint64_t time =  std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            return time;

        }   
        
        void changeInQuantity(uint32_t system_id, float price, uint32_t quantity, char req_type, char order_type) noexcept
        {
            auto brodcast = BroadCast->getNextToWriteTo();

            while (brodcast == nullptr)
            {
                std::this_thread::yield();
                brodcast = BroadCast->getNextToWriteTo();
            }

            if (UNLIKELY(brodcast != nullptr))
            {
                brodcast->system_id = system_id;
                brodcast->price = price;
                brodcast->quantity = quantity;
                brodcast->side = order_type;
                brodcast->type = req_type;
                BroadCast->updateWriteIndex();
            }
        }

        void aggresiveMatch(LOBOrder &order, bool is_buy) noexcept
        {   
            if (is_buy)
            {
                // look in sell table
                while (order.quantity > 0)
                {  
                    size_t best_ask_idx = SellOrderBook.get_optimal_price();
                    size_t price = static_cast<size_t>(order.price * 10);
                    
                    if (price < best_ask_idx)
                        break;
                    
                    std::vector<internal_lib::LOBOrder> *level = SellOrderBook.get_order_at_prize(best_ask_idx);

                    // Added Segfault protection 
                    if (level == nullptr || level->empty()) 
                        break;

                    for (LOBOrder &ord : *level)
                    {
                        if (UNLIKELY(ord.quantity == 0))
                            continue;

                        uint32_t trade = ord.quantity < order.quantity ? ord.quantity : order.quantity;

                        order.quantity -= trade;
                        ord.quantity -= trade;

                        auto write_Broadcast = matchLog->getNextToWriteTo();

                        if (LIKELY(write_Broadcast != nullptr))
                        {
                            write_Broadcast->log_id = 4;
                            write_Broadcast->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                            write_Broadcast->log_data = order;
                            matchLog->updateWriteIndex();
                        }

                        if (order.trade_id == 1)
                        {
                            Ack(order, best_ask_idx, 'T', trade);
                        }

                        if (ord.trade_id == 1)
                        {
                            Ack(ord, best_ask_idx, 'T', trade);
                        }

                        if (ord.quantity == 0)
                            SellOrderBook.deleteOrder(ord.system_id);

                        if (order.quantity == 0)
                            break;
                    }
                }
            }
            else
            {
                // look is buy table
                // Fixed infinite loop bug (quantity >= 0 changed to > 0)
                while (order.quantity > 0)
                {
                    size_t price = static_cast<size_t>(order.price * 10);
                    size_t best_bid_idx = BuyOrderBook.get_optimal_price();

                    // Fixed missing spread break
                    if (price > best_bid_idx)
                        break;
                    
                    std::vector<internal_lib::LOBOrder> *buy_order = BuyOrderBook.get_order_at_prize(best_bid_idx);

                    // Added Segfault protection 
                    if (buy_order == nullptr || buy_order->empty()) 
                        break;

                    for (internal_lib::LOBOrder &Border : *buy_order)
                    {
                        if (Border.quantity == 0)
                            continue;

                        uint32_t trade = Border.quantity < order.quantity ? Border.quantity : order.quantity;

                        order.quantity -= trade;
                        Border.quantity -= trade;

                        auto write_Broadcast = matchLog->getNextToWriteTo();

                        if (LIKELY(write_Broadcast != nullptr))
                        {
                            write_Broadcast->log_id = 4;
                            write_Broadcast->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                            write_Broadcast->log_data = order;
                            matchLog->updateWriteIndex();
                        }

                        if (order.trade_id == 1)
                        {
                            Ack(order, best_bid_idx, 'T', trade);
                        }

                        if (Border.trade_id == 1)
                        {
                            Ack(Border, best_bid_idx, 'T', trade);
                        }

                        if (Border.quantity == 0)
                            BuyOrderBook.deleteOrder(Border.system_id);

                        if (order.quantity == 0)
                            break;
                    }
                }
            }
        }

        void Ack(LOBOrder &order, size_t best_price_ask, char req_type, uint32_t trade_quantity) noexcept
        {   
            internal_lib::LOBAck *write_log = LobAck->getNextToWriteTo();
            if (write_log == nullptr)
            {
                std::this_thread::yield();
                write_log = LobAck->getNextToWriteTo();
            }

            if (LIKELY(write_log != nullptr))
            {
                write_log->system_id = order.system_id;
                write_log->quantity = trade_quantity;
                write_log->price = best_price_ask;
                write_log->side = order.order_type;
                write_log->status = req_type;
                LobAck->updateWriteIndex();
            }
        }
    };
};