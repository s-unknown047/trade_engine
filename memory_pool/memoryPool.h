#include <cstdint>
#include <vector>
#include <string>

#include "../header/macros.h"

namespace common
{
   template <typename T>  // this is a memory pool for objects of type T
    class MemPool final      // final keyword to prevent inheritance
    {
    private:
        struct ObjectBlock   // this struct represent elements in the memory pool
        {
            T object_;
            bool is_free = true;
        };
        std::vector<ObjectBlock> store_;  // the actual storage for the memory pool
        size_t next_free_index_ = 0;

    public:
        MemPool() = delete;  // We do this so that these methods are not accidentally called without our knowledge
        MemPool(const MemPool &) = delete;
        MemPool(MemPool &&) = delete;
        MemPool &operator=(const MemPool &) = delete;
        MemPool &operator=(MemPool &&) = delete;

        // constructor that initializes the memory pool with num_elems elements and it is explicit to prevent implicit conversions and implicit calling by compiler
        // this is important to avoid accidental creation of memory pools with unintended sizes
        // we also call the constructor of store_ with num_elems and a default ObjectBlock to preallocate the memory for the pool
        // num_elems: number of elements in the memory pool
        // T(): default constructor of T to initialize the object_ member of ObjectBlock

        explicit MemPool(std::size_t num_elems) : store_(num_elems, ObjectBlock{T(), true})
        {
            // ensure that the T object is the first member of ObjectBlock to guarantee proper alignment and memory layout
            
            ASSERT(reinterpret_cast<const ObjectBlock *>(&(store_[0].object_)) == &(store_[0]), "T object should be the first member of ObjectBlock ");
        }

        template <typename... Args>
        T *allocate(Args... args) noexcept
        {
            auto obj_block = &(store_[next_free_index_]);
            ASSERT(obj_block->is_free, "Expected free ObjectBlock at index " + std::to_string(next_free_index_));

            // ret is a pointer to the object_ member of the free space in store heap
            T *ret = &(obj_block->object_);
            // this is syntax to store value of type T in the address ret it did not new (address) Type(constructor_args...)
            // allocate an object from the memory pool and construct it with the provided arguments
            ret = new (ret) T *(args...); // placement new to construct the object in place
            obj_block->is_free = false;
            updateNextFreeIndex();

            return ret;
        }

    private:
        // this method updates the next_free_index_ to point to the next free ObjectBlock in the store vector
        auto updateNextFreeIndex() noexcept
        {

            const auto initial_free_index = next_free_index_;
            while (!store_[next_free_index_].is_free_)
            {
                ++next_free_index_;

                if (UNLIKELY(next_free_index_ == store_.size()))
                {
                    next_free_index_ = 0;
                }

                if (UNLIKELY(initial_free_index == next_free_index_))
                {
                    ASSERT(initial_free_index != next_free_index_, "Memory Pool out of Space");
                }
            }
        }

        // dellocate an object from the memory pool and mark the corresponding object block as free
        auto dealloctate(const T *elem) noexcept
        {

            // we calculate the index of the element being deallocated by taking the address of the element and subtracting the address of the first element in the store vector
            const auto elem_index = reinterpret_cast<ObjectBlock *>(elem) - &(store_[0]);

            // we check that the calculated index is valid and that the corresponding object block is not already free before marking it as free
            ASSERT(elem_index >= 0 && static_cast<size_t>(elem_index), "Element being deallocated does not belong to this memory pool");
            ASSERT(!stor_[elem_index].is_free_, "Element being deallocated is already free");
            // we mark the corresponding object block as free by setting its is_free_ member to true
            store_[elem_index].is_free_ = true;
        }
    };
}