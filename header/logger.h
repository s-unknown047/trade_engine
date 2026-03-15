#pragma once
#include <cstdint>
#include <iostream>
#include<fstream>
#include<thread>
#include<atomic>
#include <variant>
#include <string>

#include "../header/lock_free_queue.h"
#include "../header/macros.h"

namespace internal_lib {

  struct order{
    uint64_t order_id;
  };
  
  // match struct
  struct match{
    uint64_t match_id;
  };
  
  // broadcast struct
  struct broadcast{
    uint64_t broadcast_id;
  };

  struct LogElement {
    uint64_t log_id;
    uint64_t timestamp;
    // this is just like union but hve info at which index data is stored 
    std::variant<order, match, broadcast> log_data;
  };

  
  class Async_Logger final {

    private:
      Common::LFQueue<internal_lib::LogElement>* order_queue_;
      Common::LFQueue<internal_lib::LogElement>* match_queue_;
      Common::LFQueue<internal_lib::LogElement>* broadcast_queue_;

      std::string file_name_;
      std::atomic<bool> running = {true};
      
    public:

      Async_Logger(std::string& path, 
      Common::LFQueue<internal_lib::LogElement>* order,
      Common::LFQueue<internal_lib::LogElement>* match,
      Common::LFQueue<internal_lib::LogElement>* broadcast): order_queue_(order), match_queue_(match), broadcast_queue_(broadcast), file_name_(path){
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

        busy |= drainBatch(order_queue_, file, 50);
        busy |= drainBatch(match_queue_, file, 50);
        busy |= drainBatch(broadcast_queue_, file, 50);
        
        if (busy == false) std::this_thread::yield();
      
      }

      file.flush();
    }

      char* convert_u64_to_str(uint64_t value, char* buffer) noexcept {
        
        // we define a temp buffer size of 24 that means str can't have more character 
        // than 24 
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

            case 0:
            offset = convert_u64_to_str((std::get<internal_lib::order>(elem->log_data)).order_id, offset);
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