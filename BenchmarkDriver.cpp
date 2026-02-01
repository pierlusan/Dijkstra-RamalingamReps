/**
 * BenchmarkDriver.cpp
 * 
 * Scientific Benchmarking Framework for SSSP Algorithms.
 * Loads graphs from files in a folder and compares Static vs Dynamic algorithms.
 * 
 * Usage: ./benchmark <folder_path>
 * 
 * Supported formats:
 *  - .txt: First line = N (num vertices), then lines of "u v w" edges
 *  - .gr (DIMACS): "p sp N M" header, "a u v w" edges
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <random>
#include <memory>
#include <iomanip>
#include <cassert>
#include <algorithm>
#include <filesystem>
#include <sstream>

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



namespace fs = std::filesystem;

// --- Graph Loading Utilities ---

// Load graph from .txt format: first line = N, then "u v w" edges
std::pair<int, std::vector<Edge>> loadGraphFromTxt(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    
    int N;
    file >> N;
    
    std::vector<Edge> edges;
    int u, v;
    long long w;
    while (file >> u >> v >> w) {
        edges.push_back({u, v, w});
    }
    
    return {N, edges};
}

// Load graph from DIMACS .gr format: "p sp N M" header, "a u v w" edges
std::pair<int, std::vector<Edge>> loadGraphFromGR(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    
    std::string line;
    int N = 0;
    std::vector<Edge> edges;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        if (line[0] == 'c') {
            // Comment line, skip
            continue;
        } else if (line[0] == 'p') {
            // Problem line: p sp N M
            std::istringstream iss(line);
            std::string p, sp;
            int M;
            iss >> p >> sp >> N >> M;
            edges.reserve(M);
        } else if (line[0] == 'a') {
            // Arc line: a u v w
            std::istringstream iss(line);
            char a;
            int u, v;
            long long w;
            iss >> a >> u >> v >> w;
            // DIMACS uses 1-based indexing, convert to 0-based
            edges.push_back({u - 1, v - 1, w});
        }
    }
    
    return {N, edges};
}

// Auto-detect format and load graph
std::pair<int, std::vector<Edge>> loadGraph(const std::string& filepath) {
    std::string ext = fs::path(filepath).extension().string();
    
    if (ext == ".gr") {
        return loadGraphFromGR(filepath);
    } else {
        // Default to .txt format
        return loadGraphFromTxt(filepath);
    }
}

// Build Graph object from edges
void buildGraph(Graph& g, const std::vector<Edge>& edges) {
    for (const auto& e : edges) {
        g.addEdge(e.u, e.v, static_cast<int>(e.w));
    }
}

// --- Benchmark Logic ---

struct UpdateCase {
    int u, v;
    int oldW, newW;
    std::string type;      // "Inc", "Dec", "Add", "Del"
    std::string magnitude; // "Small", "Large", "Struct"
};

// Magnitude options: "small" = ±10%, "large" = ×2 or /2, "mixed" = random
UpdateCase generateUpdate(Graph& g, std::mt19937& rng, const std::string& magnitudeOpt) {
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
            
            // Determine actual magnitude for this update
            std::string actualMag = magnitudeOpt;
            if (magnitudeOpt == "mixed") {
                std::uniform_int_distribution<int> magDist(0, 1);
                actualMag = (magDist(rng) == 0) ? "small" : "large";
            }
            
            std::uniform_int_distribution<int> typeDist(0, 1);
            if (typeDist(rng) == 0) { // Dec
                if (w <= 1) continue;
                uc.type = "Dec";
                if (actualMag == "small") {
                    // Small: decrease by 10%
                    uc.newW = std::max(1, w - w / 10);
                    uc.magnitude = "Small";
                } else {
                    // Large: halve
                    uc.newW = std::max(1, w / 2);
                    uc.magnitude = "Large";
                }
            } else { // Inc
                uc.type = "Inc";
                if (actualMag == "small") {
                    // Small: increase by 10%
                    uc.newW = w + std::max(1, w / 10);
                    uc.magnitude = "Small";
                } else {
                    // Large: double
                    uc.newW = w * 2;
                    uc.magnitude = "Large";
                }
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

void run_benchmark_for_graph(const std::string& filepath, int K_updates, const std::string& magnitudeOpt) {
    std::string filename = fs::path(filepath).filename().string();
    
    std::cerr << "Loading graph: " << filename << std::endl;
    
    auto [N, edges] = loadGraph(filepath);
    int M = static_cast<int>(edges.size());
    
    std::cerr << "  Vertices: " << N << ", Edges: " << M << std::endl;
    
    int source_node = 0;
    
    // Setup Graphs (two copies for static and dynamic)
    Graph g_static(N);
    Graph g_dyn(N);
    buildGraph(g_static, edges);
    buildGraph(g_dyn, edges);
    
    // Setup Wrappers
    DijkstraWrapperImpl staticAlgo(g_static);
    RRWrapper dynAlgo(g_dyn);
    
    std::cerr << "  Initializing algorithms..." << std::endl;
    staticAlgo.init(source_node);
    dynAlgo.init(source_node);
    
    std::mt19937 rng(12345); // Benchmark loop rng
    
    std::cerr << "  Running " << K_updates << " updates..." << std::endl;
    
    for (int k = 0; k < K_updates; ++k) {
        UpdateCase uc = generateUpdate(g_static, rng, magnitudeOpt);
        //std::cerr << "update generati"<< std::endl;
        // --- Static Measurement ---
        Stats::reset();
        auto startS = std::chrono::high_resolution_clock::now();
        if (uc.type == "Add") staticAlgo.addEdge(uc.u, uc.v, uc.newW);
        else if (uc.type == "Del") staticAlgo.removeEdge(uc.u, uc.v);
        else staticAlgo.updateEdge(uc.u, uc.v, uc.newW);
        auto endS = std::chrono::high_resolution_clock::now();
        long long t_static = std::chrono::duration_cast<std::chrono::nanoseconds>(endS - startS).count();
        // Capture all individual static metrics
        long long heap_static = Stats::heap_ops;
        long long scanned_static = Stats::scanned_edges;
        long long visited_static = Stats::visited_nodes;
        long long relaxed_static = Stats::relaxed_edges;
        long long affected_static = Stats::affected_nodes;
        
        // --- Dynamic Measurement ---
        Stats::reset();
        auto startD = std::chrono::high_resolution_clock::now();
        if (uc.type == "Add") dynAlgo.addEdge(uc.u, uc.v, uc.newW);
        else if (uc.type == "Del") dynAlgo.removeEdge(uc.u, uc.v);
        else dynAlgo.updateEdge(uc.u, uc.v, uc.newW);
        auto endD = std::chrono::high_resolution_clock::now();
        long long t_dyn = std::chrono::duration_cast<std::chrono::nanoseconds>(endD - startD).count();
        // Capture all individual dynamic metrics
        long long heap_dyn = Stats::heap_ops;
        long long scanned_dyn = Stats::scanned_edges;
        long long visited_dyn = Stats::visited_nodes;
        long long relaxed_dyn = Stats::relaxed_edges;
        long long affected_dyn = Stats::affected_nodes;
        
        // --- Validation (sample 10 random nodes) ---
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
                  << t_static << "," << heap_static << "," << scanned_static << "," 
                  << visited_static << "," << relaxed_static << "," << affected_static << ","
                  << t_dyn << "," << heap_dyn << "," << scanned_dyn << ","
                  << visited_dyn << "," << relaxed_dyn << "," << affected_dyn << ","
                  << std::fixed << std::setprecision(2) << speedup << "\n";
    }
    
    std::cerr << "  Completed!" << std::endl;
}

void run_benchmark_suite(const std::string& folder_path, int K_updates, const std::string& magnitudeOpt) {
    std::cout << "Graph_N,Graph_M,Update_ID,Type,Magnitude,"
              << "Time_Static_ns,HeapOps_Static,ScannedEdges_Static,VisitedNodes_Static,RelaxedEdges_Static,AffectedNodes_Static,"
              << "Time_Dyn_ns,HeapOps_Dyn,ScannedEdges_Dyn,VisitedNodes_Dyn,RelaxedEdges_Dyn,AffectedNodes_Dyn,"
              << "Speedup" << std::endl;
    
    std::vector<std::string> graph_files;
    
    for (const auto& entry : fs::directory_iterator(folder_path)) {
        if (!entry.is_regular_file()) continue;
        
        std::string ext = entry.path().extension().string();
        // Only process .txt and .gr files (exclude expected_*.txt and update files)
        std::string filename = entry.path().filename().string();
        if ((ext == ".txt" || ext == ".gr") && 
            filename.find("expected") == std::string::npos &&
            filename.find("update") == std::string::npos) {
            graph_files.push_back(entry.path().string());
        }
    }
    
    // Sort for consistent ordering
    std::sort(graph_files.begin(), graph_files.end());
    
    std::cerr << "Found " << graph_files.size() << " graph files to process." << std::endl;
    
    for (const auto& filepath : graph_files) {
        run_benchmark_for_graph(filepath, K_updates, magnitudeOpt);
    }
}

int main(int argc, char* argv[]) {
    // Fast IO
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <folder_path> [num_updates] [magnitude]" << std::endl;
        std::cerr << "  folder_path: Path to folder containing graph files (.txt or .gr)" << std::endl;
        std::cerr << "  num_updates: Number of random updates per graph (default: 1000)" << std::endl;
        std::cerr << "  magnitude:   Update magnitude: small (±10%), large (×2 or /2), mixed (default: large)" << std::endl;
        return 1;
    }
    
    std::string folder_path = argv[1];
    int K_updates = (argc >= 3) ? std::stoi(argv[2]) : 1000;
    std::string magnitudeOpt = (argc >= 4) ? argv[3] : "large";
    
    // Validate magnitude option
    if (magnitudeOpt != "small" && magnitudeOpt != "large" && magnitudeOpt != "mixed") {
        std::cerr << "Error: magnitude must be 'small', 'large', or 'mixed'" << std::endl;
        return 1;
    }
    
    if (!fs::exists(folder_path) || !fs::is_directory(folder_path)) {
        std::cerr << "Error: '" << folder_path << "' is not a valid directory." << std::endl;
        return 1;
    }
    
    std::cerr << "Magnitude option: " << magnitudeOpt << std::endl;
    run_benchmark_suite(folder_path, K_updates, magnitudeOpt);
    return 0;
}
