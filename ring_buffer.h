#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <vector>
#include <atomic>

// 无锁; 单生产者单消费者
// T必须支持默认构造
template<typename T>
class RingBufferSPSC
{
public:
    explicit RingBufferSPSC(const std::size_t bits)
        : capacity_{1U << bits}
        , mask_{capacity_ - 1}
        , buffer_(capacity_) {}
    ~RingBufferSPSC() = default;
    
    // 判满 + 写入
    bool try_push(T item)
    {
        auto head = this->head_.load(std::memory_order_relaxed);
        auto next = (head + 1) & this->mask_;
        auto tail = this->tail_.load(std::memory_order_acquire);
        
        if (next == tail)   // 满
            return false;
        
        this->buffer_[head] = std::move(item);
        this->head_.store(next, std::memory_order_release);
        return true;
    }
    // 判空 + 读取
    bool try_pop(T &item)
    {
        auto tail = this->tail_.load(std::memory_order_relaxed);
        auto next = (tail + 1) & this->mask_;
        auto head = this->head_.load(std::memory_order_acquire);
        
        if (tail == head)   // 空
            return false;
        
        item = std::move(this->buffer_[tail]);
        this->tail_.store(next, std::memory_order_release);
        return true;
    }
    // 仅判空
    bool is_empty()
    {
        auto tail = this->tail_.load(std::memory_order_acquire);
        auto head = this->head_.load(std::memory_order_acquire);
        
        return tail == head;
    }
    // 仅判满
    bool is_full()
    {
        auto head = this->head_.load(std::memory_order_acquire);
        auto next = (head + 1) & this->mask_;
        auto tail = this->tail_.load(std::memory_order_acquire);
        
        return next == tail;
    }
    void clear()
    {
        this->head_.store(0, std::memory_order_release);
        this->tail_.store(0, std::memory_order_release);        
    }
    // 自动覆盖旧数据
    void push(T item)
    {
        auto head = this->head_.load(std::memory_order_relaxed);
        auto next = (head + 1) & this->mask_;
        
        this->buffer_[head] = std::move(item);
        this->head_.store(next, std::memory_order_release);
    }
    // 提前确保非空
    T pop()
    {
        T item;
        
        auto tail = this->tail_.load(std::memory_order_relaxed);
        auto next = (tail + 1) & this->mask_;
        
        item = std::move(this->buffer_[tail]);
        this->tail_.store(next, std::memory_order_release);
        
        return item;
    }

private:
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
    std::size_t capacity_;
    std::size_t mask_;
    std::vector<T> buffer_;
};

#endif // RING_BUFFER_H
