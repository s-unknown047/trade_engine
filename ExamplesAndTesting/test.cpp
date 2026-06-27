#include <x86intrin.h>
#include <chrono>
#include <iostream>

    inline uint64_t now_cycle() {
        unsigned int aux;
        _mm_lfence(); // this is to prevent the memory reordering  of the instruction
        uint64_t TSC = __rdtscp(&aux);  //_rdtscp is an x86/x64 compiler intrinsic used to read the processor's Time-Stamp Counter (TSC) along with a core/processor ID
        _mm_lfence();  // this is to prevent the memory reordering  of the instruction
        return TSC; 
    }

    inline double get_cycle_per_ns() {
        auto stTime = std::chrono::high_resolution_clock::now();
        
        uint64_t stCycle = now_cycle();

        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - stTime).count() < 200);

        uint64_t endCycle = now_cycle();

        auto endTime =  std::chrono::high_resolution_clock::now();
        
        double timePassed = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - stTime).count();
        
        return (endCycle - stCycle)/ timePassed;
    }

    int main() {
        std::cout << get_cycle_per_ns();
    }
