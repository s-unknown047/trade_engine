#pragma once
#include <cstring>
#include <iostream>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)  
// Branch prediction hints for the compiler 
// Usage: if (LIKELY(condition)) { ... } else { ... }
// The compiler will optimize for the case where condition is true  

inline auto ASSERT(bool cond, const std::string &msg) noexcept {
    if (UNLIKELY(!cond)) {
        std::cerr << "Assertion failed!" << msg <<std::endl;
        exit(EXIT_FAILURE); 
    }
}

inline auto FATAL(const std::string &msg) noexcept {
    std::cerr << "Fatal error: " << msg << std::endl;
    exit(EXIT_FAILURE);
}