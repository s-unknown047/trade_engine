#pragma once
#include <atomic>
#include <vector>
#include <iostream>
#include "macros.h"
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0) 

namespace Common
{
    template <typename T> 
    class LFQueue 
    {
    private:
        std::vector<T> store_;
        std::atomic<size_t> next_write_index_ = {0};
        std::atomic<size_t> next_read_index_ = {0};

    public:
        LFQueue(std::size_t num_elems) : store_(num_elems, T()) {}

        LFQueue() = delete;
        LFQueue(const LFQueue &) = delete;
        LFQueue(const LFQueue &&) = delete;
        LFQueue &operator=(const LFQueue &) = delete;
        LFQueue &operator=(const LFQueue &&) = delete;

        auto getNextToWriteTo() noexcept
        {
            return (next_write_index_ + 1) % store_.size() == next_read_index_ ? nullptr : &store_[next_write_index_];
        }

        auto  updateWriteIndex() noexcept
        {   
            ASSERT((next_write_index_ + 1) % store_.size() != next_read_index_, "Write is Invalid " + std::to_string(pthread_self()));
            next_write_index_ = (next_write_index_ + 1) % store_.size();
        }

        auto getNextToRead() noexcept
        {
            return (next_read_index_ == next_write_index_) ? nullptr : &store_[next_read_index_];
        }

        auto updateReadIndex() noexcept
        {
            ASSERT(next_read_index_ != next_write_index_, " Read is Invalid " + std::to_string(pthread_self()));
            next_read_index_ = (next_read_index_ + 1) % store_.size();
        }

        auto size() const noexcept
        {
            auto w = next_write_index_.load(std::memory_order_relaxed);
            auto r = next_read_index_.load(std::memory_order_relaxed);
            if (w >= r) return w - r;
            return store_.size() - r + w;
        }
    };
};
