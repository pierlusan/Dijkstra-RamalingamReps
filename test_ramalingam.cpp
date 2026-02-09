//Obiettivo: 
// Verificare che RR produca risultati corretti rispetto a Dijkstra
// Validare la complessità O(||δ|| log ||δ||) dove ||δ|| = nodi affetti + archi incidenti

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <limits>
#include "Graph.h"
#include "DijkstraSolver.h"
#include "RamalingamReps.h"
#include "BenchmarkStats.h"

const int INF = std::numeric_limits<int>::max();

// Misura tempo CPU in microsecondi
template<typename Func>
long long measureCPUTime(Func func) {
    std::clock_t start = std::clock();
    func();
    std::clock_t end = std::clock();
    return static_cast<long long>((end - start) * 1e6 / CLOCKS_PER_SEC);
}

// Verifica correttezza: confronta RR con Dijkstra ricalcolato
bool verifyCorrectness(RamalingamReps& rr, Graph& g, int source) {
    DijkstraSolver dijkstra(g);
    dijkstra.compute(source);
    
    for (int i = 0; i < g.numVertices; ++i) {
        if (rr.getDistance(i) != dijkstra.getDistance(i)) {
            return false;
        }
    }
    return true;
}

// Tipi di operazione
enum OpType { WEIGHT_INCREASE, WEIGHT_DECREASE, EDGE_DELETE, EDGE_INSERT };
const char* opTypeStr[] = {"W+", "W-", "DEL", "INS"};

