# Multi-threaded Web Crawler in C++

A high-performance, concurrent web crawler built from the ground up in C++ to efficiently scrape and index websites. This project demonstrates a deep understanding of multi-threading, synchronization primitives, and low-level resource management.

## Key Features

- **Multi-threaded Architecture**: Utilizes multiple worker threads to fetch and process web pages in parallel, maximizing I/O throughput.
- **Thread-Safe Queue**: Implements a blocking, thread-safe queue using C++ STL mutexes and condition variables to manage the list of URLs to be crawled.
- **Duplicate URL Prevention**: Employs a shared, thread-safe set to keep track of visited URLs, preventing redundant work and crawl loops.
- **Robots.txt Compliance**: Includes a module to fetch and respect `robots.txt` politeness policies before crawling a domain.
- **HTML Parsing**: Uses an external library (Gumbo-parser) to parse HTML content and extract new hyperlinks for the queue.

## Tech Stack

- **Language**: C++17
- **Core Libraries**: C++ Standard Library (STL) (`<thread>`, `<mutex>`, `<condition_variable>`)
- **Networking**: `libcurl` for making HTTP requests
- **Parsing**: `gumbo-parser` for HTML5 parsing
- **Build System**: `CMake`

## Architecture Overview

The crawler operates with a producer-consumer pattern. A main thread initializes the process and seeds a thread-safe queue with starting URLs. Multiple worker threads then run concurrently:

1. **Dequeue**: A worker thread locks the queue, dequeues a URL, and releases the lock. If the queue is empty, the thread waits on a condition variable.
2. **Fetch**: It uses `libcurl` to download the HTML content of the URL.
3. **Parse**: It uses `gumbo-parser` to extract all valid `href` links from the page.
4. **Enqueue**: For each new link found, the worker thread locks the queue and the visited set, and if the URL has not been visited, it enqueues it and adds it to the visited set.

This process continues until the queue is empty and all threads are idle.

## Setup and Installation (Placeholder)

1. Clone the repository and its submodules (if any).
2. Create a build directory: `mkdir build && cd build`
3. Run CMake to configure the project: `cmake ..`
4. Compile the project: `make`

## Usage (Placeholder)

`./crawler <start-url> <num-threads>`
