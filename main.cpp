#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <iomanip>
#include <cmath>
#include <ctime>
#include "Graph.h"
#include "DijkstraSolver.h"

namespace fs = std::filesystem;

// Esegue Dijkstra, misura CPU time e salva le distanze
// Ritorna tempo in microsecondi
long long runDijkstraAndSave(Graph& g, int source, const std::string& outputFile) {
    DijkstraSolver solver(g);
    
    std::clock_t start = std::clock(); //inizio calcolo tempo
    solver.compute(source); //calcola le distanze
    std::clock_t end = std::clock(); //fine calcolo tempo
    
    long long time_us = static_cast<long long>((end - start) * 1e6 / CLOCKS_PER_SEC); //calcolo tempo in microsecondi
    
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

// Helper per estrarre il numero di nodi dal nome file (es. graph_n1000.txt -> 1000)
// Se non riesce, ritorna 0
int extractN(const std::string& filename) {
    std::regex re("n(\\d+)");
    std::smatch match;
    if (std::regex_search(filename, match, re)) {
        return std::stoi(match[1].str());
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <directory_grafi>" << std::endl;
        return 1;
    }
    // prendo il path della directory dei grafi
    std::string directoryPath = argv[1];
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        std::cerr << "Errore: " << directoryPath << " non è una directory valida." << std::endl;
        return 1;
    }

    std::cerr << "=== DIJKSTRA BENCHMARK ===" << std::endl;
    std::cerr << "Leggendo grafi da: " << directoryPath << std::endl << std::endl;

    // Raccogli file e ordinali per n
    struct GraphFile {
        std::string path;
        std::string name;
        int n;
    };
    std::vector<GraphFile> files;

    // prendo i file .txt e li metto in files
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.path().extension() == ".txt") {
            std::string filename = entry.path().filename().string();
            int n = extractN(filename);
            files.push_back({entry.path().string(), filename, n});
        }
    }

    // Ordina per n crescente
    std::sort(files.begin(), files.end(), [](const GraphFile& a, const GraphFile& b) {
        return a.n < b.n;
    });

    if (files.empty()) {
        std::cerr << "Nessun file .txt trovato in " << directoryPath << std::endl;
        return 1;
    }

    // Crea directory per risultati
    system("mkdir -p dijkstra_results");
    
    // Stampa output CSV su stdout
    std::cout << "n,m,time_us,ratio,expected_ratio,filename" << std::endl;
    
    // Stampa header tabella su stderr
    std::cerr << std::setw(10) << "n" 
              << std::setw(12) << "m" 
              << std::setw(15) << "time (µs)"
              << std::setw(12) << "ratio"
              << std::setw(15) << "expected" 
              << "   filename" << std::endl;
    std::cerr << std::string(80, '-') << std::endl;

    long long prevTime = 0; // tempo precedente
    int prevN = 0; // numero di nodi precedente

    for (const auto& file : files) {
        Graph g(1); // Inizializza con 1 nodo dummy (0 non è permesso dal costruttore)
        try {
            g.loadFromFile(file.path);
        } catch (const std::exception& e) {
            std::cerr << "Errore caricamento " << file.name << ": " << e.what() << std::endl;
            continue;
        }

        int n = g.numVertices; //numero nodi
        // Conta archi
        long long m = 0;
        for(const auto& list : g.adj) m += list.size();
        // Prima run: salva risultati per verifica correttezza
        // Salva nella stessa cartella dei grafi: directoryPath/distances_n{n}.csv
        std::ostringstream distFileName;
        // Costruisci path: directoryPath + / + distances_n + n + .csv
        // Gestisci slash finale se presente o meno
        std::string dir = directoryPath;
        if (dir.back() != '/') dir += "/";
        distFileName << dir << "distances_n" << n << ".csv";
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
        
        // Calcola ratio atteso: O(m log n)
        double expectedRatio = 0.0;
        if (prevN > 0) {
           expectedRatio = 2.0 * std::log(static_cast<double>(n)) / std::log(static_cast<double>(prevN));
        }

        // Stampa riga tabella su stderr
        std::cerr << std::setw(10) << n 
                  << std::setw(12) << m 
                  << std::setw(15) << avgTime;
        
        if (prevTime > 0) {
            std::cerr << std::setw(12) << std::fixed << std::setprecision(2) << ratio
                      << std::setw(15) << std::fixed << std::setprecision(2) << expectedRatio;
        } else {
            std::cerr << std::setw(12) << "-" << std::setw(15) << "-";
        }
        std::cerr << "   " << file.name << std::endl;

        // Scrivi riga CSV su stdout
        std::cout << n << "," << m << "," << avgTime << ",";
        if (prevTime > 0) {
            std::cout << std::fixed << std::setprecision(3) << ratio << ","
                      << std::fixed << std::setprecision(3) << expectedRatio << ",";
        } else {
            std::cout << "-,-,";
        }
        std::cout << file.name << std::endl;

        prevTime = avgTime;
        prevN = n;
    }

    std::cerr << std::endl;
    std::cerr << "Distanze salvate in: " << directoryPath << "/distances_*.csv" << std::endl;

    return 0;
}