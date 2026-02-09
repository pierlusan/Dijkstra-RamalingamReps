#include <iostream>
#include <vector>
#include <fstream>
#include <ctime>
#include <random>
#include <memory>
#include <iomanip>
#include <cassert>
#include <algorithm>
#include <filesystem>
#include <sstream>

#include "BenchmarkStats.h"
#include "Graph.h"



struct Edge {
    int u, v;
    long long w;
};

// I wrapper unificano l'interfaccia per algoritmi statici e dinamici (Adapter Pattern),
// permettendo al benchmark di testarli in modo intercambiabile. I wrapper statici gestiscono
// gli aggiornamenti ricalcolando da zero, mentre quelli dinamici usano la loro logica incrementale.
class ISSSPWrapper {
public:
    virtual ~ISSSPWrapper() = default;
    
    // Inizializza l'algoritmo con la struttura del grafo
    virtual void init(int source) = 0;
    
    // Gestisce un aggiornamento dinamico degli archi
    virtual void updateEdge(int u, int v, int newWeight) = 0;
    virtual void addEdge(int u, int v, int weight) = 0;
    virtual void removeEdge(int u, int v) = 0;
    
    // Restituisce la distanza del cammino minimo verso il target
    virtual int getDistance(int target) = 0;
    
    virtual std::string getName() const = 0;
};

#include "DijkstraSolver.h"
#include "RamalingamReps.h"




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
    
    // Getter per accedere ai dati SPT (usati dal benchmark SPT-aware)
    const std::vector<int>& getParent() const { return solver->getParent(); }
    const std::vector<int>& getDist() const { return solver->getDist(); }
    int getSource() const { return solver->getSource(); }
};


// per non riscriverlo ogni volta per intero
namespace fs = std::filesystem;


// Carica il grafo dal formato .txt: prima riga = N, poi archi "u v w"
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

// Carica il grafo dal formato DIMACS .gr: intestazione "p sp N M", archi "a u v w"
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
           
            continue;
        } else if (line[0] == 'p') {
           
            std::istringstream iss(line);
            std::string p, sp;
            int M;
            iss >> p >> sp >> N >> M;
            edges.reserve(M);
        } else if (line[0] == 'a') {
           
            std::istringstream iss(line);
            char a;
            int u, v;
            long long w;
            iss >> a >> u >> v >> w;
           
            edges.push_back({u - 1, v - 1, w});
        }
    }
    
    return {N, edges};
}

// Rileva automaticamente il formato e carica il grafo
std::pair<int, std::vector<Edge>> loadGraph(const std::string& filepath) {
    std::string ext = fs::path(filepath).extension().string();
    
    if (ext == ".gr") {
        return loadGraphFromGR(filepath);
    } else {
        // Default al formato .txt
        return loadGraphFromTxt(filepath);
    }
}

// Costruisce l'oggetto Graph dagli archi
void buildGraph(Graph& g, const std::vector<Edge>& edges) {
    for (const auto& e : edges) {
        g.addEdge(e.u, e.v, static_cast<int>(e.w));
    }
}

// --- Logica del Benchmark ---

struct UpdateCase {
    int u, v;
    int oldW, newW;
    std::string type;      // "Inc", "Dec", "Add", "Del"
    std::string magnitude; // "Small", "Large", "Struct", "SPT_root", "SPT_middle", "SPT_leaf"
};

// Informazioni su un nodo nel SPT per selezione archi a diverse profondità
struct SPTNodeInfo {
    int node;
    int depth;      // Profondità nel SPT (hop dalla sorgente)
    int parent;     // Nodo genitore nel SPT
    int edgeWeight; // Peso arco (parent, node)
};

// Costruisce informazioni SPT con profondità per ogni nodo raggiungibile
std::vector<SPTNodeInfo> buildSPTInfo(const RRWrapper& rr, const Graph& g) {
    std::vector<SPTNodeInfo> sptNodes;
    const auto& parent = rr.getParent();
    const auto& dist = rr.getDist();
    int source = rr.getSource();
    int n = g.numVertices;
    
    // Calcola profondità con BFS sull'albero SPT
    std::vector<int> depth(n, -1);
    std::queue<int> q;
    q.push(source);
    depth[source] = 0;
    
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v = 0; v < n; v++) {
            if (parent[v] == u && depth[v] == -1) {
                depth[v] = depth[u] + 1;
                q.push(v);
            }
        }
    }
    
    // Raccogli tutti i nodi SPT (esclusa la sorgente)
    for (int v = 0; v < n; v++) {
        if (parent[v] != -1 && v != source && depth[v] > 0) {
            int w = g.getEdgeWeight(parent[v], v);
            if (w > 0) {
                sptNodes.push_back({v, depth[v], parent[v], w});
            }
        }
    }
    
    return sptNodes;
}

