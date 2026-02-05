// #pragma once

#include <thread>
#include <atomic>
#include <unistd.h>
#include <iostream>
#include <sys/syscall.h>
#include <utility> // For std::forward

namespace Common {
  /// Set affinity for current thread to be pinned to the provided core_id.
  inline auto setThreadCore(int core_id) noexcept {
    cpu_set_t cpuset;  // defne an array/set  of CPUs

    CPU_ZERO(&cpuset); // initialize the CPU set to be empty
    CPU_SET(core_id, &cpuset); // add the specified core_id to the CPU set
// set affinity of the calling thread to the CPUs in the cpuset
    return (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0);
  }
}

  //Creates a thread instance, sets affinity on it, assigns it a name and

// this is a template function that can take any callable type T and any number of arguments Args


//  T &&func is a function pointer or callable object to be executed in the new thread
//  Args &&...args are the arguments to be passed to the function when it is called
template <typename T, typename... Args>
inline auto createAndStartThread(int core_id, const std::string &name, T &&func, Args &&...args) noexcept
{

    std::cout<< "Thread core_id " << core_id << std::endl;

    std::atomic<bool> running(false), failed(false); // Use std::atomic
    auto thread_body = [&]
    {    
        if (core_id >= 0 && !Common::setThreadCore(core_id))
        {
            std::cerr << "Failed to set affinity core to " << name << " for thread " << pthread_self() << " " << core_id << std::endl;
            failed = true;
            return;
        }

        std::cout << "set core affinity for " << name << " " << pthread_self() << " to " << core_id << std::endl;

        running = true;
        std::forward<T>(func)(std::forward<Args>(args)...); // Use std::forward
    };

    auto t = new std::thread(thread_body);

    while(!running && !failed) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);
    }

    if (failed) {
        t->join();

        delete t;
        t = nullptr;
    }

    return t;
}



// #pragma once

// #include <iostream>
// #include <atomic>
// #include <thread>
// #include <unistd.h>

// #include <sys/syscall.h>

// namespace Common
// {
//   /// Set affinity for current thread to be pinned to the provided core_id.
//   inline auto setThreadCore(int core_id) noexcept
//   {
//     // array of cpu core there are 1024 cores  it a set of 1024 / __CPU__BITS  in the system and each bit in the cpuset represents a core, if the bit is set to 1 it means the thread can run on that core if it is set to 0 it means the thread cannot run on that core
//     cpu_set_t cpuset;
//     // initialize the CPU set to be empty this is a macro that sets all bits in the cpuset to 0
//     CPU_ZERO(&cpuset);
//     // this sets the bit corresponding to core_id in the cpuset to 1 this is a macro that takes the core_id and the cpuset and sets the bit corresponding to core_id in the cpuset to 1
//     CPU_SET(core_id, &cpuset);
//     // pthread_setaffinity_np is a funnction to set affinity of the callling thread to the cpu in the cpuset it takes the thread id of the calling thread which is pthread_self() and the size of the cpuset and the cpuset itself and returns 0 on success and non zero on failure
//     return (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0);
//   }

//   // Creates a thread instance, sets affinity on it, assigns it a name and
//   // passes the function to be run on that thread as well as the arguments to the function.
//   template <typename T, typename... A>
//   inline auto createAndStartThread(int core_id, const std::string &name, T &&func, A &&...args) noexcept
//   {
//     auto t = new std::thread([&]()
//                              {
//       if (core_id >= 0 && !setThreadCore(core_id)) {
//         std::cerr << "Failed to set core affinity for " << name << " " << pthread_self() << " to " << core_id << std::endl;
//         exit(EXIT_FAILURE);
//       }
//       std::cerr << "Set core affinity for " << name << " " << pthread_self() << " to " << core_id << std::endl;

//       std::forward<T>(func)((std::forward<A>(args))...); });

//     using namespace std::literals::chrono_literals;
//     std::this_thread::sleep_for(1s);

//     return t;
//   }
// }