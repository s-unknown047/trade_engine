#pragma once
#include <cstdint>
#include <iostream>
#include<fstream>
#include<thread>
#include<atomic>
#include <variant>
#include <string>

#include "lock_free_queue.h"
#include "macros.h"
#include "order_gateway_struct.h"

#define LIMIT 50

namespace internal_lib {

  struct LogElement {
    uint64_t log_id;
    uint64_t timestamp;
    // this is just like union but hve info at which index data is stored 
    std::variant<internal_lib::LOBOrder, internal_lib::UserOrder, internal_lib::UserAck, internal_lib::LOBAck> log_data;
  };

  // we have identifier arriving at Order Gateway = 1 log UserOrder
  // we have identifier from orderGateway to Matching engine = 2  log Data = LOBOrder
  // we have arrived at matcing engine  idetifier no = 3  log data = LOBOrder
  // order aggrive match identifier no = 4 log data = LOBOrder
  // order created in LOB identifier  = 5  log Data = loborder
  // order Deleted in LOB id = 6 log = ""
  // order quantity update id  = 7  loborder
  // we will not log price change as it is deleting order and creating it 
  // matching engine ack id = 8  log = loback
  // order Gateway Received an ack id = 9 log data loback
  // order Gateway send an ack to sniper/alpha id = 10 userack
  

  
  class Async_Logger final {

    private:
      Common::LFQueue<internal_lib::LogElement>* matching_engine_queue;
      Common::LFQueue<internal_lib::LogElement>* order_gateway_queue;
      
      std::string file_name_;
      std::atomic<bool> running = {true};
    
    public:
      Async_Logger(std::string& path, 
      Common::LFQueue<internal_lib::LogElement>* mtqueue,
      Common::LFQueue<internal_lib::LogElement>* orderqueue): matching_engine_queue(mtqueue), order_gateway_queue(orderqueue), file_name_(path){
    }

    inline void stop() noexcept {
      running = false;
    }

    void run() noexcept {
      std::ofstream file(file_name_, std::ios::out | std::ios::trunc);

      ASSERT(file.is_open(), "File not accessible");
  
      std::vector<char> buff(128*1024); // 128 kb

      // buff.data() give a pointer to the start of the buffer


      // set the buffer for the file stream 
      // file have its own buffer but of 4kb or 8kb 
      // so we define our own buffer of 128 kb
      // when the buffer get full uses system call to write the whole bufffer to the file
      // so we have increase the size thus reduce the number of system call required and thus reduce number of context switching required thus
      // reduce overheads 

      file.rdbuf()->pubsetbuf(buff.data(),buff.size());
  

      while (running) {
        
        bool busy = false;

        busy |= drainBatch(matching_engine_queue, file, LIMIT);
        busy |= drainBatch(order_gateway_queue, file, LIMIT);
        
        if (busy == false) std::this_thread::yield();
      }

      file.flush();
    }

      char* convert_u64_to_str(uint64_t value, char* buffer) noexcept {
        
        // we define a temp buffer size of 24 that means str can't have more character as int can't have more then 24 characters
      
        char temp[24];
        // take a pointer p at the end of temp array at 23 index
        char* p = temp + 23;
        // let the last char to be null to show it as end of string 
        *p = '\0';
        
        do {
          // take last value convert to ascii character and strore a index p-1 of temp 
          *--p = (value % 10) + '0';
          // update the value by removing last character          
             value = value/10;
        } while (value > 0);
        
        // then when the whole value is converted into to character array then store it to the buffer
        while ((*p)) *buffer++ = *p++;
        
        return buffer;
      }

      char* write_string(const char* str, char* buffer) {
        
        while (*str) {
          *buffer++= *str++;
        }
        
        return buffer;

      }

      char* write_UserOrder(const UserOrder& order, char* buffer) {
          buffer = write_string("user order Arrived_cycle count : ", buffer);
          *buffer++ = '\n';
          buffer = convert_u64_to_str(order.arrived_cycle_count, buffer);
          buffer = write_string ("UserOrder id", buffer);
          buffer = convert_u64_to_str(order.order_id, buffer);
          *buffer++ = '\n';
          buffer = write_string("Trade id", buffer);
          buffer = 
      }

      
      bool drainBatch(Common::LFQueue<internal_lib::LogElement>* q, std::ofstream& file, int limit) noexcept {
        
        // define a 4kb stack buffer to store value of LogElement we can store at most limit amout of logs   
        char buffer[4096];
        // start of buffer
        char* offset = buffer;
        // this pointing to end of the buffer - 128 for saftey so that there 
        // can be no chances that end should no point to out of bound of buffer
        char* end = buffer + sizeof(buffer) - 128;
        
        // count so that at most we can read 50 LogElement  
        int count = 0;

        while (count < limit && offset < end) {
          internal_lib::LogElement* elem = q->getNextToRead();
          if (!elem) break; // nothing to put in buffer 
         
          offset = convert_u64_to_str(elem->log_id, offset);
          *offset++ = ' ';
          offset = convert_u64_to_str(elem->timestamp, offset);
          *offset++ = ' ';
           
          switch (elem->log_data.index()) {  // elem->log_data.index() give the index of at which value is stored  0 -> oder 1 -> match 2 -> broadcast

            case 1:
            offset = write_userOrder((std::get<internal_lib::order>(elem->log_data)).order_id, offset);
            *offset++ = ' ';
            break;
            case 1:
            offset = convert_u64_to_str((std::get<internal_lib::match>(elem->log_data)).match_id, offset);
            *offset++ = ' ';
            break;
            case 2:
            offset = convert_u64_to_str((std::get<internal_lib::broadcast>(elem->log_data).broadcast_id), offset); // convert the integer to characters and store to the buffer
            *offset++ = ' ';

          }

          *offset++ = '\n'; // add new line
          q->updateReadIndex(); // update the reader pointer to the next element of queue
          
          count++;
        }
        
        // what are the two ways this program will end 
        // offset > buffer means buffer is no empty and offset - buffer is the size of buffer 
        // then write it into the 128kb buffer 
        if (offset > buffer) file.write(buffer, offset - buffer);

        return count > 0; // means something is read from the queue so return true 
    } 
  
  };
};