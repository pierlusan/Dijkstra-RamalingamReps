/**
 * BenchmarkDriver.cpp
 * 
 * Scientific Benchmarking Framework for SSSP Algorithms.
 * Implements a Design of Experiments (DoE) approach to compare Static vs Dynamic algorithms.
 * 
 * Factors:
 *  A: Graph Scale (N = 1000 to 16000)
 *  B: Update Magnitude (Small, Large)
 *  C: Update Type (Increment, Decrement)
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <random>
#include <memory>
#include <iomanip>
#include <cassert>
#include <algorithm> // for std::max

#include "BenchmarkStats.h"
#include "Graph.h"

// --- Interfaces & Adapters ---

struct Edge {
    int u, v;
    long long w;
};

class ISSSPWrapper {
public:
    virtual ~ISSSPWrapper() = default;
    
    // Initialize the algorithm with the graph structure
    virtual void init(int source) = 0;
    
    // Handle a dynamic edge update
    virtual void updateEdge(int u, int v, int newWeight) = 0;
    virtual void addEdge(int u, int v, int weight) = 0;
    virtual void removeEdge(int u, int v) = 0;
    
    // Get the shortest path distance to target
    virtual int getDistance(int target) = 0;
    
    virtual std::string getName() const = 0;
};

// --- Concrete Wrappers (Placeholders for User Implementation) ---

// --- Concrete Wrappers (Placeholders for User Implementation) ---

#include "DijkstraSolver.h"
#include "RamalingamReps.h"



// Redefining wrapper to store source properly
class DijkstraWrapperImpl : public ISSSPWrapper {
    Graph& graph;
    std::unique_ptr<DijkstraSolver> solver;
    int current_source = 0;
public:
    DijkstraWrapperImpl(Graph& g) : graph(g) {
        solver = std::make_unique<DijkstraSolver>(g);
    }

    void init(int source) override {
        current_source = source;
        solver->compute(source);
    }

    void updateEdge(int u, int v, int newWeight) override {
        graph.updateEdge(u, v, newWeight);
        solver->compute(current_source);
    }

    void addEdge(int u, int v, int weight) override {
        graph.addEdge(u, v, weight);
        solver->compute(current_source);
    }

    void removeEdge(int u, int v) override {
        graph.removeEdge(u, v);
        solver->compute(current_source);
    }

    int getDistance(int target) override {
        return solver->getDistance(target);
    }
    
    std::string getName() const override { return "Static_Dijkstra"; }
};

class RRWrapper : public ISSSPWrapper {
    Graph& graph;
    std::unique_ptr<RamalingamReps> solver;
public:
    RRWrapper(Graph& g) : graph(g) {
        solver = std::make_unique<RamalingamReps>(g);
    }

    void init(int source) override {
        solver->initialize(source);
    }

    void updateEdge(int u, int v, int newWeight) override {
        solver->handleEdgeUpdate(u, v, newWeight);
    }

    void addEdge(int u, int v, int weight) override {
        solver->handleEdgeInsertion(u, v, weight);
    }

    void removeEdge(int u, int v) override {
        solver->handleEdgeDeletion(u, v);
    }

    int getDistance(int target) override {
        return solver->getDistance(target);
    }
    
    std::string getName() const override { return "Dynamic_RR"; }
};



// --- Graph Generation Utilities ---

void generateConnectedGraph(Graph& g, int N, int M) {
    if (g.numVertices != N) {
        // Assuming Graph has a resize/reset mechanism or we recreate it.
        // For this benchmark, we pass a fresh Graph instance.
    }
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> weightDist(1, 100);
    
    // 1. Create a spanning tree to ensure connectivity (0 -> 1 -> 2 ... -> N-1 is simple but works)
    // A better random tree:
    std::vector<int> nodes(N);
    std::iota(nodes.begin(), nodes.end(), 0);
    std::shuffle(nodes.begin(), nodes.end(), rng);
    
    for (int i = 0; i < N - 1; ++i) {
        g.addEdge(nodes[i], nodes[i+1], weightDist(rng));
    }
    
    int currentEdges = N - 1;
    
    // 2. Add remaining random edges
    std::uniform_int_distribution<int> nodeDist(0, N - 1);
    while (currentEdges < M) {
        int u = nodeDist(rng);
        int v = nodeDist(rng);
        if (u != v && g.getEdgeWeight(u, v) == -1) {
            g.addEdge(u, v, weightDist(rng));
            currentEdges++;
        }
    }
}

// --- Benchmark Logic ---

struct UpdateCase {
    int u, v;
    int oldW, newW;
    std::string type;      // "Inc" or "Dec"
    std::string magnitude; // "Small" or "Large"
};

UpdateCase generateUpdate(Graph& g, std::mt19937& rng) {
    std::uniform_int_distribution<int> nodeDist(0, g.numVertices - 1);
    std::uniform_int_distribution<int> opDist(0, 100); 
    
    while(true) {
        int op = opDist(rng);
        
        // 50% Weight Update, 25% Add, 25% Delete
        
        if (op < 50) { // Weight Update
            int u = nodeDist(rng);
            if (g.adj[u].empty()) continue;
            
            std::uniform_int_distribution<int> neighborDist(0, g.adj[u].size() - 1);
            auto& edge = g.adj[u][neighborDist(rng)];
            int v = edge.first;
            int w = edge.second;
            
            UpdateCase uc;
            uc.u = u; uc.v = v; uc.oldW = w;
            
            std::uniform_int_distribution<int> typeDist(0, 1);
            if (typeDist(rng) == 0) { // Dec
                if (w <= 1) continue;
                uc.type = "Dec";
                uc.newW = std::max(1, w / 2);
                uc.magnitude = "Large"; // Simplifying: always large/halve
            } else { // Inc
                uc.type = "Inc";
                uc.newW = w * 2;
                uc.magnitude = "Large";
            }
            return uc;
            
        } else if (op < 75) { // Add Edge
            int u = nodeDist(rng);
            int v = nodeDist(rng);
            if (u == v) continue;
            if (g.getEdgeWeight(u, v) != -1) continue; // Already exists
            
            UpdateCase uc;
            uc.u = u; uc.v = v; 
            uc.type = "Add";
            std::uniform_int_distribution<int> wDist(1, 100);
            uc.newW = wDist(rng);
            uc.magnitude = "Struct";
            return uc;
            
        } else { // Delete Edge
            int u = nodeDist(rng);
            if (g.adj[u].empty()) continue;
            
            std::uniform_int_distribution<int> neighborDist(0, g.adj[u].size() - 1);
            auto& edge = g.adj[u][neighborDist(rng)];
            
            UpdateCase uc;
            uc.u = u; uc.v = edge.first;
            uc.type = "Del";
            uc.oldW = edge.second;
            uc.magnitude = "Struct";
            return uc;
        }
    }
}

void run_benchmark_suite() {
    std::vector<int> scales = {1000, 2000, 4000, 8000, 16000, 32000, 64000, 128000, 256000, 512000, 1024000, 2048000};
    int density_factor = 4;
    int K_updates = 1000;
    int source_node = 0;

    std::cout << "Graph_N,Graph_M,Update_ID,Type,Magnitude,Time_Static_ns,Ops_Static,Time_Dyn_ns,Ops_Dyn,Speedup" << std::endl;

    for (int N : scales) {
        int M = N * density_factor;
        
        // Setup Graphs
        Graph g_static(N);
        Graph g_dyn(N);
        generateConnectedGraph(g_static, N, M);
        generateConnectedGraph(g_dyn, N, M); // Identical topology/weights logic (random seed 42 in generator)
        
        // Setup Wrappers
        DijkstraWrapperImpl staticAlgo(g_static);
        RRWrapper dynAlgo(g_dyn);
        
        staticAlgo.init(source_node);
        dynAlgo.init(source_node);
        
        std::mt19937 rng(12345); // Benchmark loop rng
        
        for (int k = 0; k < K_updates; ++k) {
            UpdateCase uc = generateUpdate(g_static, rng);
            
            // --- Static Measurement ---
            Stats::reset();
            auto startS = std::chrono::high_resolution_clock::now();
            if (uc.type == "Add") staticAlgo.addEdge(uc.u, uc.v, uc.newW);
            else if (uc.type == "Del") staticAlgo.removeEdge(uc.u, uc.v);
            else staticAlgo.updateEdge(uc.u, uc.v, uc.newW);
            auto endS = std::chrono::high_resolution_clock::now();
            long long t_static = std::chrono::duration_cast<std::chrono::nanoseconds>(endS - startS).count();
            long long ops_static = Stats::heap_ops + Stats::scanned_edges;
            
            // --- Dynamic Measurement ---
            Stats::reset();
            auto startD = std::chrono::high_resolution_clock::now();
            if (uc.type == "Add") dynAlgo.addEdge(uc.u, uc.v, uc.newW);
            else if (uc.type == "Del") dynAlgo.removeEdge(uc.u, uc.v);
            else dynAlgo.updateEdge(uc.u, uc.v, uc.newW);
            auto endD = std::chrono::high_resolution_clock::now();
            long long t_dyn = std::chrono::duration_cast<std::chrono::nanoseconds>(endD - startD).count();
            long long ops_dyn = Stats::heap_ops + Stats::scanned_edges;
            
            // --- Validation ---
            for (int i = 0; i < 10; ++i) {
                std::uniform_int_distribution<int> checkDist(0, N - 1);
                int node = checkDist(rng);
                if (staticAlgo.getDistance(node) != dynAlgo.getDistance(node)) {
                     std::cerr << "Validation Failed at Update " << k << " Node " << node << std::endl;
                     std::cerr << "Static: " << staticAlgo.getDistance(node) << " Dyn: " << dynAlgo.getDistance(node) << std::endl;
                     exit(1);
                }
            }
            
            double speedup = (t_dyn > 0) ? (double)t_static / t_dyn : 0.0;
            
            std::cout << N << "," << M << "," << k << "," 
                      << uc.type << "," << uc.magnitude << ","
                      << t_static << "," << ops_static << ","
                      << t_dyn << "," << ops_dyn << ","
                      << std::fixed << std::setprecision(2) << speedup << "\n";
        }
    }
}

int main() {
    // Fast IO
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    
    run_benchmark_suite();
    return 0;
}
