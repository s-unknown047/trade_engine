#include <cstdint>
#include <vector>
#include <string>

#include "../header/macros.h"

namespace common
{
    template <typename T> // this is a memory pool for objects of type T
    class MemPool final   // final keyword to prevent inheritance
    {
    private:
        struct ObjectBlock // this struct represent elements in the memory pool
        {
            T object_;
            bool is_free = true;
        };
        std::vector<ObjectBlock> store_; // the actual storage for the memory pool
        std::vector<size_t> free_index_;

        size_t high_ind = 0;
        size_t capacity;
        size_t next_free_index_ = 0;

    public:
        MemPool() = delete; // We do this so that these methods are not accidentally called without our knowledge
        MemPool(const MemPool &) = delete;
        MemPool(MemPool &&) = delete;
        MemPool &operator=(const MemPool &) = delete;
        MemPool &operator=(MemPool &&) = delete;

        // constructor that initializes the memory pool with num_elems elements and it is explicit to prevent implicit conversions and implicit calling by compiler
        // this is important to avoid accidental creation of memory pools with unintended sizes
        // we also call the constructor of store_ with num_elems and a default ObjectBlock to preallocate the memory for the pool
        // num_elems: number of elements in the memory pool
        // T(): default constructor of T to initialize the object_ member of ObjectBlock

        explicit MemPool(std::size_t num_elems) : store_(num_elems, ObjectBlock{T(), true}), capacity(num_elems);
        {
            free_index_.resize(num_elems);
            // ensure that the T object is the first member of ObjectBlock to guarantee proper alignment and memory layout
            ASSERT(reinterpret_cast<const ObjectBlock *>(&(store_[0].object_)) == &(store_[0]), "T object should be the first member of ObjectBlock ");
        }

        
        // template <typename... Args>
        // T *allocate(Args &&...args) noexcept
        // {   
        //     auto obj_block = &(store_[next_free_index_]);
        //     ASSERT(obj_block->is_free, "Expected free ObjectBlock at index " + std::to_string(next_free_index_));

        //     // ret is a pointer to the object_ member of the free space in store heap
        //     T *ret = &(obj_block->object_);
        //     // this is syntax to store value of type T in the address ret it did not new (address) Type(constructor_args...)
        //     // allocate an object from the memory pool and construct it with the provided arguments
        //     obj_block->is_free = false;
        //     updateNextFreeIndex();
        //     ret = new (ret) T *(args...); // placement new to construct the object in place

        //     return ret;
        // }

        template <typename... Args>
        inline T *allocate(Args&&... args) noexcept{
            size_t index;

            if (!free_index_.empty()) {
                index = free_index.back();
                free_index.pop_back();
            } else if (LIKELY(high_water_mark < capacity)) {
                index = high_water_mark;
            } else {
                std::cerr << "No space left in memory" << std::endl;
                std::terminate();
            }

            ObjectBlock* block = &store[index];
            block->is_free = false;

            T* ptr = &(block->object_);
            new(ptr) T(std::forward<Args>(args)...);
            return ptr;
        }

    private:
        // this method updates the next_free_index_ to point to the next free ObjectBlock in the store vector
        // auto updateNextFreeIndex() noexcept
        // {

        //     const auto initial_free_index = next_free_index_;
        //     while (!store_[next_free_index_].is_free_)
        //     {
        //         ++next_free_index_;

        //         if (UNLIKELY(next_free_index_ == store_.size()))
        //         {
        //             next_free_index_ = 0;
        //         }

        //         if (UNLIKELY(initial_free_index == next_free_index_))
        //         {
        //             ASSERT(initial_free_index != next_free_index_, "Memory Pool out of Space");
        //         }
        //     }
        // }

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

            free_index_.push_back(index);
        }
    };
}

// this memeory pool have a major problem that we have to find the next free index which take o(n) when vector is almost full so make time complexity unpridictable.
// so we need to find a solution to this problem that is creating a vector know as free index that store index of free space
// when we deallocate we add that index into vector stack when allocate we remove that index from the freeList 
// we use a hybrid approach we create a varible high_mark and a free list 
// at startup the free list is empty when need memory, we just  increment the high water mark instance
// when we deallocate then only push element to free list 
