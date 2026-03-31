#pragma once

#include <thread>
#include <atomic>
#include <unistd.h>
#include <iostream>
#include <sys/syscall.h>
#include <utility> // For std::forward

namespace Common {
 
  inline auto setThreadCore(int core_id) noexcept {   //Set affinity for current thread to be pinned to the provided core_id.
    cpu_set_t cpuset;  // defne an array/set  of CPUs

    CPU_ZERO(&cpuset); // initialize the CPU set to be empty
    CPU_SET(core_id, &cpuset); // add the specified core_id to the CPU set
  
    return (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0); // set affinity of the calling thread to the CPUs in the cpuset
  }
};

//Creates a thread instance, sets affinity on it, assigns it a name and
// this is a template function that can take any callable type T and any number of arguments Args
//  T &&func is a function pointer or callable object to be executed in the new thread
//  Args &&...args are the arguments to be passed to the function when it is called

template <typename T, typename... Args>
inline std::thread* createAndStartThread(int core_id, const std::string &name, T &&func, Args &&...args) noexcept
{

    std::cout<< "Thread core_id " << core_id << std::endl;


    std::atomic<bool> running(false), failed(false); 
    auto thread_body = [&] ()
    {     


        std::cout<<"i am inside"<<std::endl;

        if (core_id >= 0 && !Common::setThreadCore(core_id))
        {
            std::cerr << "Failed to set affinity core to " << name << " for thread " << pthread_self() << " " << core_id << std::endl;
            failed = true;
            return;
        }

        std::cout << "set core aaffinity for " << name << " " << pthread_self() << " to " << core_id << std::endl;

        running = true;
      
        std::forward<T>(func)(std::forward<Args>(args)...);

    };

     
    

   
        auto t = new std::thread(thread_body);

        while (!running && !failed) {
            using namespace std::literals::chrono_literals;
            std::this_thread::yield();
        }

        if (failed) {
            std::cerr << "Thread failed to start for: " << name << std::endl;
            t->join();
            delete t;
            t = nullptr;
        }

        return t;
}