int main() {
    std::cout << "=== TEST RAMALINGAM-REPS: O(||δ|| log ||δ||) ===" << std::endl;
    std::cout << "Test: aumento/diminuzione peso, inserimento, eliminazione" << std::endl;
    
    // Carica grafo USA-road
    Graph g(1);
    std::string graphPath = "/home/pierluca/Desktop/Algorithm-Engineering/Dijkstra/grafi_rr/USA-road-d.W.gr";
    
    std::cout << "Caricamento grafo..." << std::endl;
    try {
        g.loadFromDIMACS(graphPath); // carica il grafo
    } catch (const std::exception& e) {
        std::cerr << "Errore caricamento: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Grafo caricato: " << g.numVertices << " nodi" << std::endl;
    
    // Inizializza RR
    int source = 0;
    std::cout << "Inizializzazione RR da sorgente " << source << "..." << std::endl;
    RamalingamReps rr(g);
    Stats::reset();
    auto initTime = measureCPUTime([&]() { rr.initialize(source); });
    std::cout << "Inizializzazione completata in " << initTime << " µs" << std::endl;
    
    // Prepara update a diverse distanze dalla sorgente
    std::vector<std::tuple<int, int, int, int>> testEdges; // (u, v, weight, dist_from_source)
    
    DijkstraSolver dijkstra(g);
    dijkstra.compute(source); // calcola distanze con Dijkstra per riferimento
    
    std::mt19937 rng(42); // seed
    
    // Raccoglie tutti gli archi con distanza finita dalla sorgente 
    for (int u = 0; u < g.numVertices; ++u) {
        int distU = dijkstra.getDistance(u);
        if (distU == INF) continue;
        
        for (auto& edge : g.adj[u]) {
            int v = edge.first;
            int w = edge.second;
            testEdges.push_back({u, v, w, distU});
        }
    }
    
    // Ordina per distanza e campiona uniformemente
    std::sort(testEdges.begin(), testEdges.end(), 
              [](const auto& a, const auto& b) { return std::get<3>(a) < std::get<3>(b); });
    
    // Seleziona ~120 archi (30 per ogni tipo di operazione)
    std::vector<std::tuple<int, int, int, int>> selectedEdges;
    int step = std::max(1, (int)testEdges.size() / 120);
    for (size_t i = 0; i < testEdges.size(); i += step) {
        selectedEdges.push_back(testEdges[i]);
        if (selectedEdges.size() >= 120) break;
    }
    
    std::cout << "\nTesterò " << selectedEdges.size() << " operazioni:" << std::endl;
    std::cout << "  - ~30 aumenti peso (W+)" << std::endl;
    std::cout << "  - ~30 diminuzioni peso (W-)" << std::endl;
    std::cout << "  - ~30 eliminazioni + reinserimenti (DEL)" << std::endl;
    std::cout << "  - ~30 inserimenti nuovo arco (INS)" << std::endl;
    
    // Apri CSV per output
    std::ofstream csv("rr_complexity.csv");
    csv << "update_id,op_type,edge_u,edge_v,old_w,new_w,dist_from_source,affected_nodes,affected_edges,delta_size,time_us,normalized_time,dijkstra_match" << std::endl;
    
    // Stampa header
    std::cout << std::endl;
    std::cout << std::setw(5) << "ID" 
              << std::setw(6) << "OP"
              << std::setw(10) << "|δ|"
              << std::setw(12) << "||δ||"
              << std::setw(12) << "time(µs)"
              << std::setw(12) << "normalized"
              << std::setw(8) << "OK" << std::endl;
    std::cout << std::string(65, '-') << std::endl;
    
    int passed = 0, failed = 0;
    int opCounts[4] = {0, 0, 0, 0}; // conta operazioni per tipo
    
    for (size_t i = 0; i < selectedEdges.size(); ++i) {
        auto [u, v, origW, distFromSource] = selectedEdges[i];
        
        // Determina tipo di operazione (ciclo tra i 4 tipi). Li alterna in ordine
        OpType opType = static_cast<OpType>(i % 4);
        
        int currentW = g.getEdgeWeight(u, v); // peso attuale
        int newW = currentW;
        std::string opDesc;
        
        // Reset stats
        Stats::reset();
        long long rr_time = 0;
        
        switch (opType) {
            case WEIGHT_INCREASE:
                if (currentW == -1) continue;
                newW = currentW * 2;
                rr_time = measureCPUTime([&]() {
                    rr.handleEdgeUpdate(u, v, newW);
                });
                break;
                
            case WEIGHT_DECREASE:
                if (currentW == -1) continue;
                newW = std::max(1, currentW / 2);
                rr_time = measureCPUTime([&]() {
                    rr.handleEdgeUpdate(u, v, newW);
                });
                break;
                
            case EDGE_DELETE:
                if (currentW == -1) continue;
                // Elimina e poi reinserisci per non alterare il grafo permanentemente
                rr_time = measureCPUTime([&]() {
                    rr.handleEdgeDeletion(u, v);
                });
                // Verifica dopo eliminazione
                {
                    bool correctAfterDel = verifyCorrectness(rr, g, source); // controlla se il risultato ottenuto è corretto
                    if (correctAfterDel) passed++; else failed++;
                    opCounts[opType]++;
                    
                    long long affectedNodes = Stats::affected_nodes;
                    long long affectedEdges = Stats::affected_edges;
                    long long deltaSize = affectedNodes + affectedEdges;
                    double normalizedTime = (deltaSize > 1) ? rr_time / (deltaSize * std::log2(deltaSize)) : 0;
                    
                    std::cout << std::setw(5) << (i+1)
                              << std::setw(6) << opTypeStr[opType]
                              << std::setw(10) << affectedNodes
                              << std::setw(12) << deltaSize
                              << std::setw(12) << rr_time
                              << std::setw(12) << std::fixed << std::setprecision(4) << normalizedTime
                              << std::setw(8) << (correctAfterDel ? "✓" : "✗") << std::endl;
                    
                    csv << (i+1) << "," << opTypeStr[opType] << "," << u << "," << v << "," 
                        << currentW << "," << 0 << "," << distFromSource << "," 
                        << affectedNodes << "," << affectedEdges << "," << deltaSize << "," 
                        << rr_time << "," << normalizedTime << "," << (correctAfterDel ? 1 : 0) << std::endl;
                }
                // Reinserisci per ripristinare stato
                Stats::reset();
                rr.handleEdgeInsertion(u, v, currentW); // reinserisce l'arco
                continue; 
                
            case EDGE_INSERT:
                // Inserisci un arco verso un nodo casuale vicino
                {
                    // Trova un nodo vicino senza arco esistente
                    int newV = (v + 1) % g.numVertices;
                    while (g.getEdgeWeight(u, newV) != -1 && newV != v) {
                        newV = (newV + 1) % g.numVertices;
                    }
                    if (newV == v) continue; // Non trovato
                    
                    int insertW = 100; // Peso fisso per inserimento
                    rr_time = measureCPUTime([&]() {
                        rr.handleEdgeInsertion(u, newV, insertW);
                    });
                    
                    // Verifica e poi rimuovi
                    bool correctAfterIns = verifyCorrectness(rr, g, source);
                    if (correctAfterIns) passed++; else failed++;
                    opCounts[opType]++;
                    
                    long long affectedNodes = Stats::affected_nodes;
                    long long affectedEdges = Stats::affected_edges;
                    long long deltaSize = affectedNodes + affectedEdges;
                    double normalizedTime = (deltaSize > 1) ? rr_time / (deltaSize * std::log2(deltaSize)) : 0;
                    
                    std::cout << std::setw(5) << (i+1)
                              << std::setw(6) << opTypeStr[opType]
                              << std::setw(10) << affectedNodes
                              << std::setw(12) << deltaSize
                              << std::setw(12) << rr_time
                              << std::setw(12) << std::fixed << std::setprecision(4) << normalizedTime
                              << std::setw(8) << (correctAfterIns ? "✓" : "✗") << std::endl;
                    
                    csv << (i+1) << "," << opTypeStr[opType] << "," << u << "," << newV << "," 
                        << 0 << "," << insertW << "," << distFromSource << "," 
                        << affectedNodes << "," << affectedEdges << "," << deltaSize << "," 
                        << rr_time << "," << normalizedTime << "," << (correctAfterIns ? 1 : 0) << std::endl;
                    
                    // Rimuovi per ripristinare stato
                    Stats::reset();
                    rr.handleEdgeDeletion(u, newV);
                }
                continue;
        }
        
        // Per WEIGHT_INCREASE e WEIGHT_DECREASE
        if (opType == WEIGHT_INCREASE || opType == WEIGHT_DECREASE) {
            long long affectedNodes = Stats::affected_nodes;
            long long affectedEdges = Stats::affected_edges;
            long long deltaSize = affectedNodes + affectedEdges;
            
            bool correct = verifyCorrectness(rr, g, source);
            if (correct) passed++; else failed++;
            opCounts[opType]++;
            
            // Tempo normalizzato: verifica O(||δ|| log ||δ||)
            double normalizedTime = (deltaSize > 1) ? rr_time / (deltaSize * std::log2(deltaSize)) : 0;
            
            std::cout << std::setw(5) << (i+1)
                      << std::setw(6) << opTypeStr[opType]
                      << std::setw(10) << affectedNodes
                      << std::setw(12) << deltaSize
                      << std::setw(12) << rr_time
                      << std::setw(12) << std::fixed << std::setprecision(4) << normalizedTime
                      << std::setw(8) << (correct ? "✓" : "✗") << std::endl;
            
            csv << (i+1) << "," << opTypeStr[opType] << "," << u << "," << v << "," 
                << currentW << "," << newW << "," << distFromSource << "," 
                << affectedNodes << "," << affectedEdges << "," << deltaSize << "," 
                << rr_time << "," << normalizedTime << "," << (correct ? 1 : 0) << std::endl;
        }
    }
    
    csv.close();
    
    // Riepilogo
    std::cout << std::endl;
    std::cout << "=== RIEPILOGO ===" << std::endl;
    std::cout << "Test passati: " << passed << "/" << (passed + failed) << std::endl;
    std::cout << "Test falliti: " << failed << "/" << (passed + failed) << std::endl;
    std::cout << "\nPer tipo operazione:" << std::endl;
    std::cout << "  Aumento peso (W+):    " << opCounts[0] << std::endl;
    std::cout << "  Diminuzione peso (W-): " << opCounts[1] << std::endl;
    std::cout << "  Eliminazione (DEL):   " << opCounts[2] << std::endl;
    std::cout << "  Inserimento (INS):    " << opCounts[3] << std::endl;
    std::cout << "\nRisultati salvati in: rr_complexity.csv" << std::endl;
    std::cout << "\nPer plottare: python3 plot_ramalingam.py" << std::endl;
    
    return (failed > 0) ? 1 : 0;
}
