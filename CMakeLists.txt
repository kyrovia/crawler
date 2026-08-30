cmake_minimum_required(VERSION 3.16)
project(crawler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

add_library(crawler_core
    src/http.cpp
    src/html.cpp
    src/walk.cpp
)
target_include_directories(crawler_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(crawler_core PUBLIC CURL::libcurl Threads::Threads)
target_compile_options(crawler_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(crawler src/crawler.cpp)
target_link_libraries(crawler PRIVATE crawler_core)
target_compile_options(crawler PRIVATE -Wall -Wextra -Wpedantic)

add_executable(crawler_concurrent src/crawler_concurrent.cpp)
target_link_libraries(crawler_concurrent PRIVATE crawler_core)
target_compile_options(crawler_concurrent PRIVATE -Wall -Wextra -Wpedantic)
