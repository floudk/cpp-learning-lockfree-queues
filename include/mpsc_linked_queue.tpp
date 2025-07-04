template<typename T>
class MPSCLinkedQueue : public LockFreeQueue<T, MPSCLinkedQueue<T>> {
private:
    struct Node{
        alignas(std::hardware_destructive_interference_size) std::atomic<Node*> next_{nullptr};
        T data_;
        Node():next_(nullptr){}
        template<typename U>
        Node(U&& data):data_(std::forward<U>(data)),next_(nullptr){}
    };

    alignas(std::hardware_destructive_interference_size) std::atomic<Node*> head_;
    Node* tail_;

public:
    MPSCLinkedQueue(){
        Node* stub = new Node();
        head_.store(stub, std::memory_order_relaxed);
        tail_ = stub;
    }

    ~MPSCLinkedQueue(){
        T tmp;
        while(dequeue(tmp)) {};
        delete tail_;
    }

    template<typename U>
    bool enqueue(U&& item){
        Node* node = new Node(std::forward<U>(item));
        Node* prev = head_.exchange(node, std::memory_order_acq_rel);
        prev->next_.store(node, std::memory_order_release);
        return true;
    }

    bool dequeue(T& item){
        Node* tail = tail_;
        Node* next =  tail->next_.load(std::memory_order_acquire);
        if (next == nullptr){
            return false;
        }
        item = std::move(next->data_);
        tail = next;
        delete tail;
        return true;
    }

};