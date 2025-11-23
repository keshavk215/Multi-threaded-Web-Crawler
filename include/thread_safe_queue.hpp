#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional> // Requires C++17

// A thread-safe queue for storing URLs to be crawled.
template <typename T>
class ThreadSafeQueue {
public:
    // Adds an item to the back of the queue.
    void push(T item) {
        std::lock_guard<std::mutex> lock(mut); // Lock the mutex
        queue.push(item);
        cond.notify_one(); // Notify one waiting thread (if any)
    } // Mutex is automatically unlocked when lock goes out of scope

    // Attempts to remove and return an item from the front of the queue.
    // Waits if the queue is empty.
    // Returns std::nullopt if the queue is signaled to stop.
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mut); // Use unique_lock for condition variable
        // Wait until the queue is not empty OR stop_requested is true
        cond.wait(lock, [this] { return !queue.empty() || stop_requested; });

        // If stop was requested and queue is empty, return nullopt
        if (stop_requested && queue.empty()) {
            return std::nullopt;
        }

        // Retrieve the item
        T item = queue.front();
        queue.pop();
        return item;
    }

    // Signals the queue to stop processing and wakes up waiting threads.
    void request_stop() {
        std::lock_guard<std::mutex> lock(mut);
        stop_requested = true;
        cond.notify_all(); // Wake up all waiting threads
    }

    // Checks if the queue is empty (thread-safe).
    bool empty() const {
        std::lock_guard<std::mutex> lock(mut);
        return queue.empty();
    }

private:
    std::queue<T> queue;
    mutable std::mutex mut; // Mutex to protect the queue
    std::condition_variable cond; // Condition variable for waiting
    bool stop_requested = false; // Flag to signal stopping
};

#endif // THREAD_SAFE_QUEUE_HPP
