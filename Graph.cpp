//
// Created by pierluca on 12/26/25.
//

#include "Graph.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> // per std::find_if
#include <zlib.h>    // per supporto file .gz

Graph::Graph(int vertices, bool undirected) : numVertices( vertices ), isUndirected( undirected ) {
    if (vertices <= 0) {
        throw std::invalid_argument("Il numero di vertici deve essere maggiore di 0.");
    }

    // Inizializza la lista di adiacenza, vuota per ogni vertice
    adj.resize(vertices);
    rev_adj.resize(vertices);
}

void Graph::addEdge(int u, int v, int weight) {
    if (u < 0 || u >= numVertices || v < 0 || v >= numVertices) {
        throw std::out_of_range("Indice del vertice fuori dai limiti.");
    }

    // Aggiungi l'arco dalla sorgente alla destinazione
    adj[u].emplace_back(v, weight);
    // Aggiungi l'arco inverso per tracciare i predecessori (v -> u)
    rev_adj[v].emplace_back(u, weight);

    // Se il grafo è non orientato, aggiungi anche l'arco inverso
    if (isUndirected) {
        adj[v].emplace_back(u, weight);
        rev_adj[u].emplace_back(v, weight);
    }
}


void Graph::updateEdge(int u, int v, int newWeight) {
    if (u < 0 || u >= numVertices || v < 0 || v >= numVertices) {
        throw std::out_of_range("Indice del vertice fuori dai limiti.");
    }

    // Lambda per aggiornare il peso in una lista di adiacenza
    auto updateList = [](std::vector<std::pair<int, int>>& list, int target, int weight) {
        for (auto& edge : list) {
            if (edge.first == target) {
                edge.second = weight;
                return true;
            }
        }
        return false;
    };

    bool found = updateList(adj[u], v, newWeight);
    if (!found) {
         // Se l'arco non esiste, potremmo decidere di lanciarlo o di aggiungerlo. 
         // Per ora assumiamo che updateEdge sia chiamabile solo su archi esistenti.
         throw std::runtime_error("Arco non trovato per update.");
    }
    
    // Aggiorna anche rev_adj
    updateList(rev_adj[v], u, newWeight);

    if (isUndirected) {
        updateList(adj[v], u, newWeight);
        updateList(rev_adj[u], v, newWeight);
    }
}

void Graph::removeEdge(int u, int v) {
    if (u < 0 || u >= numVertices || v < 0 || v >= numVertices) {
        throw std::out_of_range("Indice del vertice fuori dai limiti.");
    }

    // Lambda per rimuovere un arco da una lista
    auto removeFromList = [](std::vector<std::pair<int, int>>& list, int target) {
        auto it = std::find_if(list.begin(), list.end(), [target](const std::pair<int, int>& edge) {
            return edge.first == target;
        });
        if (it != list.end()) {
            list.erase(it);
        }
    };

    removeFromList(adj[u], v);
    removeFromList(rev_adj[v], u);

    if (isUndirected) {
        removeFromList(adj[v], u);
        removeFromList(rev_adj[u], v);
    }
}

void Graph::loadFromFile(const std::string& filename) {
    std::ifstream infile(filename);
    //controlla se ha aperto il file 
    if (!infile.is_open()) {
        throw std::runtime_error("Impossibile aprire il file: " + filename);
    }

    int n;
    if (!(infile >> n)) {
        throw std::runtime_error("Errore lettura numero vertici.");
    }

    // Reset del grafo con i nuovi vertici
    numVertices = n;
    adj.clear();
    rev_adj.clear();
    adj.resize(n);
    rev_adj.resize(n);

    int u, v, w;
    while (infile >> u >> v >> w) {
        if(w < 0){
            throw std::runtime_error("Peso negativo trovato!!!");
        }
        else{
            addEdge(u, v, w);
        }
    }

    infile.close();
    std::cerr << "Grafo caricato da " << filename << " con " << numVertices << " vertici." << std::endl;
}

void Graph::printGraph() {
    for (int i = 0; i < numVertices; ++i) {
        std::cout << "Vertice " << i << ":";
        for (const auto& neighbor : adj[i]) {
            std::cout << " -> (Destinazione: " << neighbor.first << ", Peso: " << neighbor.second << ")";
        }
        std::cout << std::endl;
    }
}

void Graph::exportToDot() {
    std::cout << "===== COPIA DA QUI SOTTO =====" << std::endl;

    if (isUndirected) {
        std::cout << "graph G {" << std::endl; // 'graph' per non orientati
        std::cout << "  // Per evitare duplicati visivi, stampiamo solo se u < v" << std::endl;
    } else {
        std::cout << "digraph G {" << std::endl; // 'digraph' per orientati
    }

    for (int u = 0; u < numVertices; u++) {
        for (const auto& edge : adj[u]) {
            int v = edge.first;
            int w = edge.second;

            // Logica per visualizzazione corretta
            if (isUndirected) {
                // Nei grafi non orientati, l'arco esiste sia in u->v che v->u.
                // Per il disegno ne basta uno solo, prendiamo quello dove u < v
                if (u < v) {
                    std::cout << "  " << u << " -- " << v << " [label=\"" << w << "\"];" << std::endl;
                }
            } else {
                // Nei grafi orientati stampiamo tutto
                std::cout << "  " << u << " -> " << v << " [label=\"" << w << "\"];" << std::endl;
            }
        }
    }
    std::cout << "}" << std::endl;
    std::cout << "===== FINO A QUI =====" << std::endl;
}

