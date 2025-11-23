#ifndef THREAD_SAFE_SET_HPP
#define THREAD_SAFE_SET_HPP

#include <unordered_set>
#include <mutex>
#include <string>

// A thread-safe set for storing visited URLs.
class ThreadSafeSet {
public:
    // Attempts to insert a URL into the set.
    // Returns true if insertion occurred (URL was not present).
    // Returns false if the URL was already present.
    bool insert(const std::string& url) {
        std::lock_guard<std::mutex> lock(mut); // Lock the mutex
        // try_emplace returns a pair: iterator and bool (true if inserted)
        return visited_urls.insert(url).second;
    } // Mutex is automatically unlocked here

    // Checks if a URL is present in the set (thread-safe).
    bool contains(const std::string& url) const {
        std::lock_guard<std::mutex> lock(mut);
        return visited_urls.count(url) > 0;
    }

    // Returns the number of items in the set (thread-safe).
    size_t size() const {
        std::lock_guard<std::mutex> lock(mut);
        return visited_urls.size();
    }

private:
    std::unordered_set<std::string> visited_urls;
    mutable std::mutex mut; // Mutex to protect the set
};

#endif // THREAD_SAFE_SET_HPP
