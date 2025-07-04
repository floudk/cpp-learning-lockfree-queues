template<typename T, Capacity_t Capacity>
class Kfifo : public LockFreeQueue<T, Kfifo<T, Capacity>>{
private:
    static_assert(Capacity > 0, "Capacity must be greater than 0");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
    
    static constexpr size_t capacity_ = Capacity;
    static constexpr size_t mask_ = Capacity - 1;

    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> enqueue_pos_{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> dequeue_pos_{0};

    std::unique_ptr<T[]> buffer_;
    bool full() const;
    bool empty() const;
    size_t size() const;
public:
    Kfifo();
    template<typename U>
    bool enqueue(U&& item);
    bool dequeue(T& item);
};


template<typename T, Capacity_t Capacity>
Kfifo<T, Capacity>::Kfifo():
    buffer_(std::make_unique<T[]>(capacity_)){}

template<typename T, Capacity_t Capacity>
bool Kfifo<T, Capacity>::full() const{
    return enqueue_pos_.load(std::memory_order_relaxed) - dequeue_pos_.load(std::memory_order_relaxed) >= capacity_;
}

template<typename T, Capacity_t Capacity>
bool Kfifo<T, Capacity>::empty() const{
    return enqueue_pos_.load(std::memory_order_acquire) == dequeue_pos_.load(std::memory_order_relaxed);
}

template<typename T, Capacity_t Capacity>
size_t Kfifo<T, Capacity>::size() const{
    return enqueue_pos_.load(std::memory_order_relaxed) - dequeue_pos_.load(std::memory_order_relaxed);
}

template<typename T, Capacity_t Capacity>
template<typename U>
bool Kfifo<T, Capacity>::enqueue(U&& item){
    if(full())
        return false;
    size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
    buffer_[pos & mask_] = std::forward<U>(item);
    enqueue_pos_.store(pos+1, std::memory_order_release);
    return true;
}

template<typename T, Capacity_t Capacity>
bool Kfifo<T, Capacity>::dequeue(T& item){
    if(empty())
        return false;
    size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
    item = std::move(buffer_[pos & mask_]);
    dequeue_pos_.store(pos+1, std::memory_order_release);
    return true;
}