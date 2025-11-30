#include "thread_pool.h"
#include <iostream>

ThreadPool::ThreadPool(int size) : pool_size(size), stop(false), active_count(0) {
    for (int i = 0; i < pool_size; ++i) {
        workers.emplace_back(&ThreadPool::worker_thread, this);
    }
    std::cout << "[ThreadPool] Created with " << pool_size << " worker threads" << std::endl;
}

ThreadPool::~ThreadPool() {
    stop = true;
    condition.notify_all();
    
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    std::cout << "[ThreadPool] All workers terminated" << std::endl;
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(task);
    }
    condition.notify_one();
}

int ThreadPool::get_active_count() const {
    return active_count.load();
}

int ThreadPool::get_pool_size() const {
    return pool_size;
}

void ThreadPool::worker_thread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            
            if (stop && tasks.empty()) {
                return;
            }
            
            if (!tasks.empty()) {
                task = std::move(tasks.front());
                tasks.pop();
            }
        }
        
        if (task) {
            active_count++;
            task();
            active_count--;
        }
    }
}