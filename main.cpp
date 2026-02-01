
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <sstream>
#include "Graph.h"
#include "DijkstraSolver.h"

// Genera un grafo random sparso con n nodi e m = 4n archi
// Pesi random in [1, maxWeight]
void generateRandomGraph(Graph& g, int n, std::mt19937& rng, int maxWeight = 1000) {
    std::uniform_int_distribution<int> nodeDist(0, n - 1);
    std::uniform_int_distribution<int> weightDist(1, maxWeight);
    
    int m = 4 * n; // Grafo sparso: m = O(n)
    
    for (int i = 0; i < m; ++i) {
        int u = nodeDist(rng);
        int v = nodeDist(rng);
        if (u != v) { // Evita self-loop
            int w = weightDist(rng);
            g.addEdge(u, v, w);
        }
    }
}

// Salva il grafo in formato leggibile da Python (compatibile con fuzzing_oracle.py)
// Formato: prima riga = numero nodi, righe successive = "u v w"
void saveGraphToFile(Graph& g, const std::string& filename) {
    std::ofstream out(filename);
    out << g.numVertices << std::endl;
    for (int u = 0; u < g.numVertices; ++u) {
        for (auto& edge : g.adj[u]) {
            int v = edge.first;
            int w = edge.second;
            out << u << " " << v << " " << w << std::endl;
        }
    }
    out.close();
}



// Esegue Dijkstra, misura CPU time e salva le distanze
// Ritorna tempo in microsecondi
long long runDijkstraAndSave(Graph& g, int source, const std::string& outputFile) {
    DijkstraSolver solver(g);
    
    std::clock_t start = std::clock();
    solver.compute(source);
    std::clock_t end = std::clock();
    
    long long time_us = static_cast<long long>((end - start) * 1e6 / CLOCKS_PER_SEC);
    
    // Salva distanze per verifica correttezza
    std::ofstream out(outputFile);
    out << "node,distance" << std::endl;
    for (int i = 0; i < g.numVertices; ++i) {
        int dist = solver.getDistance(i);
        out << i << "," << (dist == std::numeric_limits<int>::max() ? -1 : dist) << std::endl;
    }
    out.close();
    
    return time_us;
}

// Solo misura tempo (per run successivi senza salvare)
long long measureCPUTime(Graph& g, int source) {
    DijkstraSolver solver(g);
    
    std::clock_t start = std::clock();
    solver.compute(source);
    std::clock_t end = std::clock();
    
    return static_cast<long long>((end - start) * 1e6 / CLOCKS_PER_SEC);
}

int main() {
    std::cout << "=== DOUBLING EXPERIMENT: Dijkstra O(m log n) ===" << std::endl;
    std::cout << "Configurazione: m = 4n (grafo sparso)" << std::endl << std::endl;
    
    // Range di n: 1K -> 1M (potenze di 2)
    std::vector<int> sizes = {1000, 2000, 4000, 8000, 16000, 32000, 
                               64000, 128000, 256000, 512000, 1024000};
    
    // Seed fisso per riproducibilità
    std::mt19937 rng(42);
    
    // Crea directory per risultati
    system("mkdir -p dijkstra_results");
    
    // Apri file CSV per output tempi
    std::ofstream csv("dijkstra_doubling.csv");
    csv << "n,m,time_us,ratio,expected_ratio" << std::endl;
    
    // Stampa header tabella
    std::cout << std::setw(10) << "n" 
              << std::setw(12) << "m" 
              << std::setw(15) << "time (µs)"
              << std::setw(12) << "ratio"
              << std::setw(15) << "expected" << std::endl;
    std::cout << std::string(64, '-') << std::endl;
    
    long long prevTime = 0;
    int prevN = 0;
    
    for (int n : sizes) {
        int m = 4 * n;
        
        // Crea e popola grafo
        Graph g(n);
        generateRandomGraph(g, n, rng);
        
        // Salva grafo per verifica con Python/networkx
        std::ostringstream graphFileName;
        graphFileName << "dijkstra_results/graph_n" << n << ".txt";
        saveGraphToFile(g, graphFileName.str());
        
        // Prima run: salva risultati per verifica correttezza
        std::ostringstream distFileName;
        distFileName << "dijkstra_results/distances_n" << n << ".csv";
        long long firstTime = runDijkstraAndSave(g, 0, distFileName.str());

        
        // Run aggiuntivi per media (senza salvare)
        int totalRuns = 3;
        long long totalTime = firstTime;
        for (int r = 1; r < totalRuns; ++r) {
            totalTime += measureCPUTime(g, 0);
        }
        long long avgTime = totalTime / totalRuns;
        
        // Calcola ratio
        double ratio = (prevTime > 0) ? static_cast<double>(avgTime) / prevTime : 0;
        
        // Calcola ratio atteso: 2 * log(2n) / log(n) per O(n log n)
        double expectedRatio = (prevN > 0) 
            ? 2.0 * std::log(static_cast<double>(n)) / std::log(static_cast<double>(prevN))
            : 0;
        
        // Stampa riga
        std::cout << std::setw(10) << n 
                  << std::setw(12) << m 
                  << std::setw(15) << avgTime;
        
        if (prevTime > 0) {
            std::cout << std::setw(12) << std::fixed << std::setprecision(2) << ratio
                      << std::setw(15) << std::fixed << std::setprecision(2) << expectedRatio;
        } else {
            std::cout << std::setw(12) << "-" << std::setw(15) << "-";
        }
        std::cout << std::endl;
        
        // Scrivi CSV
        csv << n << "," << m << "," << avgTime << ",";
        if (prevTime > 0) {
            csv << std::fixed << std::setprecision(3) << ratio << ","
                << std::fixed << std::setprecision(3) << expectedRatio;
        } else {
            csv << "-,-";
        }
        csv << std::endl;
        
        prevTime = avgTime;
        prevN = n;
    }
    
    csv.close();
    
    std::cout << std::endl;
    std::cout << "Risultati tempi salvati in: dijkstra_doubling.csv" << std::endl;
    std::cout << "Distanze salvate in: dijkstra_results/distances_n*.csv" << std::endl;
    std::cout << "\nPer plottare i risultati: python3 plot_dijkstra.py" << std::endl;
    
    return 0;
}