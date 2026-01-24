#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <list>
#include <utility> // per std::pair
#include <string>

class Graph {
public:
    int numVertices;
    bool isUndirected;
    // Lista di adiacenza: vettore di liste di coppie (destinazione, peso)
    std::vector<std::vector<std::pair<int, int>>> adj;

    // Lista di adiacenza inversa: vettore di liste di coppie (sorgente, peso) -> per trovare i predecessori
    std::vector<std::vector<std::pair<int, int>>> rev_adj;

    // Costruttore
    Graph(int vertices, bool undirected = false);

    // Metodo per aggiungere archi 
    void addEdge(int u, int v, int weight);
    
    // Metodo per aggiornare il peso di un arco (necessario per algoritmi dinamici)
    void updateEdge(int u, int v, int newWeight);

    // Metodo per rimuovere un arco
    void removeEdge(int u, int v);

    // Metodo per caricare il grafo da file
    // Formato atteso: 
    // Prima riga: numero di vertici
    // Righe successive: u v w
    void loadFromFile(const std::string& filename);

    // Metodo per stampare il grafo (per debug)
    void printGraph();


    //lo stampa in formato dot e lo posso visualizzare https://dreampuf.github.io/GraphvizOnline
    void exportToDot();

    // Genera un'immagine PNG del grafo usando Graphviz
    void renderGraph(const std::string& outputName = "graph");
    
    // Restituisce il peso di un arco (u, v). Ritorna -1 se l'arco non esiste
    int getEdgeWeight(int u, int v) const;
    
    // Carica grafo da file DIMACS (9th DIMACS Challenge format)
    // Supporta file .gz compressi e file non compressi
    // Formato: linee "c" commenti, "p sp n m" header, "a u v w" archi
    void loadFromDIMACS(const std::string& filename);
};

#endif