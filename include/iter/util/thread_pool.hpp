#ifndef ITER_THREAD_POOL_HPP
#define ITER_THREAD_POOL_HPP

#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace iter {

class ThreadPool {
public:
    // If pool_size < 1, it will be fixed to 1.
    ThreadPool(int pool_size = 1);
    ~ThreadPool();

    // Get the size of thread pool.
    int Size();

    template <class Func, class ...Args>
    auto PushTask(Func&& f, Args&& ...args)
        -> std::future <typename std::result_of <Func(Args...)>::type>;

private:
    int pool_size_;
    bool shutdown_;
    std::vector <std::thread> thread_list_;
    std::queue <std::function <void()> > task_queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
};

ThreadPool::ThreadPool(int pool_size) :
        pool_size_(std::max(pool_size, 1)), shutdown_(false) {
    for (int i = 0; i < pool_size; i++) {
        thread_list_.emplace_back(
            [&] {
                while (1) {
                    std::function <void()> task;
                    { // Critical region.
                        std::unique_lock <std::mutex> lck(mtx_);
                        cv_.wait(lck, [=]{ return shutdown_ || !task_queue_.empty(); });
                        if (shutdown_ && task_queue_.empty()) return;
                        task = std::move(task_queue_.front());
                        task_queue_.pop();
                    }
                    task();
                }
            }
        );
    }
}

ThreadPool::~ThreadPool() {
    { // Critical region.
        std::unique_lock <std::mutex> lck(mtx_);
        shutdown_ = true;
    }
    cv_.notify_all();
    for (auto& t : thread_list_) t.join();
}

int ThreadPool::Size() {
    return pool_size_;
}

template <class Func, class ...Args>
auto ThreadPool::PushTask(Func&& f, Args&& ...args)
    -> std::future <typename std::result_of <Func(Args...)>::type> {
    using return_type = typename std::result_of <Func(Args...)>::type;
    // If thread pool is shutdown, return an empty future object.
    if (shutdown_) return std::future <return_type> ();
    auto task_ptr = std::make_shared <std::packaged_task<return_type()> >(
        std::bind(std::forward <Func> (f), std::forward <Args> (args)...));
    { // Critical region.
        std::unique_lock <std::mutex> lck(mtx_);
        task_queue_.emplace([=](){ (*task_ptr)(); });
    }
    cv_.notify_one();
    return task_ptr->get_future();
}

} // iter

#endif // ITER_THREAD_POOL_HPP
