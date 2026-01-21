#ifndef RAMALINGAM_REPS_H
#define RAMALINGAM_REPS_H

#include "Graph.h"
#include <vector>
#include <queue>
#include <set>

class RamalingamReps {
private:
    Graph& graph;
    std::vector<int> dist;
    std::vector<int> parent; // Optional: to reconstruct path
    int sourceVertex;

    // Helper functions
    void dijkstra();
    bool recomputeNode(int u);

public:
    RamalingamReps(Graph& g);

    // Inizializza l'algoritmo calcolando i percorsi minimi iniziali
    void initialize(int source);

    // Gestisce l'aggiornamento di un arco
    // Aggiorna automaticamente il grafo e ricalcola i percorsi minimi
    // Se oldWeight non è specificato (-1), viene recuperato automaticamente dal grafo
    void handleEdgeUpdate(int u, int v, int newWeight);
    
    // Gestisce l'inserimento di un nuovo arco
    void handleEdgeInsertion(int u, int v, int w);

    // Gestisce la rimozione di un arco
    void handleEdgeDeletion(int u, int v);

    // Restituisce la distanza calcolata verso un nodo
    int getDistance(int target);
};

#endif
