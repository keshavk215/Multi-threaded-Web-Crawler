
# Multi-threaded Web Crawler in C++

A high-performance, concurrent web crawler built from the ground up in C++ to efficiently scrape and index websites within a given domain. This project demonstrates a deep understanding of multi-threading, synchronization primitives (mutexes, condition variables), low-level resource management, and core networking/parsing concepts using external libraries.

## Key Features

* **Multi-threaded Architecture** : Utilizes `std::thread` to create multiple worker threads that fetch and process web pages in parallel, maximizing network I/O throughput.
* **Thread-Safe Queue** : Implements a blocking, thread-safe queue (`ThreadSafeQueue.hpp`) using `std::mutex` and `std::condition_variable` to manage the list of URLs to be crawled, preventing race conditions and ensuring efficient thread waiting.
* **Duplicate URL Prevention** : Employs a shared, thread-safe set (`ThreadSafeSet.hpp`) using `std::mutex` to keep track of visited URLs, preventing redundant work and crawl loops.
* **HTTPS & Redirects** : Uses `libcurl` for making robust HTTPS requests, automatically following server-side redirects.
* **HTML Parsing & Link Extraction** : Uses `gumbo-parser` to parse HTML5 content and accurately extract all valid hyperlinks (`<a>` tags).
* **Relative URL Resolution** : Includes basic logic to resolve relative URLs (e.g., `/about`, `page.html`) into absolute URLs based on the current page's URL.
* **Robots.txt Awareness (Design Consideration)** : Designed with the standard requirement of respecting `robots.txt` policies in mind (implementation of fetching/parsing `robots.txt` is a planned enhancement).

## Tech Stack

* **Language** : C++17
* **Core Libraries** : C++ Standard Library (`<thread>`, `<mutex>`, `<condition_variable>`, `<atomic>`, `<vector>`, `<string>`, `<queue>`, `<unordered_set>`, `<optional>`)
* **Networking** : `libcurl` (via CMake `find_package`)
* **Parsing** : `gumbo-parser` (via CMake `find_path`/`find_library`)
* **Build System** : `CMake`

## Architecture Overview

The crawler operates with a producer-consumer pattern using a central thread-safe queue and set:

1. **Initialization** : The main thread initializes `libcurl`, seeds the `url_queue` with the starting URL provided via command line.
2. **Worker Threads** : Multiple worker threads (`NUM_THREADS`) are launched. Each worker runs a loop:

* **Dequeue** : Waits for and pops a URL from the `url_queue` (thread-safe). If the queue signals stop and is empty, the thread exits.
* **Check Visited** : Attempts to insert the URL into the `visited_urls` set (thread-safe). If already present, skips to the next URL.
* **Fetch** : Uses its own `libcurl` handle to download the HTML content of the URL, handling HTTPS and redirects.
* **Parse** : If the fetch is successful and content is HTML, uses `gumbo-parser` to parse the content.
* **Extract & Resolve** : Extracts all `href` attributes from `<a>` tags and resolves them into absolute URLs.
* **Enqueue** : For each valid, absolute URL belonging to the original domain, pushes it onto the `url_queue` (thread-safe).

3. **Monitoring & Termination** : The main thread periodically checks if the `url_queue` is empty and if any `active_workers` (tracked by an `std::atomic`) are still busy. When both conditions are met (queue empty, workers idle), it signals the queue to stop and waits (`join`) for all worker threads to finish.

This process continues until no new, unvisited URLs within the target domain are found.

## Setup and Installation

1. **Prerequisites** :

* A C++17 compliant compiler (GCC, Clang, MSVC).
* CMake (version 3.10+).
* `libcurl` development library installed.
* `gumbo-parser` development library installed.
* (Recommended for Windows) `vcpkg` to manage C++ dependencies.

2. **Clone Repository** : `git https://github.com/keshavk215/Multi-threaded-Web-Crawler`

3. **Configure with CMake** :

```
   cd Multi-threaded-Web-Crawler
   mkdir build
   cd build
   # If using vcpkg (replace path if needed):
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake
   # Or, if libraries are in standard system paths:
   # cmake ..
```

4. **Compile** :

```
   cmake --build .
   # Or on Linux/macOS: make
```

5. **Place CA Certificate** : Ensure `cacert.pem` (downloaded from curl website) is automatically copied to the build output directory by CMake (as configured in `CMakeLists.txt`).

## Usage

Run the compiled executable from the build output directory (e.g., `build/Debug` or `build/`), providing a starting URL:

```
./crawler <start-url>
```

Example:

```
./crawler https://google.com
```
