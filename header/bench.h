#pragma once
#include <iostream>
#include <chrono>
#include <vector>  
#include <algorithm> 
#include <string>
#include <x86intrin.h>

namespace internal_lib { 
    
    __attribute__((always_inline))  void compiler_barrier() {
        asm volatile("" ::: "memory");
    }

    // __rdtscp is  a non Serializing instruction means that it means that an instruction does not force the CPU to 
    //execute things in strict, sequential order.

    //  so by using __mm_lfence(); code __mm_lfence(); 
    // __mm_lfence() it completely stops its Out-of-Order engine It forces the CPU to finish every single instruction that came before it
    // and absolutely deny any future instructions from starting until the __rdtscp() is not complete.
    //
    inline uint64_t now_cycle() {
        unsigned int aux;
        _mm_lfence(); // this is to prevent the memory reordering  of the instruction is a hardware fence. It stops the physical CPU pipeline from moving loads across that boundary.
        uint64_t TSC = __rdtscp(&aux);  //_rdtscp is an x86/x64 compiler intrinsic used to read the processor's Time-Stamp Counter (TSC) along with a core/processor ID
        _mm_lfence();  // this is to prevent the memory reordering  of the instruction
        return TSC; 
    }

    inline double get_cycle_per_ns() {
        auto stTime = std::chrono::high_resolution_clock::now();
        
        uint64_t stCycle = now_cycle();

        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - stTime).count() < 50);

        uint64_t endCycle = now_cycle();

        auto endTime =  std::chrono::high_resolution_clock::now();
        
        double timePassed = std::chrono::duration_cast<std::chrono::nanoseconds>(stTime - endTime).count();
        
        return (endCycle - stCycle)/ timePassed;
    }
    
    // cpns is cycle per nanosecond
    void showBenchmark(std::vector<uint64_t> time, double cpns, std::string msg) {
        
        sort(time.begin(), time.end());
        int n = time.size();
        // as time vecto have cycle count not time in ns so 
        auto convert = [&] (uint64_t cycleCount) ->  uint64_t { return (uint64_t)(cycleCount/cpns);};
        
        std::cout << "----------" << msg << "----------" << std::endl;

        std::cout << "P50: " << time[n >> 2] << " ns" << std::endl;
        std::cout << "P75: " << time[n * 0.75]<< " ns" << std::endl;
        std::cout << "P95: " << time[n * 0.95] << " ns" << std::endl;
        std::cout << "P99: " << time[n * 0.99] << " ns" << std::endl;
        
    }
}