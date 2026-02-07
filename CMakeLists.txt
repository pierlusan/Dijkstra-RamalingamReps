cmake_minimum_required(VERSION 3.10)
project(Dijkstra)

set(CMAKE_CXX_STANDARD 17)

# Build type: Release per massima efficienza (no debug symbols)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Trova zlib per supporto file .gz
find_package(ZLIB REQUIRED)

# Generatore grafi (sparsi, Barabási-Albert, densi)
add_executable(graph_generator graph_generator.cpp
        Graph.cpp
        Graph.h)
target_link_libraries(graph_generator PRIVATE ZLIB::ZLIB)

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

# Test Ramalingam-Reps complessità O(||δ|| log ||δ||)
add_executable(test_ramalingam test_ramalingam.cpp
        DijkstraSolver.cpp
        DijkstraSolver.h
        Graph.cpp
        Graph.h
        RamalingamReps.cpp
        RamalingamReps.h
        BenchmarkStats.h)
target_link_libraries(test_ramalingam PRIVATE ZLIB::ZLIB)

