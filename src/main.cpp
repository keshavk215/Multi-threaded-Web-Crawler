#include <iostream>
#include <string>
#include <vector>
#include <thread> // For std::thread
#include <atomic> // For std::atomic_int
#include <chrono> // For std::chrono::seconds
#include <optional>
#include <set>    // For basic URL processing
#include <curl/curl.h>
#include <gumbo.h>

// Include our thread-safe classes
#include "thread_safe_queue.hpp"
#include "thread_safe_set.hpp"

// --- Global Shared Data ---
// These are declared globally or passed around so all threads can access them
ThreadSafeQueue<std::string> url_queue;
ThreadSafeSet visited_urls;
std::atomic<int> active_workers = 0; // Count of threads actively fetching/parsing
const int NUM_THREADS = 4;           // Number of worker threads to create

// --- Function Declarations ---
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
void search_for_links(GumboNode* node, std::vector<std::string>& links, const std::string& base_url);
std::string resolve_url(const std::string& base_url, const std::string& relative_url);
void worker_thread_function(int id);

// --- libcurl Write Callback (remains the same) ---
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

// --- Gumbo Parser (Modified to resolve relative URLs) ---
// We now need the base URL to correctly handle relative links like "/about"
static void search_for_links(GumboNode* node, std::vector<std::string>& links, const std::string& base_url) {
    if (node->type != GUMBO_NODE_ELEMENT) return;

    if (node->v.element.tag == GUMBO_TAG_A) {
        GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
        if (href && href->value && href->value[0] != '\0') { // Check if href exists and is not empty
             // NEW: Resolve relative URLs (like "/about") into absolute URLs
             std::string resolved = resolve_url(base_url, href->value);
             if (!resolved.empty()) { // Only add valid, resolved URLs
                links.push_back(resolved);
             }
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        search_for_links(static_cast<GumboNode*>(children->data[i]), links, base_url);
    }
}

// --- NEW: URL Resolution Helper ---
// Basic function to convert relative URLs to absolute URLs
// A robust crawler needs a much more complex version of this!
std::string resolve_url(const std::string& base_url, const std::string& relative_url) {
    // Very basic checks - skip javascript, mailto, anchors, etc.
    if (relative_url.rfind("javascript:", 0) == 0 ||
        relative_url.rfind("mailto:", 0) == 0 ||
        relative_url.find('#') != std::string::npos) {
        return ""; // Ignore these types of links
    }

    // If it's already an absolute URL
    if (relative_url.rfind("http://", 0) == 0 || relative_url.rfind("https://", 0) == 0) {
        return relative_url;
    }

    // If it starts with "//" (protocol-relative)
    if (relative_url.rfind("//", 0) == 0) {
        // Find protocol of base URL
        size_t proto_end = base_url.find(':');
        if (proto_end != std::string::npos) {
            return base_url.substr(0, proto_end + 1) + relative_url;
        }
        return ""; // Cannot determine protocol
    }

    // If it starts with "/" (relative to root)
    if (relative_url.rfind("/", 0) == 0) {
        // Find the end of the domain part of the base URL
        // Example: https://www.example.com/some/path -> https://www.example.com
        size_t protocol_pos = base_url.find("//");
        if (protocol_pos == std::string::npos) {
             return ""; // Invalid base URL?
        }
        size_t domain_end = base_url.find('/', protocol_pos + 2);
        if (domain_end != std::string::npos) {
            return base_url.substr(0, domain_end) + relative_url;
        } else {
            // Base URL might just be the domain (e.g., https://example.com)
            return base_url + relative_url;
        }
    }

    // Otherwise, it's relative to the current path (e.g., "otherpage.html")
    size_t last_slash = base_url.rfind('/');
    // Make sure it's after the protocol part "https://" (index > 7)
    if (last_slash != std::string::npos && last_slash > 7) {
        return base_url.substr(0, last_slash + 1) + relative_url;
    } else {
         // Base URL might not have a path (e.g., https://example.com)
        return base_url + "/" + relative_url;
    }
     // A production crawler needs a proper URL parsing library (like Boost.URL) for robust handling!
}

// --- NEW: Worker Thread Function ---
void worker_thread_function(int id) {
    std::cout << "Worker [" << id << "] started." << std::endl;
    CURL* curl_handle = curl_easy_init(); // Each thread needs its own curl handle
    if (!curl_handle) {
        std::cerr << "Worker [" << id << "] failed to initialize curl handle." << std::endl;
        return;
    }
     // Set common curl options once
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "MySimpleCrawler/1.0"); // Be polite, identify crawler
    curl_easy_setopt(curl_handle, CURLOPT_CAINFO, "cacert.pem"); // Path to CA cert bundle
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L); // Verify the server's SSL certificate
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L); // Verify the certificate's name against host
    // Set timeouts to prevent threads from getting stuck indefinitely
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 10L); // 10 seconds to connect
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 20L); // 20 seconds for the entire transfer


    while (true) {
        std::optional<std::string> maybe_url = url_queue.pop(); // Wait for a URL

        // Check if we should stop (pop returns nullopt if stop requested & queue empty)
        if (!maybe_url) {
             //std::cout << "Worker [" << id << "] received stop signal or queue empty." << std::endl;
            break; // Exit the loop
        }

        std::string url = *maybe_url;

        // --- Critical Section: Check Visited Set ---
        // Insert returns true only if the url was NOT already present
        if (!visited_urls.insert(url)) {
            //std::cout << "Worker [" << id << "] skipping already visited: " << url << std::endl;
            continue; // Already visited, grab the next URL
        }
        // --- End Critical Section ---

        active_workers++; // Increment active worker count (atomic, safe)
        //std::cout << "Worker [" << id << "] fetching: " << url << " (Visited: " << visited_urls.size() << ")" << std::endl;

        std::string readBuffer; // Buffer specific to this request in this thread
        curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &readBuffer); // Point to this thread's buffer

        CURLcode res = curl_easy_perform(curl_handle); // Perform the fetch

        if (res == CURLE_OK) {
            long response_code;
            curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);

            // Only parse if the response was successful (HTTP 2xx)
             if (response_code >= 200 && response_code < 300) {
                 // Check if the content type is HTML before parsing
                 char *ct = nullptr;
                 res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
                 // Check if content type exists and contains "text/html"
                 if (res == CURLE_OK && ct && std::string(ct).find("text/html") != std::string::npos) {

                    GumboOutput* output = gumbo_parse(readBuffer.c_str()); // Parse HTML
                    if (output && output->root) {
                        std::vector<std::string> links;
                        search_for_links(output->root, links, url); // Extract links, pass base URL
                        gumbo_destroy_output(&kGumboDefaultOptions, output); // Free Gumbo memory

                        // --- Add newly found links to the queue ---
                        int added = 0;
                        for (const std::string& link : links) {
                             // Basic check: Only crawl URLs from the same domain (simplistic!)
                             // Extract domain from start_url (passed implicitly for now)
                             // This needs a proper URL parsing library for robustness
                             // Example: if start_url is https://www.iitk.ac.in/path/
                             // We only want links starting with https://www.iitk.ac.in
                             size_t start_domain_pos = url.find("//") + 2;
                             size_t end_domain_pos = url.find('/', start_domain_pos);
                             std::string domain = url.substr(0, end_domain_pos);

                             if (link.rfind(domain, 0) == 0) { // Check if link starts with the same domain
                                url_queue.push(link); // Add to shared queue (thread-safe)
                                added++;
                             }
                        }
                         //std::cout << "Worker [" << id << "] parsed " << links.size() << " links, added " << added << " from: " << url << std::endl;
                    } else {
                         std::cerr << "Worker [" << id << "] Gumbo failed to parse: " << url << std::endl;
                    }
                 } else {
                     //std::cout << "Worker [" << id << "] skipping non-HTML content (" << (ct ? ct : "N/A") << "): " << url << std::endl;
                 }
             } else {
                  //std::cerr << "Worker [" << id << "] received non-2xx status code (" << response_code << ") for: " << url << std::endl;
             }
        } else {
            // Log curl errors, but continue working
            std::cerr << "Worker [" << id << "] curl_easy_perform() failed for " << url << ": " << curl_easy_strerror(res) << std::endl;
        }
        active_workers--; // Decrement active worker count (atomic, safe)
    } // End of while loop

    curl_easy_cleanup(curl_handle); // Clean up this thread's curl handle
    std::cout << "Worker [" << id << "] finished." << std::endl;
}

