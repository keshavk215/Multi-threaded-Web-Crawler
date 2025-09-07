/**
 * @file main.cpp
 * @brief Entry point for the Multi-threaded Web Crawler.
 *
 * This file contains the main function that initializes the crawler,
 * parses command-line arguments, creates and manages worker threads,
 * and waits for the crawling process to complete.
 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include "../include/crawler.hpp"

/**
 * @brief Prints the usage instructions for the program.
 * @param prog_name The name of the executable.
 */
void print_usage(const std::string& prog_name) {
    std::cerr << "Usage: " << prog_name << " <start-url> <num-threads>" << std::endl;
    std::cerr << "Example: " << prog_name << " https://www.example.com 8" << std::endl;
}

/**
 * @brief The main function of the web crawler application.
 *
 * It parses command-line arguments for the starting URL and the number of
 * threads to use. It then creates a Crawler instance and the specified
 * number of worker threads to begin the crawling process.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return Returns 0 on successful execution, 1 on error.
 */
int main(int argc, char* argv[]) {
    // --- 1. Argument Parsing ---
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string start_url;
    int num_threads = 0;

    try {
        start_url = argv[1];
        num_threads = std::stoi(argv[2]);
        if (num_threads <= 0) {
            std::cerr << "Error: Number of threads must be a positive integer." << std::endl;
            return 1;
        }
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: Invalid number of threads provided." << std::endl;
        print_usage(argv[0]);
        return 1;
    } catch (const std::out_of_range& e) {
        std::cerr << "Error: Number of threads is out of range." << std::endl;
        return 1;
    }

    std::cout << "Starting crawl from: " << start_url << std::endl;
    std::cout << "Using " << num_threads << " worker threads." << std::endl;

    // --- 2. Crawler Initialization ---
    // Create an instance of the Crawler class, passing the starting URL.
    // This will initialize the thread-safe queue and the set of visited URLs.
    Crawler crawler(start_url);

    // --- 3. Thread Creation ---
    // Create a vector to hold all the worker threads.
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    // Launch the specified number of worker threads.
    // Each thread will execute the `run` method of the crawler instance.
    for (int i = 0; i < num_threads; ++i) {
        // The `std::ref(crawler)` ensures that each thread gets a reference
        // to the same crawler object, not a copy.
        threads.emplace_back(&Crawler::run, std::ref(crawler));
        std::cout << "Launched thread " << i + 1 << std::endl;
    }

    // --- 4. Wait for Completion ---
    // The main thread will now wait for all worker threads to finish their execution.
    // This is a blocking call; the main function will not proceed until all
    // threads have joined.
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "Crawling finished." << std::endl;
    std::cout << "Total unique pages visited: " << crawler.get_visited_count() << std::endl;

    // --- 5. Print Results (Optional) ---
    // Optionally, you could add a function here to print the crawled links
    // or the index that was built.
    // crawler.print_results();

    return 0;
}