void Graph::renderGraph(const std::string& outputName) {
    // Crea la cartella output_grafi se non esiste
    int dirRes = system("mkdir -p output_grafi");
    (void)dirRes; // Silence unused variable warning
    
    std::string dotParam = "output_grafi/" + outputName + ".dot";
    std::string pngParam = "output_grafi/" + outputName + ".png";
    std::ofstream outfile(dotParam);

    if (!outfile.is_open()) {
        std::cerr << "Errore creazione file " << dotParam << std::endl;
        return;
    }

    if (isUndirected) {
        outfile << "graph G {" << std::endl;
    } else {
        outfile << "digraph G {" << std::endl;
    }

    for (int u = 0; u < numVertices; u++) {
        for (const auto& edge : adj[u]) {
            int v = edge.first;
            int w = edge.second;
            if (isUndirected) {
                if (u < v) {
                    outfile << "  " << u << " -- " << v << " [label=\"" << w << "\"];" << std::endl;
                }
            } else {
                outfile << "  " << u << " -> " << v << " [label=\"" << w << "\"];" << std::endl;
            }
        }
    }
    outfile << "}" << std::endl;
    outfile.close();

    std::string cmd = "dot -Tpng " + dotParam + " -o " + pngParam;
    int res = system(cmd.c_str());
    if (res == 0) {
        std::cout << "Grafo renderizzato in: " << pngParam << std::endl;
        // Prova ad aprirlo (Linux)
        std::string openCmd = "xdg-open " + pngParam + " > /dev/null 2>&1 &";
        int openRes = system(openCmd.c_str());
        (void)openRes; 
    } else {
        std::cerr << "Errore esecuzione comando dot. Assicurati che Graphviz sia installato." << std::endl;
    }
}

int Graph::getEdgeWeight(int u, int v) const {
    if (u < 0 || u >= numVertices || v < 0 || v >= numVertices) {
        return -1; // Indici non validi
    }
    
    // Cerca l'arco nella lista di adiacenza
    for (const auto& edge : adj[u]) {
        if (edge.first == v) {
            return edge.second; // Peso trovato
        }
    }
    
    return -1; // Arco non trovato
}

void Graph::loadFromDIMACS(const std::string& filename) {
    
    gzFile file = gzopen(filename.c_str(), "rb");
    if (!file) {
        throw std::runtime_error("Impossibile aprire il file: " + filename);
    }
    
    // Buffer per lettura efficiente
    constexpr size_t BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE];
    
    int n = 0, m = 0;
    bool headerFound = false;
    
    // Prima passata: trova l'header per pre-allocare
    while (gzgets(file, buffer, BUFFER_SIZE) != nullptr) {
        if (buffer[0] == 'c') {
            // Commento, ignora
            continue;
        } else if (buffer[0] == 'p') {
            // Header: "p sp <nodi> <archi>"
            if (sscanf(buffer, "p sp %d %d", &n, &m) == 2) {
                headerFound = true;
                break;
            }
        }
    }
    
    if (!headerFound || n <= 0) {
        gzclose(file);
        throw std::runtime_error("Header DIMACS non trovato o non valido.");
    }
    
    // Reset e pre-allocazione
    numVertices = n;
    adj.clear();
    rev_adj.clear();
    adj.resize(n);
    rev_adj.resize(n);
    
    // Stima archi per nodo per pre-allocazione (opzionale ma utile)
    // Media: m/n archi per nodo, ma distribuiti non uniformemente
    // Non pre-allochiamo le liste interne per evitare sprechi di memoria
    
    int edgesLoaded = 0;
    int u, v, w;
    
    // Parsing degli archi
    while (gzgets(file, buffer, BUFFER_SIZE) != nullptr) {
        if (buffer[0] == 'a') {
            // Arco: "a <sorgente> <destinazione> <peso>"
            if (sscanf(buffer, "a %d %d %d", &u, &v, &w) == 3) {
                // DIMACS usa indici 1-based, convertiamo a 0-based
                u--;
                v--;
                
                if (u < 0 || u >= n || v < 0 || v >= n) {
                    gzclose(file);
                    throw std::runtime_error("Indice vertice fuori range nel file DIMACS.");
                }
                
                if (w < 0) {
                    gzclose(file);
                    throw std::runtime_error("Peso negativo trovato nel file DIMACS.");
                }
                
                // Inserimento diretto (più efficiente di addEdge per caricamento massivo)
                adj[u].emplace_back(v, w);
                rev_adj[v].emplace_back(u, w);
                edgesLoaded++;
            }
        }
        // Ignora altre linee (commenti 'c', ecc.)
    }
    
    gzclose(file);
    
    std::cerr << "Grafo DIMACS caricato da " << filename << ": " 
              << numVertices << " nodi, " << edgesLoaded << " archi." << std::endl;
}