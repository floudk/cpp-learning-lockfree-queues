template<typename T, Capacity_t Capacity>
LockFreeRingBufferVyukov<T, Capacity>::LockFreeRingBufferVyukov():
    capacity_(Capacity),
    mask_(Capacity - 1),
    enqueue_pos_(0),
    dequeue_pos_(0),
    buffer_(std::make_unique<Cell[]>(capacity_)){
        for(size_t i = 0; i < capacity_; i++){
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }
    
template<typename T, Capacity_t Capacity>
template<typename U>
void LockFreeRingBufferVyukov<T, Capacity>::enqueue(U&& item){
    Cell* cell;
    size_t pos = enqueue_pos_.fetch_add(1, std::memory_order_relaxed);
    cell = &buffer_[pos & mask_];
    while(1){
        size_t seq = cell->sequence.load(std::memory_order_acquire);
        if (seq == pos)
            break;
        std::this_thread::yield(); // Yield to avoid busy waiting
    }
    cell->data = std::forward<U>(item);
    cell->sequence.store(pos + 1, std::memory_order_release);
}



template<typename T, Capacity_t Capacity>
bool LockFreeRingBufferVyukov<T, Capacity>::dequeue(T& item){
    Cell* cell;
    size_t pos = dequeue_pos_.fetch_add(1, std::memory_order_relaxed);
    cell = &buffer_[pos & mask_];
    while(1){
        size_t seq = cell->sequence.load(std::memory_order_acquire);
        if (seq == pos + 1)
            break;
        std::this_thread::yield(); // Yield to avoid busy waiting
    }
    item = std::move(cell->data);
    cell->sequence.store(pos + capacity_, std::memory_order_release);
    return true;
}