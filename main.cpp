
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include "Graph.h"
#include "DijkstraSolver.h"
#include "RamalingamReps.h"
#include <chrono>
#include <iomanip>


// Funzione helper per confrontare i risultati
bool compareResults(DijkstraSolver& d, RamalingamReps& r, int numVertices) {
    bool match = true;
    //std::cout << "\nConfronto Risultati:" << std::endl;
    //std::cout << "Nodo | Dijkstra | Ramalingam | Status" << std::endl;
    //std::cout << "-----|----------|------------|-------" << std::endl;

    for (int i = 0; i < numVertices; ++i) {
        int distD = d.getDistance(i);
        int distR = r.getDistance(i);
        
        bool currentMatch = (distD == distR);
        /*
        std::cout << std::setw(4) << i << " | " 
                  << std::setw(8) << (distD == INF ? -1 : distD) << " | " 
                  << std::setw(10) << (distR == INF ? -1 : distR) << " | "
                  << (currentMatch ? "✓" : "✗") << std::endl;
        */
        if (!currentMatch) match = false;
    }

    if (match) std::cout << "CHECK OK: Tutti i percorsi coincidono." << std::endl;
    else std::cout << "CHECK FAIL: Rilevate differenze!" << std::endl;
    return match;
}

// Helper per misurare e stampare
template<typename Func>
long long measure(const std::string& name, Func func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout <<  name << ": " << duration << " µs" << std::endl;
    return duration;
}




// Funzione per verificare la correttezza di Dijkstra confrontando con l'oracolo
bool verifyDijkstraFromFile(Graph& g, const std::string& expectedDistFile) {
    std::cout << "Verifica Dijkstra vs Oracle (" << expectedDistFile << ")..." << std::endl;
    std::ifstream infile(expectedDistFile);
    if (!infile.is_open()) {
        std::cerr << "ERRORE: Impossibile aprire " << expectedDistFile << std::endl;
        return false;
    }

    DijkstraSolver solver(g);
    solver.compute(0); // Assumiamo sorgente 0 come da oracolo

    int node, expectedDist;
    bool allMatch = true;
    while (infile >> node >> expectedDist) {
        int computedDist = solver.getDistance(node);
        // L'oracolo usa -1 per infinito, noi usiamo INF (che è std::numeric_limits<int>::max())
        // Dobbiamo normalizzare per il confronto
        int normalizedComputed = (computedDist == std::numeric_limits<int>::max()) ? -1 : computedDist;

        if (normalizedComputed != expectedDist) {
            std::cout << "FAIL: Nodo " << node 
                      << " | Atteso: " << expectedDist 
                      << " | Ottenuto: " << normalizedComputed << std::endl;
            allMatch = false;
        }
    }
    
    if (allMatch) {
        std::cout << "PASS: Dijkstra corrisponde all'oracolo!" << std::endl;
    } else {
        std::cout << "FAIL: Trovate discrepanze." << std::endl;
    }
    return allMatch;
}

// Funzione per verificare Ramalingam-Reps con l'aggiornamento dinamico
bool verifyRamalingamFromFile(Graph& g, const std::string& updateFile, const std::string& expectedDistFile) {
    std::cout << "Verifica Ramalingam vs Oracle..." << std::endl;
    
    // 1. Inizializza Ramalingam (dovrebbe essere già stato fatto o facciamolo qui)
    // Per un test pulito, assumiamo di partire da uno stato corretto.
    // Eseguiamo prima un Dijkstra/Init per sicurezza nel caso il grafo g passato sia "vergine"
    RamalingamReps solver(g);
    solver.initialize(0);

    // 2. Leggi l'aggiornamento
    std::ifstream udpFile(updateFile);
    if (!udpFile.is_open()) {
        std::cerr << "ERRORE: Impossibile aprire " << updateFile << std::endl;
        return false;
    }
    
    int u, v, oldW, newW;
    // Il file contiene solo una riga: "u v old_w new_w"
    if (!(udpFile >> u >> v >> oldW >> newW)) {
        std::cerr << "ERRORE: Formato update non valido" << std::endl;
        return false;
    }
    
    std::cout << "Applico update oracolo: " << u << "->" << v << " (" << oldW << " -> " << newW << ")" << std::endl;
    
    // 3. Applica l'aggiornamento dinamico
    solver.handleEdgeUpdate(u, v, newW);

    // 4. Confronta con expected_final_dist.txt
    std::ifstream expFile(expectedDistFile);
    if (!expFile.is_open()) {
        std::cerr << "ERRORE: Impossibile aprire " << expectedDistFile << std::endl;
        return false;
    }

    int node, expectedDist;
    bool allMatch = true;
    while (expFile >> node >> expectedDist) {
        int computedDist = solver.getDistance(node);
        int normalizedComputed = (computedDist == std::numeric_limits<int>::max()) ? -1 : computedDist;

        if (normalizedComputed != expectedDist) {
            std::cout << "FAIL (Ramalingam): Nodo " << node 
                      << " | Atteso (Oracle): " << expectedDist 
                      << " | Ottenuto: " << normalizedComputed << std::endl;
            allMatch = false;
        }
    }

    if (allMatch) {
        std::cout << "PASS: Ramalingam corrisponde all'oracolo!" << std::endl;
    } else {
        std::cout << "FAIL: Ramalingam ha discrepanze." << std::endl;
    }
    return allMatch;
}

