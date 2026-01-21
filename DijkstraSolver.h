#ifndef DIJKSTRASOLVER_H
#define DIJKSTRASOLVER_H

#include "Graph.h" // Il solver deve conoscere la forma del grafo
#include <vector>

class DijkstraSolver {
private:
    Graph& graph; // Riferimento al grafo (non copia)
    std::vector<int> dist;
    std::vector<int> parent;

public:
    // Costruttore: accetta il grafo su cui lavorare
    DijkstraSolver(Graph& g);

    // Esegue l'algoritmo a partire da source
    void compute(int source);

    // Restituisce il percorso (dopo aver fatto compute)
    std::vector<int> getPath(int target);
    
    // Restituisce la distanza minima verso target (dopo aver fatto compute)
    int getDistance(int target);
};

#endif