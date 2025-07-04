template<typename T, Capacity_t Capacity>
class SPMCQueue : public LockFreeQueue<T, SPMCQueue<T, Capacity>> {
    static_assert(Capacity > 0, "Capacity must be > 0");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

private:
    struct Cell {
        std::atomic<size_t> sequence;
        T data;
    };

    std::unique_ptr<Cell[]> buffer_;
    static constexpr size_t mask_ = Capacity - 1;
    alignas(64) std::atomic<size_t> enqueue_pos_;
    alignas(64) std::atomic<size_t> dequeue_pos_;

public:
    SPMCQueue()
        : buffer_(new Cell[Capacity]),
          enqueue_pos_(0),
          dequeue_pos_(0)
    {
        for (size_t i = 0; i < Capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    template<typename R>
    bool enqueue(R&& data) {
        Cell* cell;
        size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
        cell = &buffer_[pos & mask_];
        size_t seq = cell->sequence.load(std::memory_order_relaxed);
        size_t dif = seq - pos;
        if (dif !=0){
            return false;
        }
        cell->data = std::forward<R>(data);
        cell->sequence.store(pos + 1, std::memory_order_release);
        enqueue_pos_.store(pos + 1, std::memory_order_relaxed);
        return true;
    }

    bool dequeue(T& data) {
        Cell* cell;
        size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        for(;;){
            cell = &buffer_[pos & mask_];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            std::intptr_t dif =static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos) - 1;
            if (dif == 0){
                if (dequeue_pos_.compare_exchange_weak(
                        pos, pos + 1,
                        std::memory_order_acquire,
                        std::memory_order_relaxed
                    )) {
                        data = std::move(cell->data);
                        cell->sequence.store(pos + Capacity, std::memory_order_release);
                        return true;
                    }
                }
                continue;
            }else if (dif < 0){
                return false;
            }else{
                pos = dequeue_pos_.load(std::memory_order_relaxed);
            }
        }
    }