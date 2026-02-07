#ifndef RAMALINGAM_REPS_H
#define RAMALINGAM_REPS_H

#include "Graph.h"
#include <vector>
#include <queue>
#include <set>

class RamalingamReps {
private:
    Graph& graph;
    std::vector<int> dist;  // d[u]: current distance estimate
    std::vector<int> rhs;   // rhs[u]: target value based on predecessors
    std::vector<int> parent; // Optional: to reconstruct path
    int sourceVertex;

    // Priority queue type: (key, node) with min-heap
    using PQEntry = std::pair<int, int>;
    using MinHeap = std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>>;
    
    // Global priority queue for inconsistent nodes
    MinHeap pq;

    // Helper functions
    void updateRhs(int u);       // Recompute rhs[u] from predecessors
    int computeKey(int u);       // Returns min(dist[u], rhs[u])
    void stabilize();            // Main fixpoint loop to restore consistency

public:
    RamalingamReps(Graph& g);

    // Inizializza l'algoritmo calcolando i percorsi minimi iniziali
    void initialize(int source);

    // Gestisce l'aggiornamento di un arco
    // Aggiorna automaticamente il grafo e ricalcola i percorsi minimi
    void handleEdgeUpdate(int u, int v, int newWeight);
    
    // Gestisce l'inserimento di un nuovo arco
    void handleEdgeInsertion(int u, int v, int w);

    // Gestisce la rimozione di un arco
    void handleEdgeDeletion(int u, int v);

    // Restituisce la distanza calcolata verso un nodo
    int getDistance(int target);
    
    // Getter per accedere ai dati SPT (usati dal benchmark)
    const std::vector<int>& getParent() const { return parent; }
    const std::vector<int>& getDist() const { return dist; }
    int getSource() const { return sourceVertex; }
};

#endif
