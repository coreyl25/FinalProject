#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::atomic<int> active_count;
    
    int pool_size;

public:
    ThreadPool(int size);
    ~ThreadPool();
    
    void enqueue(std::function<void()> task);
    int get_active_count() const;
    int get_pool_size() const;
    
private:
    void worker_thread();
};

#endif // THREAD_POOL_H