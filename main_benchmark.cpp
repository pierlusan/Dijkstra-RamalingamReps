#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <cassert>
#include "Graph.h"
#include "DijkstraSolver.h"
#include "RamalingamReps.h"
#include "BenchmarkStats.h"

// --- Interfaces & Adapters ---

class ISSSPWrapper {
public:
    virtual ~ISSSPWrapper() = default;
    virtual void init(int source) = 0;
    virtual void updateEdge(int u, int v, int w) = 0;
    virtual int getDist(int target) = 0;
    virtual std::string getName() = 0;
};

class DijkstraWrapper : public ISSSPWrapper {
    Graph& graph;
    DijkstraSolver solver;
    int source = -1;
public:
    DijkstraWrapper(Graph& g) : graph(g), solver(g) {}

    void init(int s) override {
        source = s;
        solver.compute(source);
    }

    void updateEdge(int u, int v, int w) override {
        // Static: update graph, then re-run from scratch
        graph.updateEdge(u, v, w);
        if(source != -1) {
            solver.compute(source);
        }
    }

    int getDist(int target) override {
        return solver.getDistance(target);
    }

    std::string getName() override { return "Static_Dijkstra"; }
};

class RRWrapper : public ISSSPWrapper {
    Graph& graph;
    RamalingamReps solver;
    int source = -1;
public:
    RRWrapper(Graph& g) : graph(g), solver(g) {}

    void init(int s) override {
        source = s;
        solver.initialize(source);
    }

    void updateEdge(int u, int v, int w) override {
        // Dynamic: handle update incrementally
        // Note: Graph update is handled inside handleEdgeUpdate usually, 
        // but RamalingamReps::handleEdgeUpdate spec says:
        // "Aggiorna automaticamente il grafo..." so we just call it.
        solver.handleEdgeUpdate(u, v, w);
    }

    int getDist(int target) override {
        return solver.getDistance(target);
    }

    std::string getName() override { return "Dynamic_RR"; }
};

// --- Benchmark Logic ---

struct EdgeUpdate {
    int u, v, oldW, newW;
    std::string type; // "Inc" or "Dec"
};

void generateRandomGraph(Graph& g, int N, int M) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> distNodes(0, N - 1);
    std::uniform_int_distribution<int> distWeight(1, 100);

    int edgesCheck = 0;
    while(edgesCheck < M) {
        int u = distNodes(rng);
        int v = distNodes(rng);
        if(u != v && g.getEdgeWeight(u, v) == -1) {
            g.addEdge(u, v, distWeight(rng));
            edgesCheck++;
        }
    }
}

std::vector<EdgeUpdate> generateUpdates(Graph& g, int K) {
    std::vector<EdgeUpdate> updates;
    std::mt19937 rng(12345); // Different seed
    std::uniform_int_distribution<int> distNodes(0, g.numVertices - 1);
    std::uniform_int_distribution<int> typeDist(0, 1);

    int attempts = 0;
    while(updates.size() < K && attempts < K * 10) {
        attempts++;
        int u = distNodes(rng);
        // Find a valid neighbor to ensure we update an existing edge
        if(g.adj[u].empty()) continue;
        
        std::uniform_int_distribution<int> neighborDist(0, g.adj[u].size() - 1);
        auto& edge = g.adj[u][neighborDist(rng)];
        int v = edge.first;
        int oldW = edge.second;

        int newW;
        std::string type;
        if(typeDist(rng) == 0) { // Decrease
            if(oldW <= 1) continue;
            newW = oldW / 2;
            type = "Dec";
        } else { // Increase
            newW = oldW * 2;
            type = "Inc";
        }

        updates.push_back({u, v, oldW, newW, type});
    }
    return updates;
}

