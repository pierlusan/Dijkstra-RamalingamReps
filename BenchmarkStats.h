#ifndef BENCHMARK_STATS_H
#define BENCHMARK_STATS_H

#include <atomic>

/**
 * Singleton/Static struct for collecting abstract algorithmic metrics.
 * Designed to be decoupled from timing logic.
 * 
 * Usage in Algorithms:
 *   #include "BenchmarkStats.h"
 *   ...
 *   Stats::heap_ops++;
 *   Stats::scanned_edges++;
 */
struct Stats {
    // Total Priority Queue operations (push + pop + decrease_key)
    static inline std::atomic<long long> heap_ops{0};
    
    // Number of edges relaxed/visited during the algorithm execution
    static inline std::atomic<long long> scanned_edges{0};
    
    // Number of nodes extracted from the queue (or finalized)
    static inline std::atomic<long long> visited_nodes{0};
    
    // Only used for previous code compatibility if needed, but per scientific reqs we rely on the above.
    static inline std::atomic<long long> relaxed_edges{0}; 

    static void reset() {
        heap_ops = 0;
        scanned_edges = 0;
        visited_nodes = 0;
        relaxed_edges = 0;
    }
};

#endif // BENCHMARK_STATS_H
