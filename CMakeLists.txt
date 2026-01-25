cmake_minimum_required(VERSION 3.10)
project(Dijkstra)

set(CMAKE_CXX_STANDARD 17)

# Profiling configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")

# Trova zlib per supporto file .gz
find_package(ZLIB REQUIRED)

add_executable(Dijkstra main.cpp
        DijkstraSolver.cpp
        DijkstraSolver.h
        Graph.cpp
        Graph.h
        RamalingamReps.cpp
        RamalingamReps.h)

# Linka zlib all'eseguibile
target_link_libraries(Dijkstra PRIVATE ZLIB::ZLIB)

add_executable(benchmark_driver main_benchmark.cpp
        DijkstraSolver.cpp
        DijkstraSolver.h
        Graph.cpp
        Graph.h
        RamalingamReps.cpp
        RamalingamReps.h
        BenchmarkStats.h)
target_link_libraries(benchmark_driver PRIVATE ZLIB::ZLIB)

add_executable(scientific_benchmark BenchmarkDriver.cpp
        DijkstraSolver.cpp
        DijkstraSolver.h
        Graph.cpp
        Graph.h
        RamalingamReps.cpp
        RamalingamReps.h
        BenchmarkStats.h)
target_link_libraries(scientific_benchmark PRIVATE ZLIB::ZLIB)

add_executable(dijkstra_runner dijkstra_runner.cpp
        DijkstraSolver.cpp
        DijkstraSolver.h
        Graph.cpp
        Graph.h
        BenchmarkStats.h)
target_link_libraries(dijkstra_runner PRIVATE ZLIB::ZLIB)
