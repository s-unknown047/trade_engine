#pragma once
#include <cstdint>
// this include four struct
// User Order is the order data given by the market participants to the ordergateway
// and its receive an acknowledgement from the market
// then that order is converted to LOB order
// LOB order is the data which is given to the matching engine
// ordergateway send it to matching engine and receive ack from the matching engine

// sniper high precision trading stategy
namespace internal_lib
{

    // we will make sure that our structure are power of 2 because it make them chache and memory frendly
    // as the first address of every structure must be divisible by 2 and if it is multiple of two then

    // 32 byte
    struct LOBOrder
    {
        // 8 byte
        uint64_t arrival_cycle_count;

        // 8 byte
        int system_id; // every order have its unique id in order gateway
        float price;

        // 8 byte
        int quantity;
        short int trade_id; // 2 byte id for traders 100 total users 99 market participants and 1 sniper
        char order_type;    // b = buy and s = sell
        char req_type;      // c - create u - update d - delete

        // 8 byte
        uint64_t out_cycle_count;
    };

    struct LOBAck
    {   // 4
        int system_id;
        // 8
        float price;  //
        int quantity; // trade quntity (if match) or remaining qty (if updated/new order)
       
        // 2
        char side;   // B or S
        char status; // -


        // STATUS CODES:
        // 'N' = New Order Accepted  (Qty = Initial Size)
        // 'U' = Update Accepted     (Qty = New Balance)
        // 'C' = Cancel Accepted     (Qty = 0)
        // 'T' = Trade / Fill        (Qty = Executed Amount)
        // 'R' = Rejected            (Qty = 0)
    };

    // 32 byte
    struct UserOrder
    {
        uint64_t arrived_cycle_count;

        int order_id;
        short trader_id;
        char order_type;
        char req_type;

        float price;
        int quantity;

        uint64_t out_cycle_count;
    };
     
    // 16 byte
    struct UserAck
    {
        // 4 byte
        int order_id;
        
        // 8 byte
        float price;
        int quantity;

        // 2 byte
        char side;
        char status;

        // usable data 14
        short int pad;
    };
};