// --- Main function (rewritten for multi-threading) ---
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <Start URL>" << std::endl;
        return 1;
    }
    std::string start_url = argv[1];

    // --- Initialize curl globally ---
    // Needs to be called once per program run
    curl_global_init(CURL_GLOBAL_ALL);

    // Add the starting URL to the queue
    url_queue.push(start_url);

    // --- Create and launch worker threads ---
    std::vector<std::thread> workers;
    std::cout << "Launching " << NUM_THREADS << " worker threads..." << std::endl;
    for (int i = 0; i < NUM_THREADS; ++i) {
        // Create a thread and run worker_thread_function with its ID
        workers.emplace_back(worker_thread_function, i);
    }

    // --- Main loop to monitor progress and decide when to stop ---
    // This simple logic stops when the queue is empty AND no workers are busy.
    // A more robust crawler might have a timeout or max pages limit.
    while (true) {
        // Sleep for a short duration to avoid busy-waiting in the main thread
        std::this_thread::sleep_for(std::chrono::seconds(2));

        bool is_queue_empty = url_queue.empty(); // Check if queue is empty (thread-safe check)
        int current_active = active_workers.load(); // Read atomic counter (thread-safe)

        std::cout << "Monitoring: Queue empty? " << (is_queue_empty ? "Yes" : "No")
                  << ", Active workers: " << current_active
                  << ", Visited: " << visited_urls.size() << std::endl;

        // If the queue is empty AND no threads are currently fetching/parsing, we are done.
        if (is_queue_empty && current_active == 0) {
            std::cout << "Queue empty and workers idle. Requesting stop..." << std::endl;
            url_queue.request_stop(); // Signal the queue to stop and wake up waiting threads
            break; // Exit the monitoring loop
        }
    }

    // --- Wait for all worker threads to finish ---
    std::cout << "Waiting for workers to join..." << std::endl;
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join(); // Wait for the thread to complete its execution
        }
    }

    // --- Cleanup curl globally ---
    // Needs to be called once after all curl operations are done
    curl_global_cleanup();

    std::cout << "\n--- Crawling Finished ---" << std::endl;
    std::cout << "Total unique pages visited: " << visited_urls.size() << std::endl;

    return 0;
}
