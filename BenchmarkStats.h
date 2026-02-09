#ifndef BENCHMARK_STATS_H
#define BENCHMARK_STATS_H

#include <atomic>

struct Stats {
    // Total Priority Queue operations (push + pop + decrease_key)
    static inline std::atomic<long long> heap_ops{0};
    
    // Number of edges examined during the algorithm execution
    static inline std::atomic<long long> scanned_edges{0};
    
    // Number of nodes extracted from the queue (or finalized)
    static inline std::atomic<long long> visited_nodes{0};
    
    // Number of edges successfully relaxed (distance improved)
    static inline std::atomic<long long> relaxed_edges{0}; 
    
    // Number of nodes whose distance changed after an update (for dynamic algorithms)
    static inline std::atomic<long long> affected_nodes{0};
    
    // Number of edges incident to affected nodes (for ||δ|| calculation)
    static inline std::atomic<long long> affected_edges{0};

    static void reset() {
        heap_ops = 0;
        scanned_edges = 0;
        visited_nodes = 0;
        relaxed_edges = 0;
        affected_nodes = 0;
        affected_edges = 0;
    }
};

#endif // BENCHMARK_STATS_H