int main() {
    // --- PART 1: AUTO-VERIFICATION ---
    std::cout << "=== AUTO-VERIFICATION PHASE ===" << std::endl;
    Graph gTest(1); // Placeholder dimension, will be resized
    try {
        // --- TEST 1: STATIC (DIJKSTRA) ---
        gTest.loadFromFile("test_case_graph.txt");
        bool staticOk = verifyDijkstraFromFile(gTest, "expected_init_dist.txt");
        
        // --- TEST 2: DYNAMIC (RAMALINGAM) ---
        // Nota: verifyRamalingamFromFile ri-inizializza solver su gTest, che è invariato finora.
        // Poi applica l'update su gTest.
        if (staticOk) {
            std::cout << "-------------------------------" << std::endl;
            verifyRamalingamFromFile(gTest, "test_case_update.txt", "expected_final_dist.txt");
        } else {
            std::cout << "SKIP: Ramalingam test saltato perché Dijkstra ha fallito." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Errore durante il test verifica: " << e.what() << std::endl;
        std::cerr << "Assicurati di aver generato i file con 'python3 fuzzing_oracle.py'" << std::endl;
    }
    // --- PART 2: TEST SU GRAFO DIMACS ---
    std::cout << "\n=== TEST SU GRAFO DIMACS ===" << std::endl;
    
    Graph gDimacs(1); // Placeholder, verrà ridimensionato
    try {
        std::cout << "Caricamento grafo DIMACS..." << std::endl;
        auto loadStart = std::chrono::high_resolution_clock::now();
        gDimacs.loadFromDIMACS("/home/pierluca/Desktop/Algorithm-Engineering/Dijkstra/grafi/USA-road-d.W.gr");
        auto loadEnd = std::chrono::high_resolution_clock::now();
        auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count();
        std::cout << "Tempo caricamento: " << loadTime << " ms" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Errore caricamento DIMACS: " << e.what() << std::endl;
        return 1;
    }
    
    int source = 0; // Nodo sorgente per SSSP
    
    // --- TEST DIJKSTRA ---
    std::cout << "\n--- DIJKSTRA ---" << std::endl;
    DijkstraSolver dijkstraSolver(gDimacs);
    long long dijkstraTime = measure("Dijkstra SSSP", [&]() { 
        dijkstraSolver.compute(source); 
    });
    
    // Stampa alcune distanze di esempio
    std::cout << "Distanze da nodo " << source << ":" << std::endl;
    for (int i = 0; i < std::min(5, gDimacs.numVertices); i++) {
        int dist = dijkstraSolver.getDistance(i);
        std::cout << "  -> Nodo " << i << ": " << (dist == std::numeric_limits<int>::max() ? -1 : dist) << std::endl;
    }
    
    // --- TEST RAMALINGAM-REPS ---
    std::cout << "\n--- RAMALINGAM-REPS ---" << std::endl;
    RamalingamReps ramalingamSolver(gDimacs);
    long long ramalingamInitTime = measure("Ramalingam Init", [&]() { 
        ramalingamSolver.initialize(source); 
    });
    
    // Verifica che i risultati iniziali coincidano
    std::cout << "\nConfronto risultati iniziali..." << std::endl;
    bool initMatch = compareResults(dijkstraSolver, ramalingamSolver, gDimacs.numVertices);
    
    if (!initMatch) {
        std::cerr << "ERRORE: Risultati iniziali non coincidono!" << std::endl;
        return 1;
    }
    
    // --- TEST MULTIPLI ARCHI ---
    std::cout << "\n=== TEST MULTIPLI ARCHI ===" << std::endl;
    
    // Seleziona archi da testare: diversi a varie distanze dalla sorgente
    // Formato: (u, v, originalWeight, increase: true=aumento, false=diminuzione)
    std::vector<std::tuple<int, int, int, bool>> testEdges;
    
    // Trova archi a diverse "profondità" nel grafo
    std::vector<int> testNodes = {0, 100, 1000, 10000, 100000, 1000000, 3000000, 5000000};
    
    for (int node : testNodes) {
        if (node < gDimacs.numVertices && !gDimacs.adj[node].empty()) {
            int u = node;
            int v = gDimacs.adj[node][0].first;
            int w = gDimacs.adj[node][0].second;
            // Aggiungi test per AUMENTO peso
            testEdges.push_back({u, v, w, true});
            // Aggiungi test per DIMINUZIONE peso (ripristina peso originale / dimezza)
            testEdges.push_back({u, v, w, false});
        }
    }
    
    std::cout << "Testerò " << testEdges.size() << " operazioni (" 
              << testEdges.size()/2 << " aumenti + " << testEdges.size()/2 << " diminuzioni).\n" << std::endl;
    
    int testsPassed = 0;
    int testsFailed = 0;
    long long totalRamalingamTime = 0;
    long long totalDijkstraTime = 0;
    
    for (size_t i = 0; i < testEdges.size(); i++) {
        auto [u, v, origWeight, isIncrease] = testEdges[i];
        
        // Calcola nuovo peso: raddoppia per aumento, dimezza per diminuzione
        int currentWeight = gDimacs.getEdgeWeight(u, v);
        int newWeight;
        std::string opType;
        
        if (isIncrease) {
            newWeight = currentWeight * 2;
            opType = "AUMENTO";
        } else {
            newWeight = std::max(1, currentWeight / 2); // Minimo peso = 1
            opType = "DIMINUZIONE";
        }
        
        std::cout << "--- Test " << (i+1) << "/" << testEdges.size() << " [" << opType << "] ---" << std::endl;
        std::cout << "Arco: " << u << " -> " << v << " (peso: " << currentWeight << " -> " << newWeight << ")" << std::endl;
        
        // Applica update con Ramalingam
        auto rStart = std::chrono::high_resolution_clock::now();
        ramalingamSolver.handleEdgeUpdate(u, v, newWeight);
        auto rEnd = std::chrono::high_resolution_clock::now();
        long long rTime = std::chrono::duration_cast<std::chrono::microseconds>(rEnd - rStart).count();
        totalRamalingamTime += rTime;
        
        // Aggiorna grafo e ricalcola Dijkstra
        gDimacs.updateEdge(u, v, newWeight);
        DijkstraSolver dijkstraCheck(gDimacs);
        auto dStart = std::chrono::high_resolution_clock::now();
        dijkstraCheck.compute(source);
        auto dEnd = std::chrono::high_resolution_clock::now();
        long long dTime = std::chrono::duration_cast<std::chrono::microseconds>(dEnd - dStart).count();
        totalDijkstraTime += dTime;
        
        // Confronta
        bool match = true;
        for (int n = 0; n < gDimacs.numVertices; n++) {
            if (dijkstraCheck.getDistance(n) != ramalingamSolver.getDistance(n)) {
                match = false;
                if (testsFailed == 0) { // Mostra solo primo errore
                    std::cout << "  MISMATCH al nodo " << n 
                              << ": Dijkstra=" << dijkstraCheck.getDistance(n)
                              << ", Ramalingam=" << ramalingamSolver.getDistance(n) << std::endl;
                }
            }
        }
        
        if (match) {
            std::cout << "  ✓ PASS (Ramalingam: " << rTime << " µs, Dijkstra: " << dTime << " µs, Speedup: " 
                      << std::fixed << std::setprecision(2) << (double)dTime/rTime << "x)" << std::endl;
            testsPassed++;
        } else {
            std::cout << "  ✗ FAIL" << std::endl;
            testsFailed++;
        }
    }
    
    // --- RIEPILOGO ---
    std::cout << "\n=== RIEPILOGO FINALE ===" << std::endl;
    std::cout << "Test passati: " << testsPassed << "/" << testEdges.size() << std::endl;
    std::cout << "Test falliti: " << testsFailed << "/" << testEdges.size() << std::endl;
    std::cout << "\nTempi totali su " << testEdges.size() << " update:" << std::endl;
    std::cout << "  Ramalingam totale: " << totalRamalingamTime << " µs" << std::endl;
    std::cout << "  Dijkstra totale:   " << totalDijkstraTime << " µs" << std::endl;
    std::cout << "  Speedup medio:     " << std::fixed << std::setprecision(2) 
              << (double)totalDijkstraTime / totalRamalingamTime << "x" << std::endl;
    
    if (testsFailed > 0) {
        std::cout << "\n⚠️  ATTENZIONE: Alcuni test sono falliti!" << std::endl;
        return 1;
    } else {
        std::cout << "\n✅ Tutti i test passati!" << std::endl;
    }
    
    std::cout << "\n=== FINE TEST ===" << std::endl;
    return 0;
}