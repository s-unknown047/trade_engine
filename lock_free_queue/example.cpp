#pragma once
#include "../header/Thread.h"
#include "../header/lock_free_queue.h"  
#include <iostream>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)  


struct MyStruct {
    int d_[3];
};

using namespace Common;

auto consumeFunc(LFQueue<MyStruct> *lfq, bool *flag) noexcept
{
    
    while (!(*flag));// Wait until the producer has started producin
      
    while (lfq->size())
    {
        const auto d = lfq->getNextToRead();
        lfq->updateReadIndex();
        std::cout << "Consumed: " << d->d_[0] << ", " << d->d_[1] << ", " << d->d_[2] <<" size "<<lfq->size()<< std::endl;

    }

   
    std::cout << "Consumer is done consuming" << std::endl;
}

auto inline producerFunc(LFQueue<MyStruct> *lfq, bool *flag) noexcept
{
    // Set the flag to true to indicate that the producer has started producing
   
    for (int i = 0; i < 100; ++i)
    { 
        // Set the flag to true to indicate that the producer has started producing
         
        const MyStruct d{i, i * 10, i * 100};
        *(lfq->getNextToWriteTo()) = d;
        lfq->updateWriteIndex();

        std::cout << "Producer constructed elem:" << d.d_[0] << "," << d.d_[1] << "," << d.d_[2] << " lfq-size:" << lfq->size() << std::endl;
        if (UNLIKELY(*flag == false)) *flag = true;
      
    }

    std::cout<< "Producer is done producing" << std::endl;
}

int main(int, char **)
{
    LFQueue<MyStruct> lfq(100);
    bool flag = false; 
    auto ct = createAndStartThread(-1, "this is first thread", consumeFunc, &lfq, &flag);   
    auto pt = createAndStartThread(-1, "this is producer thread", producerFunc, &lfq, &flag);

    pt->join();
    std::cout << "main is done producing" << std::endl;
    ct->join();
    std::cout << "main is done consuming" << std::endl;
}
