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

#define LIMIT 100

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
  

  // final keyword represents this class can not be derived thus reduce overheads
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
      std::cout << "logger,h" << std::endl;
      std::ofstream file(file_name_, std::ios::out | std::ios::trunc);

      if (!file.is_open()) {
          std::cerr << "Failed to open log file: " << file_name_ << std::endl;
          return;
      }

      std::vector<char> buff(128 * 1024); // 128 KB
      file.rdbuf()->pubsetbuf(buff.data(), buff.size());

      while (running) {
          bool busy = false;
                std::cout << "logger,h 1" << std::endl;

          busy |= drainBatch(matching_engine_queue, file, LIMIT);
          busy |= drainBatch(order_gateway_queue, file, LIMIT);
          std::cout<< "is this working";
          if (busy == false) std::this_thread::yield();
      }

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

      char* float_to_char(const float num, char* buffer) {
          int int_part = int(num);
          buffer = convert_u64_to_str(int_part, buffer);
          *buffer++ = '.';

          int frac_part = (int)((num - int_part) * 100);
          if (frac_part < 0) frac_part =  -frac_part;
          
          *buffer++ = (frac_part / 10) + '0';
          *buffer++ = (frac_part % 10 ) + '0';

          return buffer;
      }

      char* write_UserOrder(const UserOrder &order, char *buffer)
      {

        std::cout<<" persisted : "<< order.order_id <<"\n";
        buffer = write_string("user order Arrived_cycle count : ", buffer);
        *buffer++ = '\n';
        buffer = convert_u64_to_str(order.arrived_cycle_count, buffer);
        buffer = write_string("UserOrder id ", buffer);
        buffer = convert_u64_to_str(order.order_id, buffer);
        *buffer++ = '\n';
        buffer = write_string("Trade id ", buffer);
        buffer = convert_u64_to_str(order.trader_id, buffer);
        *buffer++ = '\n';
        buffer = write_string("req_type ", buffer);
        *buffer = order.req_type;
        buffer = write_string("price ", buffer);
        buffer = float_to_char(order.price, buffer);
        *buffer++ = '\n';
        buffer = write_string("quantity", buffer);
        buffer = convert_u64_to_str(order.quantity, buffer);
        *buffer++ = '\n';
        buffer = write_string("out cycle count ", buffer);
        buffer = convert_u64_to_str(order.out_cycle_count, buffer);
        *buffer++ = '\n';
        return buffer;
      }

      char* write_LOBOrder(const LOBOrder &lobOrder, char *buffer) {
        buffer = write_string("arrival_cycle_count", buffer);
        *buffer++ = '\n';
        buffer = convert_u64_to_str(lobOrder.arrival_cycle_count, buffer);
        *buffer++ = '\n';
        buffer = write_string("LOB System_id", buffer);
        *buffer++ = '\n';
        buffer = convert_u64_to_str(lobOrder.system_id, buffer);
        *buffer++ = '\n';
        buffer = write_string("LOB price ", buffer);
        *buffer++ = '\n';
        buffer = float_to_char (lobOrder.price, buffer);
        *buffer++ = '\n';
        buffer = write_string("LOB quantity", buffer);
        *buffer++ = '\n';
        buffer = convert_u64_to_str(lobOrder.quantity, buffer);
        *buffer++ = '\n';
        buffer = write_string("lob ordertype ", buffer);
        *buffer++ = '\n';
        *buffer++ = lobOrder.order_type;
        buffer = write_string("lob req_type", buffer);
        *buffer++ = '\n';
        *buffer++ = lobOrder.req_type;
        *buffer++ = '\n';
        buffer = write_string("lob trade id ",buffer);
        *buffer++ = '\n';
        buffer = convert_u64_to_str(lobOrder.trade_id, buffer);
        *buffer++ = '\n';
        buffer = write_string("Out of Cycle ", buffer);
        *buffer++ = '\n';
        buffer = convert_u64_to_str(lobOrder.out_cycle_count, buffer);
        *buffer++ = '\n';     
        return buffer; 
      }

      char* write_lobAck (const LOBAck &lback, char* buffer) {
        buffer = write_string("System id", buffer);
        *buffer++= '\n';
        buffer = convert_u64_to_str(lback.system_id, buffer);
        *buffer ++ = '\n';
        buffer = write_string("Price ",buffer);
        *buffer++ = '\n';
        buffer = float_to_char(lback.price, buffer);
        *buffer ++ = '\n';
        buffer = write_string("Quantity ",buffer);
        *buffer++ = '\n';
        buffer =  convert_u64_to_str(lback.quantity, buffer);
        *buffer++ = '\n';
        buffer = write_string("side ",buffer);
        *buffer++ = '\n';
        *buffer++ = lback.side;
        *buffer++ = '\n';
        buffer = write_string("Status ",buffer);
        *buffer++ = '\n';
        *buffer++ = lback.status;
        *buffer++ = '\n';
        return buffer;
      }

      char* write_UsrAck (const UserAck& usrAck, char* buffer) {
        buffer = write_string("Order id", buffer);
        *buffer++= '\n';
        buffer = convert_u64_to_str(usrAck.order_id, buffer);
        *buffer ++ = '\n';
        buffer = write_string("Price ",buffer);
        *buffer++ = '\n';
        buffer = float_to_char(usrAck.price, buffer);
        *buffer ++ = '\n';
        buffer = write_string("Quantity ",buffer);
        *buffer++ = '\n';
        buffer =  convert_u64_to_str(usrAck.quantity, buffer);
        *buffer++ = '\n';
        buffer = write_string("side ", buffer);
        *buffer++ = '\n';
        *buffer++ = usrAck.side;
        *buffer++ = '\n';
        buffer = write_string("Status ",buffer);
        *buffer++ = '\n';
        *buffer++ = usrAck.status;
        *buffer++ = '\n';
        return buffer;
      }

      bool drainBatch(Common::LFQueue<internal_lib::LogElement>* q, std::ofstream& file, int limit) noexcept {
        
        // define a 4kb stack buffer to store value of LogElement we can store at most limit amout of logs   
        std::cout<< "in drain batch \n"; 
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
         
          std::cout<<" received " <<elem->log_id<<"\n";
          offset = convert_u64_to_str(elem->log_id, offset);
          *offset++ = ' ';  
          offset = convert_u64_to_str(elem->timestamp, offset);
          *offset++ = ' ';
           
          switch (elem->log_id) {  // elem->log_data.index() give the index of at which value is stored  0 -> oder 1 -> match 2 -> broadcast

            case 1:
            offset = write_UserOrder(std::get<internal_lib::UserOrder>(elem->log_data) , offset);  
            *offset++ = ' ';
            break;
            case 2:
            offset = write_LOBOrder(std::get<internal_lib::LOBOrder>(elem->log_data), buffer);
            *offset++ = ' ';
            break;
            case 3:
            offset = write_LOBOrder(std::get<internal_lib::LOBOrder>(elem->log_data), buffer); 
            *offset++ = ' ';
            break;
            case 4:
            offset = write_LOBOrder(std::get<internal_lib::LOBOrder>(elem->log_data),buffer); 
            *offset++ = ' ';
            break;
            case 5:
            offset =write_LOBOrder(std::get<internal_lib::LOBOrder>(elem->log_data),buffer); 
            *offset++ = ' ';
            break;
            case 6:
						// Order Deleted In LOB ====> Identifier = 6 ====> Log Data = LOBOrder
						offset = write_LOBOrder(std::get<LOBOrder>(elem->log_data), offset);
                 		break;

					case 7:
						// Order Quantity Update in LOB ====> Identifier = 7 ====> Log Data = LOBOrder
						offset = write_LOBOrder(std::get<LOBOrder>(elem->log_data), offset);
                 		break;

					case 8:
						// Matching Engine Passed An Acknowledgement To Order Gateway====> Identifier = 8 ======> Log Data = LOBAcknowledgement
						offset = write_lobAck(std::get<LOBAck>(elem->log_data), offset);
                 		break;

					case 9:
						// Order Gateway Received An Acknowledgement ====> Identifier = 9 =====> Log Data = LOBAcknowledgement
						offset = write_lobAck(std::get<LOBAck>(elem->log_data), offset);
                 		break;

					case 10:
						// Order Gateway Sends An Acknowledgement To Sniper/Alpha ====> Identifier = 10 =====> Log Data = UserAcknowledgement
						offset = write_UsrAck(std::get<UserAck>(elem->log_data), offset);
            break;
          }

          *offset++ = '\n'; // add new line
          q->updateReadIndex(); // update the reader pointer to the next element of queue
          
          count++;
        }
        
        // what are the two ways this program will end 
        // offset > buffer means buffer is no empty and offset - buffer is the size of buffer 
        // then write it into the 128kb buffer 
        if (offset > buffer) file.write(buffer, offset-buffer);

        return count > 0; // means something is read from the queue so return true 
    } 
  };
};