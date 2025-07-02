#pragma once
#include<common.h>

template<typename T, typename Impl>
class LockFreeQueue {
public:
    template<typename R>
    void enqueue(R&& item){
        static_cast<Impl*>(this)->enqueue(std::forward<R>(item));
    }
    bool dequeue(T& item){
        return static_cast<Impl*>(this)->dequeue(item);
    }
};

using Capacity_t = std::size_t;
template<typename T, Capacity_t Capacity>
class LockFreeRingBufferVyukov : public LockFreeQueue<T, LockFreeRingBufferVyukov<T, Capacity>>{
private:
    static_assert(Capacity > 0, "Capacity must be greater than 0");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

    const size_t capacity_;
    const size_t mask_;

    struct Cell{
        T data;
        alignas(std::hardware_destructive_interference_size) std::atomic<size_t> sequence;
    };

    std::unique_ptr<Cell[]> buffer_;

    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> enqueue_pos_;
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> dequeue_pos_;    

public:
    LockFreeRingBufferVyukov();
    template<typename U>
    void enqueue(U&& item);
    bool dequeue(T& item);
    LockFreeRingBufferVyukov(const LockFreeRingBufferVyukov&) = delete;
    LockFreeRingBufferVyukov(LockFreeRingBufferVyukov&&) = delete;
    LockFreeRingBufferVyukov& operator=(const LockFreeRingBufferVyukov&) = delete;
    LockFreeRingBufferVyukov& operator=(LockFreeRingBufferVyukov&&) = delete;
};

#include "ring_buffer_vyukov.tpp"