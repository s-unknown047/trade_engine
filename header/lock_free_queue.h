#include <atomic>
#include <vector>
#include <iostream>
#include "../header/macros.h"

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0) 

namespace Common
{
    template <typename T>
    class LFQueue final
    {
    private:
        std::vector<T> store_;
        std::atomic<size_t> next_write_index_ = {0};
        std::atomic<size_t> next_read_index_ = {0};
        std::atomic<size_t> num_elements_ = {0};

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

        auto updateWriteIndex() noexcept
        {   
            ASSERT((next_write_index_ + 1) % store_.size() != next_read_index_, "Write is Invalid " + std::to_string(pthread_self()));
            next_write_index_ = (next_write_index_ + 1) % store_.size();
            num_elements_++;
        }

        auto getNextToRead() noexcept
        {
            return (next_read_index_ == next_write_index_) ? nullptr : &store_[next_read_index_];
        }

        auto updateReadIndex() noexcept
        {
            next_read_index_ = (next_read_index_ + 1) % store_.size();
            ASSERT(num_elements_ != 0, " Read is Invalid " + std::to_string(pthread_self()));
            num_elements_--;
        }

        auto size() const noexcept
        {
            return num_elements_.load();
        }
    };
};
