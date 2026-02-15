#include <chrono>
#include <iostream>
#include <thread>
using namespace std;

int main() {

    auto start = std::chrono::high_resolution_clock::now();
    this_thread::yield();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double,std::nano> interval = end - start;

    std::cout << "Time taken: " << interval.count() << " nanoseconds" << std::endl;

     return 0;
}