
#include <iostream>
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

int main() {
    int numVertices = 1000;
    Graph g(numVertices);
    
    // Carichiamo un grafo (assicurati che il file esista e sia abbastanza grande)
    std::cout << "Caricamento grafo..." << std::endl;
    g.loadFromFile("/home/pierluca/Desktop/Algorithm-Engineering/Dijkstra/grafi/grafo40000000.txt"); // O grafo500.txt

    DijkstraSolver solverD(g);
    RamalingamReps solverR(g);

    std::cout << "\n--- 1. INIZIALIZZAZIONE ---" << std::endl;
    measure("Dijkstra (Statico)", [&]() { solverD.compute(0); });
    measure("Ramalingam (Init)",   [&]() { solverR.initialize(0); });
    compareResults(solverD, solverR, g.numVertices);

    // TEST 1: DECREMENTO PESO (Dovrebbe essere veloce)
    std::cout << "\n--- 2. TEST DECREMENTO PESO (0->1: 100 -> 10) ---" << std::endl;
    // Assicuriamoci che l'arco esista, o creiamolo se vogliamo testare quello
    // Per il test supponiamo esista. Se non esiste, lo inseriamo dopo.
    // Usiamo updateEdge diretto
    measure("Ramalingam Update", [&]() { solverR.handleEdgeUpdate(0, 1, 10); });
    measure("Dijkstra Update",   [&]() { solverD.compute(0); });
    compareResults(solverD, solverR, g.numVertices);

    // TEST 2: INCREMENTO PESO (Dovrebbe essere lento se rompe SPJO)
    std::cout << "\n--- 3. TEST INCREMENTO PESO (0->1: 10 -> 2000) ---" << std::endl;
    measure("Ramalingam Update", [&]() { solverR.handleEdgeUpdate(0, 1, 2000); });
    measure("Dijkstra Update",   [&]() { solverD.compute(0); });
    compareResults(solverD, solverR, g.numVertices);

    // TEST 3: INSERIMENTO NUOVO ARCO (Veloce come decremento)
    // Scegliamo due nodi a caso, es. 0 -> 50
    std::cout << "\n--- 4. TEST INSERIMENTO ARCO (0->50, w=5) ---" << std::endl;
    // Rimuoviamolo prima per sicurezza se c'era
    try { 
        if(g.getEdgeWeight(0, 50) != -1) {
            std::cout << "(Rimuovo arco esistente per test pulito...)" << std::endl;
            solverR.handleEdgeDeletion(0, 50); 
            solverD.compute(0);
        }
    } catch(...) {}

    measure("Ramalingam Insert", [&]() { solverR.handleEdgeInsertion(0, 50, 5); });
    measure("Dijkstra Update",   [&]() { solverD.compute(0); });
    compareResults(solverD, solverR, g.numVertices);

    // TEST 4: CANCELLAZIONE ARCO (Lento come incremento INF)
    std::cout << "\n--- 5. TEST CANCELLAZIONE ARCO (0->50) ---" << std::endl;
    measure("Ramalingam Delete", [&]() { solverR.handleEdgeDeletion(0, 50); });
    measure("Dijkstra Update",   [&]() { solverD.compute(0); });
    compareResults(solverD, solverR, g.numVertices);

    return 0;
}