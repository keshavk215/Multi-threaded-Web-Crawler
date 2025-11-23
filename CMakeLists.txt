cmake_minimum_required(VERSION 3.10) # Set minimum CMake version

project(SimpleWebCrawler LANGUAGES CXX) # Project name and language

set(CMAKE_CXX_STANDARD 17) # Use C++17 standard
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Find the libcurl library ---
# find_package is the CMake command to locate installed libraries
# REQUIRED means CMake will error out if it can't find curl
find_package(CURL REQUIRED)

# --- Find Gumbo manually using find_path and find_library ---
# Find the header file (gumbo.h)
find_path(GUMBO_INCLUDE_DIR gumbo.h
    HINTS ${CMAKE_INSTALL_PREFIX}/include # Check standard include paths first
    PATH_SUFFIXES include                # Look in 'include' subdirectories
    # You might need to add specific hints if vcpkg installed elsewhere and isn't integrated
    # HINTS C:/src/vcpkg/installed/x64-windows/include
)

# Find the library file (e.g., gumbo.lib on Windows, libgumbo.a/.so on Linux)
find_library(GUMBO_LIBRARY NAMES gumbo          # Look for 'gumbo' (adjust if needed)
    HINTS ${CMAKE_INSTALL_PREFIX}/lib # Check standard lib paths
    PATH_SUFFIXES lib                 # Look in 'lib' subdirectories
    # You might need to add specific hints if vcpkg installed elsewhere and isn't integrated
    # HINTS C:/src/vcpkg/installed/x64-windows/lib
)

# --- Check if Gumbo components were found ---
if(NOT GUMBO_INCLUDE_DIR OR NOT GUMBO_LIBRARY)
    message(FATAL_ERROR "Could not find Gumbo library. Check include/library paths.")
else()
    message(STATUS "Found Gumbo include directory: ${GUMBO_INCLUDE_DIR}")
    message(STATUS "Found Gumbo library: ${GUMBO_LIBRARY}")
endif()

# --- Find Threads (for std::thread) ---
find_package(Threads REQUIRED)

# --- Define our executable ---
# Add header files to the executable target so IDEs recognize them (optional but good)
add_executable(crawler src/main.cpp include/thread_safe_queue.hpp include/thread_safe_set.hpp)

# --- Link libcurl to our executable ---
# target_link_libraries tells CMake to link the specified libraries
# to our 'crawler' executable.
# CURL::libcurl is the modern way to refer to the imported curl target
target_link_libraries(crawler PRIVATE
    CURL::libcurl    # Use the target from find_package for curl
    ${GUMBO_LIBRARY} # Link the manually found gumbo library
    Threads::Threads
)
# --- Include directories ---
# Make sure the compiler can find the libcurl headers
# (Often needed, especially if not installed in a standard system location)
target_include_directories(crawler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include # Include path for our own headers
    ${CURL_INCLUDE_DIRS} # Include path from find_package for curl
    ${GUMBO_INCLUDE_DIR} # Include path found manually for gumbo
)

# --- NEW: Auto-copy cacert.pem after building ---
# Define the source path (project root) and destination path (executable directory)
set(CERT_SOURCE_PATH ${CMAKE_SOURCE_DIR}/cacert.pem)
set(CERT_DEST_PATH ${CMAKE_CURRENT_BINARY_DIR}/cacert.pem) # Default build output

# For multi-config generators like Visual Studio, adjust the destination
if(CMAKE_CONFIGURATION_TYPES)
  set(CERT_DEST_PATH ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/cacert.pem)
endif()

# Add the command to copy the file after the 'crawler' target is built
add_custom_command(TARGET crawler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CERT_SOURCE_PATH}
        ${CERT_DEST_PATH}
    COMMENT "Copying cacert.pem to output directory"
)

# Print a message indicating where the executable will be built
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Executable will be built in ${CMAKE_BINARY_DIR}")





# ** Building and Running**

# Use CMake to build the program.

# 1.  **Open your terminal** in the main project folder (`web-crawler-cpp`).
# 2.  **Create a build directory:** This keeps the build files separate from your source code.
#     ```bash
#     mkdir build
#     cd build
#     ```
# 3.  **Run CMake to configure the build:** This is where you tell CMake about vcpkg. Replace the path with your actual vcpkg path.
#     ```bash
#     cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake
#     ```
#     *(If you integrated vcpkg system-wide, you might be able to just run `cmake ..`)*
#     You should see output confirming CMake found the Curl library.
# 4.  **Compile the code:**
#     ```bash
#     cmake --build .
#     ```
#     *(On Linux/macOS, you can often just type `make` here)*
#     This will compile `main.cpp` and link it with `libcurl` to create an executable file (e.g., `crawler.exe` on Windows, `crawler` on Linux/macOS) inside the `build` directory (or a subdirectory like `Debug`).
# 5.  **Run the executable:** Navigate to where the executable was created (it might be in `build/Debug/`) and run it, providing a URL:
#     ```bash
#     # Example (adjust path and URL as needed):
#     ./crawler https://example.com
    
