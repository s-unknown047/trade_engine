#pragma once
#include <iostream>
#include <chrono>
#include <vector>  
#include <algorithm> 
#include <string>
  

void showBenchmark(std::vector<std::chrono::duration<double,std::nano>>& time, std::string msg) {
    
    sort(time.begin(), time.end());

    std::cout<< msg <<" time median: " << time[time.size() / 2].count() << " ns" << std::endl;
    std::cout<< msg <<" time 0.75: " << time[time.size() * 0.75].count() << " ns" << std::endl;
    std::cout << msg << " time 0.95: " << time[time.size() * 0.95].count() << " ns" << std::endl;
    std::cout << msg << " time 0.99: " << time[time.size() * 0.99].count() << " ns" << std::endl;
}