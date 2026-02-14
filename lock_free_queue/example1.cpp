#pragma once
#include "../header/Thread.h"
#include "../header/lock_free_queue.h"  
#include <iostream>
#include <atomic>
#include <chrono>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)  


// while using normal variable i am getting that consumer finish at last but using atomic flag consumer finish before producer which is not 
struct MyStruct {
    int d_[3];
};

using namespace Common;

auto consumeFunc(LFQueue<MyStruct> *lfq, std::atomic<bool> *flag) noexcept
{
    
    while (!(flag->load()));
    
      
    while (lfq->size())
    {
        const auto d = lfq->getNextToRead();
        lfq->updateReadIndex();
        std::cout << "Consumed: " << d->d_[0] << ", " << d->d_[1] << ", " << d->d_[2] <<" size "<<lfq->size()<<"Fuck offf fofff ofofofofofofoofofofoofofofoofofofofofofo"<< std::endl;

    }
   
    std::cout << "Consumer is done consuming" << std::endl;
}

auto inline producerFunc(LFQueue<MyStruct> *lfq, std::atomic<bool> *flag) noexcept
{
    

    for (int i = 0; i < 100000; ++i)
    { 
        const MyStruct d{i, i * 10, i * 100};
        *(lfq->getNextToWriteTo()) = d;
        lfq->updateWriteIndex();

        std::cout << "Producer constructed elem:" << d.d_[0] << "," << d.d_[1] << "," << d.d_[2] << " lfq-size:" << lfq->size() << std::endl;
        // check flag condition 
        if (UNLIKELY(!flag->load())) flag->store(true);
      
    }

    std::cout<< "8888888888888888888888888888888888888888888 Producer is done producing 88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888" << std::endl;
}

int main(int, char **)
{   
    LFQueue<MyStruct> lfq(5000000);
    std::atomic<bool> flag(false); 
    auto pt = createAndStartThread(1, "this is producer thread", producerFunc, &lfq, &flag);
    auto ct = createAndStartThread(2, "this is first thread", consumeFunc, &lfq, &flag);   



    pt->join();
    std::cout << "main is done producing" << std::endl;
    ct->join();
    std::cout << "main is done consuming" << std::endl;

}

// #pragma once
// #include "../header/Thread.h"
// #include "../header/lock_free_queue.h"
// #include <iostream>
// #include <atomic>

// #define LIKELY(x) __builtin_expect(!!(x), 1)
// #define UNLIKELY(x) __builtin_expect(!!(x), 0)

// struct MyStruct {
//     int d_[3];
// };

// using namespace Common;

// class ProducerConsumer {
// private:
//     LFQueue<MyStruct> lfq_;
//     std::atomic<bool> flag_; 

// public:
//     explicit ProducerConsumer(size_t queue_size) : lfq_(queue_size), flag_(false) {}

//     void producerFunc() noexcept {
//         for (int i = 0; i < 100; ++i) {
//             const MyStruct d{i, i * 10, i * 100};
//             *(lfq_.getNextToWriteTo()) = d;
//             lfq_.updateWriteIndex();

//             std::cout << "Producer constructed elem: " << d.d_[0] << ", " << d.d_[1] << ", " << d.d_[2]
//                       << " lfq-size: " << lfq_.size() << std::endl;

//             if (UNLIKELY(!flag_.load())) {
//                 flag_.store(true);
//             }
//         }

//         std::cout << "Producer is done producing" << std::endl;
//     }

//     void consumeFunc() noexcept {

//         while (!flag_.load());

//         while (lfq_.size()) {
//             while ();
//             const auto d = lfq_.getNextToRead();
//             lfq_.updateReadIndex();
//             std::cout << "Consumed: " << d->d_[0] << ", " << d->d_[1] << ", " << d->d_[2]
//                       << " size: " << lfq_.size() << std::endl;
//         }

//         std::cout << "Consumer is done consuming" << std::endl;
//     }

//     auto getProducerFunc() {
//         return [this]() { producerFunc(); };
//     }

//     auto getConsumerFunc() {
//         return [this]() { consumeFunc(); };
//     }
// };

// int main(int, char **) {

//     ProducerConsumer pc(500);

//     auto ct = createAndStartThread(-1, "Consumer Thread", pc.getConsumerFunc());
//     auto pt = createAndStartThread(-1, "Producer Thread", pc.getProducerFunc());

//     pt->join();
//     std::cout << "Main is done producing" << std::endl;
//     ct->join();
//     std::cout << "Main is done consuming" << std::endl;

//     return 0;
// }