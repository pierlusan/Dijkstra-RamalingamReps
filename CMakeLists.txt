cmake_minimum_required(VERSION 3.10)
project(Dijkstra)

set(CMAKE_CXX_STANDARD 17)

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