int main() {
    int N = 0; // Will be set after loading
    int K = 50; 
    int source = 1; // Use 1 as source (common in DIMACS, 1-based usually converted or not? Check Graph.cpp if needed, but safe index)

    // Load USA-road-d.W.gr
    // Note: Graph constructor asks for numVertices, but loadFromDIMACS might resize or we should know it.
    // Graph::loadFromDIMACS usually parsers header.
    // But Graph constructor expects N. Let's create with dummy N and let it handle or read header first.
    // Looking at Graph.h, loadFromDIMACS parsers "p sp n m" header.
    
    // NOTE: Graph implementation might require specific initialization. 
    // Let's assume N=1 (dummy) and it resizes or we read N first?
    // Since we don't know N beforehand easily without parsing, let's trust Graph to handle it or provide a large enough N.
    // The USA-road-d.W.gr usually has ~6M nodes.
    
    std::string graphPath = "grafi/USA-road-d.W.gr";
    
    std::cout << "Loading graph from " << graphPath << "..." << std::endl;
    
    // Create graphs. Initialize with 1 to pass constructor validation (throws if <= 0).
    // loadFromDIMACS will resize it properly based on file header.
    Graph g_static(1);
    Graph g_dynamic(1);

    g_static.loadFromDIMACS(graphPath);
    g_dynamic.loadFromDIMACS(graphPath); // Load twice to have independent instances

    N = g_static.numVertices;
    // M is not exposed directly in Graph public fields easily except via adj size sum, but let's just print loaded.
    std::cout << "Graph loaded. N=" << N << std::endl;
    std::cout << "Benchmarking SSSP: K=" << K << std::endl;

    DijkstraWrapper staticAlgo(g_static);
    RRWrapper dynAlgo(g_dynamic);

    // Init
    staticAlgo.init(source);
    dynAlgo.init(source);

    auto updates = generateUpdates(g_static, K); // Generate updates based on initial state

    std::ofstream csvFile("results.csv");
    if(!csvFile.is_open()) {
        std::cerr << "Error: Could not open results.csv for writing." << std::endl;
        return 1;
    }
    
    // Header to CSV
    csvFile << "UpdateID,Type,Time_Static_us,HeapOps_Static,Relax_Static,Time_Dyn_us,HeapOps_Dyn,Relax_Dyn,Speedup" << std::endl;
    
    std::cout << "Starting benchmark. Results will be saved to 'results.csv'." << std::endl;
    std::cout << "Progress:" << std::endl;

    int verification_failures = 0;

    for(int i = 0; i < K; ++i) {
        auto& up = updates[i];
        
        std::cout << "\rProcessing update " << (i+1) << "/" << K << "..." << std::flush;
        
        // --- Static ---
        Stats::reset();
        auto startStatic = std::chrono::high_resolution_clock::now();
        staticAlgo.updateEdge(up.u, up.v, up.newW);
        auto endStatic = std::chrono::high_resolution_clock::now();
        long long timeStatic = std::chrono::duration_cast<std::chrono::microseconds>(endStatic - startStatic).count();
        long long opsStatic = Stats::heap_ops;
        long long relaxStatic = Stats::relaxed_edges;

        // --- Dynamic ---
        Stats::reset();
        auto startDyn = std::chrono::high_resolution_clock::now();
        dynAlgo.updateEdge(up.u, up.v, up.newW);
        auto endDyn = std::chrono::high_resolution_clock::now();
        long long timeDyn = std::chrono::duration_cast<std::chrono::microseconds>(endDyn - startDyn).count();
        long long opsDyn = Stats::heap_ops;
        long long relaxDyn = Stats::relaxed_edges;

        // --- Verification ---
        // Check random nodes
        bool ok = true;
        for(int t = 0; t < N; t+= N/10 + 1) {
            if(staticAlgo.getDist(t) != dynAlgo.getDist(t)) {
                ok = false;
                break;
            }
        }
        if(!ok) verification_failures++;

        // Speedup
        double speedup = (timeDyn > 0) ? (double)timeStatic / timeDyn : 0.0;

        csvFile << i << "," << up.type << "," 
                  << timeStatic << "," << opsStatic << "," << relaxStatic << ","
                  << timeDyn << "," << opsDyn << "," << relaxDyn << ","
                  << std::fixed << std::setprecision(2) << speedup << "\n";
                  
        // Optional: flush to ensure data is safe even if user Ctrl+C
        csvFile.flush();
    }
    std::cout << std::endl; // Newline after progress bar

    if(verification_failures > 0) {
        std::cerr << "WARNING: " << verification_failures << " verification failures detected!" << std::endl;
    } else {
        std::cout << "Benchmark completed. Verification passed." << std::endl;
    }

    csvFile.close();
    return 0;
}