// Opzioni di magnitudo: "small" = ±10%, "large" = ×2 o /2, "mixed" = casuale
UpdateCase generateUpdate(Graph& g, std::mt19937& rng, const std::string& magnitudeOpt) {
    std::uniform_int_distribution<int> nodeDist(0, g.numVertices - 1);
    std::uniform_int_distribution<int> opDist(0, 100); 
    
    while(true) {
        int op = opDist(rng); // Numero random 0-100
        
        // 50% Aggiornamento Peso, 25% Aggiunta, 25% Eliminazione
        
        if (op < 50) { // Weight Update
            int u = nodeDist(rng); // Numero random 0-100
            if (g.adj[u].empty()) continue;
            
            std::uniform_int_distribution<int> neighborDist(0, g.adj[u].size() - 1);
            auto& edge = g.adj[u][neighborDist(rng)];
            int v = edge.first;
            int w = edge.second;
            
            UpdateCase uc;
            uc.u = u; uc.v = v; uc.oldW = w;
            
            // se la magnitudo è mista, scelgo random
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

// Genera aggiornamenti su archi del SPT a diverse profondità per testare correttamente RR
// depthCategory: "root" (top 33%), "middle" (33-66%), "leaf" (bottom 33%), "mixed"
// updateType: "increase", "decrease", "mixed"
UpdateCase generateSPTUpdate(
    Graph& g, 
    const RRWrapper& rr,
    std::mt19937& rng, 
    const std::string& depthCategory,
    const std::string& updateType
) {
    auto sptNodes = buildSPTInfo(rr, g);
    
    if (sptNodes.empty()) {
        // Fallback a update casuale se SPT vuoto
        return generateUpdate(g, rng, "large");
    }
    
    // Ordina per profondità
    std::sort(sptNodes.begin(), sptNodes.end(), 
              [](const SPTNodeInfo& a, const SPTNodeInfo& b) { return a.depth < b.depth; });
    
    int maxDepth = sptNodes.back().depth;
    if (maxDepth == 0) maxDepth = 1; // Evita divisione per zero
    
    // Filtra per categoria di profondità
    std::vector<SPTNodeInfo> candidates;
    for (const auto& info : sptNodes) {
        double relativeDepth = (double)info.depth / maxDepth;
        
        bool include = false;
        if (depthCategory == "root" && relativeDepth <= 0.33) include = true;
        else if (depthCategory == "middle" && relativeDepth > 0.33 && relativeDepth <= 0.66) include = true;
        else if (depthCategory == "leaf" && relativeDepth > 0.66) include = true;
        else if (depthCategory == "mixed") include = true;
        
        if (include) candidates.push_back(info);
    }
    
    // Fallback se categoria vuota
    if (candidates.empty()) candidates = sptNodes;
    
    // Scegli un nodo candidato a caso
    std::uniform_int_distribution<int> candDist(0, candidates.size() - 1);
    const auto& chosen = candidates[candDist(rng)];
    
    UpdateCase uc;
    uc.u = chosen.parent;
    uc.v = chosen.node;
    uc.oldW = chosen.edgeWeight;
    
    // Determina tipo di update
    std::string actualType = updateType;
    if (updateType == "mixed") {
        std::uniform_int_distribution<int> typeDist(0, 1);
        actualType = (typeDist(rng) == 0) ? "increase" : "decrease";
    }
    
    if (actualType == "decrease") {
        uc.type = "Dec";
        uc.newW = std::max(1, uc.oldW / 2);  // Dimezza il peso
        uc.magnitude = "SPT_" + depthCategory;
    } else {
        uc.type = "Inc";
        uc.newW = uc.oldW * 2;  // Raddoppia il peso
        uc.magnitude = "SPT_" + depthCategory;
    }
    
    return uc;
}

void run_benchmark_for_graph(const std::string& filepath, double update_factor, const std::string& magnitudeOpt,
                             const std::string& sptMode, const std::string& depthOpt, const std::string& updateTypeOpt) {
    std::string filename = fs::path(filepath).filename().string();
    
    std::cerr << "Loading graph: " << filename << std::endl;
    
    auto [N, edges] = loadGraph(filepath);
    int M = static_cast<int>(edges.size());
    
    std::cerr << "  Vertices: " << N << ", Edges: " << M << std::endl;
    
    // Calcola il numero di aggiornamenti basato sulla dimensione del grafo (proporzionale a N)
    int K_updates = std::max(1, static_cast<int>(N * update_factor));

    int source_node = 0;
    
    // due grafi, uno statico e uno dinamico
    Graph g_static(N);
    Graph g_dyn(N);
    buildGraph(g_static, edges);
    buildGraph(g_dyn, edges);
    
    // due wrapper, uno statico e uno dinamico
    DijkstraWrapperImpl staticAlgo(g_static);
    RRWrapper dynAlgo(g_dyn);
    
    std::cerr << "  Initializing algorithms..." << std::endl;
    staticAlgo.init(source_node);
    dynAlgo.init(source_node);
    
    std::mt19937 rng(12345); // Benchmark loop rng, seed fisso per riproducibilità, servirà per generare gli update
    
    std::cerr << "  Running " << K_updates << " updates (Factor: " << update_factor << " * N)";
    if (sptMode == "spt") {
        std::cerr << " [SPT mode: depth=" << depthOpt << ", type=" << updateTypeOpt << "]";
    }
    std::cerr << "..." << std::endl;
    
    for (int k = 0; k < K_updates; ++k) {
        UpdateCase uc;
        
        // Scegli se usare update SPT-aware o casuale
        if (sptMode == "spt") {
            uc = generateSPTUpdate(g_static, dynAlgo, rng, depthOpt, updateTypeOpt);
        } else {
            uc = generateUpdate(g_static, rng, magnitudeOpt);
        }
        
        // misura statica
        Stats::reset();
        struct timespec startS, endS;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startS);
        if (uc.type == "Add") staticAlgo.addEdge(uc.u, uc.v, uc.newW);
        else if (uc.type == "Del") staticAlgo.removeEdge(uc.u, uc.v);
        else staticAlgo.updateEdge(uc.u, uc.v, uc.newW);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endS);
        long long t_static = (endS.tv_sec - startS.tv_sec) * 1000000000LL + (endS.tv_nsec - startS.tv_nsec);
        // misura metriche statiche
        long long heap_static = Stats::heap_ops;
        long long scanned_static = Stats::scanned_edges;
        long long visited_static = Stats::visited_nodes;
        long long relaxed_static = Stats::relaxed_edges;
        long long affected_static = Stats::affected_nodes;
        
        // misura dinamica
        Stats::reset();
        struct timespec startD, endD;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startD);
        if (uc.type == "Add") dynAlgo.addEdge(uc.u, uc.v, uc.newW);
        else if (uc.type == "Del") dynAlgo.removeEdge(uc.u, uc.v);
        else dynAlgo.updateEdge(uc.u, uc.v, uc.newW);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endD);
        long long t_dyn = (endD.tv_sec - startD.tv_sec) * 1000000000LL + (endD.tv_nsec - startD.tv_nsec);
        // misura metriche dinamiche
        long long heap_dyn = Stats::heap_ops;
        long long scanned_dyn = Stats::scanned_edges;
        long long visited_dyn = Stats::visited_nodes;
        long long relaxed_dyn = Stats::relaxed_edges;
        long long affected_dyn = Stats::affected_nodes;
        

        // Validazione: prendo 10 nodi a caso e controllo che le distanze siano uguali (da eliminare in futuro)
        // for (int i = 0; i < 10; ++i) {
        //     std::uniform_int_distribution<int> checkDist(0, N - 1);
        //     int node = checkDist(rng);
        //     if (staticAlgo.getDistance(node) != dynAlgo.getDistance(node)) {
        //          std::cerr << "Validation Failed at Update " << k << " Node " << node << std::endl;
        //          std::cerr << "Static: " << staticAlgo.getDistance(node) << " Dyn: " << dynAlgo.getDistance(node) << std::endl;
        //          exit(1);
        //     }
        // }
        
        // Calcolo speedup
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

void run_benchmark_suite(const std::string& folder_path, double update_factor, const std::string& magnitudeOpt,
                         const std::string& sptMode, const std::string& depthOpt, const std::string& updateTypeOpt) {
    std::cout << "Graph_N,Graph_M,Update_ID,Type,Magnitude,"
              << "Time_Static_ns,HeapOps_Static,ScannedEdges_Static,VisitedNodes_Static,RelaxedEdges_Static,AffectedNodes_Static,"
              << "Time_Dyn_ns,HeapOps_Dyn,ScannedEdges_Dyn,VisitedNodes_Dyn,RelaxedEdges_Dyn,AffectedNodes_Dyn,"
              << "Speedup" << std::endl;
    
    std::vector<std::string> graph_files;
    
    // Scansiona la cartella per file .txt e .gr (esclude expected_*, update_*)
    for (const auto& entry : fs::directory_iterator(folder_path)) {
        if (!entry.is_regular_file()) continue;
        
        std::string ext = entry.path().extension().string();
        // Processa solo file .txt e .gr (esclude expected_*.txt e file update)
        std::string filename = entry.path().filename().string();
        if ((ext == ".txt" || ext == ".gr") && 
            filename.find("expected") == std::string::npos &&
            filename.find("update") == std::string::npos) {
            graph_files.push_back(entry.path().string());
        }
    }
    
    // Ordina per nome file per consistenza
    std::sort(graph_files.begin(), graph_files.end());
    
    std::cerr << "Found " << graph_files.size() << " graph files to process." << std::endl;
    
    // Per ogni file esegue il benchmark
    for (const auto& filepath : graph_files) {
        run_benchmark_for_graph(filepath, update_factor, magnitudeOpt, sptMode, depthOpt, updateTypeOpt);
    }
}

int main(int argc, char* argv[]) {
    // I/O Veloce: Ottimizza drasticamente la velocità di Input/Output
    // Disabilita la sincronizzazione con gli stream C (printf/scanf) per performance
    std::ios_base::sync_with_stdio(false);
    // Slega cin da cout: evita il flush automatico del buffer di output ad ogni input (utile per grandi letture)
    std::cin.tie(NULL);
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <folder_path> [update_factor] [magnitude] [spt_mode] [depth] [update_type]" << std::endl;
        std::cerr << "  folder_path:   Path to folder containing graph files (.txt or .gr)" << std::endl;
        std::cerr << "  update_factor: Multiplier for updates relative to N (k = N * factor) (default: 1.0)" << std::endl;
        std::cerr << "  magnitude:     Update magnitude: small (±10%), large (×2 or /2), mixed (default: large)" << std::endl;
        std::cerr << "  spt_mode:      'random' (default) or 'spt' (select only SPT edges)" << std::endl;
        std::cerr << "  depth:         SPT depth category: root, middle, leaf, mixed (default: mixed)" << std::endl;
        std::cerr << "  update_type:   SPT update type: increase, decrease, mixed (default: mixed)" << std::endl;
        return 1;
    }
    // Prende il path della cartella da cui prendere i file
    std::string folder_path = argv[1];
    double update_factor = (argc >= 3) ? std::stod(argv[2]) : 1.0;
    std::string magnitudeOpt = (argc >= 4) ? argv[3] : "large";
    std::string sptMode = (argc >= 5) ? argv[4] : "spt";
    std::string depthOpt = (argc >= 6) ? argv[5] : "mixed";
    std::string updateTypeOpt = (argc >= 7) ? argv[6] : "mixed";
    
    // Controlla se la magnitudo è valida
    if (magnitudeOpt != "small" && magnitudeOpt != "large" && magnitudeOpt != "mixed") {
        std::cerr << "Error: magnitude must be 'small', 'large', or 'mixed'" << std::endl;
        return 1;
    }
    
    // Controlla se spt_mode è valido
    if (sptMode != "random" && sptMode != "spt") {
        std::cerr << "Error: spt_mode must be 'random' or 'spt'" << std::endl;
        return 1;
    }
    
    // Controlla se depth è valido
    if (depthOpt != "root" && depthOpt != "middle" && depthOpt != "leaf" && depthOpt != "mixed") {
        std::cerr << "Error: depth must be 'root', 'middle', 'leaf', or 'mixed'" << std::endl;
        return 1;
    }
    
    // Controlla se update_type è valido
    if (updateTypeOpt != "increase" && updateTypeOpt != "decrease" && updateTypeOpt != "mixed") {
        std::cerr << "Error: update_type must be 'increase', 'decrease', or 'mixed'" << std::endl;
        return 1;
    }
    
    if (!fs::exists(folder_path) || !fs::is_directory(folder_path)) {
        std::cerr << "Error: '" << folder_path << "' is not a valid directory." << std::endl;
        return 1;
    }
    
    std::cerr << "Magnitude option: " << magnitudeOpt << std::endl;
    if (sptMode == "spt") {
        std::cerr << "SPT mode: depth=" << depthOpt << ", update_type=" << updateTypeOpt << std::endl;
    }
    run_benchmark_suite(folder_path, update_factor, magnitudeOpt, sptMode, depthOpt, updateTypeOpt);
    return 0;
}
