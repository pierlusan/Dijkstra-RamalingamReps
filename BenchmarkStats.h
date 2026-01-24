#ifndef BENCHMARK_STATS_H
#define BENCHMARK_STATS_H

#include <iostream>

struct Stats {
    static long long heap_ops;
    static long long visited_nodes;
    static long long relaxed_edges;

    static void reset() {
        heap_ops = 0;
        visited_nodes = 0;
        relaxed_edges = 0;
    }

    static void print() {
        std::cout << "Heap Ops: " << heap_ops << ", Visited: " << visited_nodes << ", Relaxed: " << relaxed_edges << std::endl;
    }
};

// Definition of static members
inline long long Stats::heap_ops = 0;
inline long long Stats::visited_nodes = 0;
inline long long Stats::relaxed_edges = 0;

#endif // BENCHMARK_STATS_H
