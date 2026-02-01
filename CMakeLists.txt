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

# Eseguibile principale con doubling experiment
add_executable(dijkstra_main main.cpp
        DijkstraSolver.cpp
        DijkstraSolver.h
        Graph.cpp
        Graph.h
        RamalingamReps.cpp
        RamalingamReps.h
        BenchmarkStats.h)
target_link_libraries(dijkstra_main PRIVATE ZLIB::ZLIB)

# Benchmark scientifico (Dijkstra vs Ramalingam-Reps)
add_executable(scientific_benchmark BenchmarkDriver.cpp
        DijkstraSolver.cpp
        DijkstraSolver.h
        Graph.cpp
        Graph.h
        RamalingamReps.cpp
        RamalingamReps.h
        BenchmarkStats.h)
target_link_libraries(scientific_benchmark PRIVATE ZLIB::ZLIB)
