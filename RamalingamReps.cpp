#include "RamalingamReps.h"
#include <limits>
#include <algorithm>
#include <iostream>
#include <queue>
#include "BenchmarkStats.h" // BENCHMARK INSTRUMENTATION

const int INF = std::numeric_limits<int>::max();

RamalingamReps::RamalingamReps(Graph& g) : graph(g), sourceVertex(-1) {}

// Calcola la chiave per l'ordinamento nella coda di priorità: min(dist[u], rhs[u])
int RamalingamReps::computeKey(int u) {
    return std::min(dist[u], rhs[u]);
}

// Ricalcola rhs[u] in base ai predecessori
// rhs[u] = min su tutti i predecessori p di (dist[p] + weight(p, u))
void RamalingamReps::updateRhs(int u) {
    if (u == sourceVertex) {
        rhs[u] = 0;
        return;
    }
    
    int best = INF;
    int bestPar = -1;
    
    for (auto& edge : graph.rev_adj[u]) {
        int pred = edge.first;
        int w = edge.second;
        Stats::scanned_edges++; // BENCHMARK: Arco esaminato
        
        if (dist[pred] != INF) {
            long long newD = (long long)dist[pred] + w; // evita overflow
            if (newD < best) {
                best = (int)newD;
                bestPar = pred;
            }
        }
    }
    
    rhs[u] = best;
    // Aggiorna il genitore per la ricostruzione del cammino
    if (best != INF) {
        parent[u] = bestPar;
    }
}

// Ciclo principale di stabilizzazione: elabora i nodi inconsistenti finché tutti sono consistenti
// Un nodo è consistente quando dist[u] == rhs[u]
// Sovra-consistente: dist[u] > rhs[u] -> riduci dist a rhs
// Sotto-consistente: dist[u] < rhs[u] -> aumenta dist a INF
void RamalingamReps::stabilize() {
    while (!pq.empty()) {
        auto [key, u] = pq.top();
        pq.pop();
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
        
        // Salta le entry obsolete
        int currentKey = computeKey(u);
        if (key > currentKey) continue;
        
        // Salta i nodi già consistenti
        if (dist[u] == rhs[u]) continue;
        
        Stats::visited_nodes++; // BENCHMARK INSTRUMENTATION
        
        if (dist[u] > rhs[u]) {
            // Sovra-consistente: abbassa dist per corrispondere a rhs
            dist[u] = rhs[u];
            Stats::affected_nodes++; // BENCHMARK: Distanza del nodo cambiata
            Stats::affected_edges += graph.adj[u].size() + graph.rev_adj[u].size(); // ||δ||: archi incidenti
            
            // Aggiorna i valori rhs di tutti i successori
            for (auto& edge : graph.adj[u]) {
                int succ = edge.first;
                Stats::scanned_edges++; // BENCHMARK: Arco esaminato
                
                int oldRhs = rhs[succ];
                updateRhs(succ);
                
                // Se il successore è diventato inconsistente, aggiungilo alla coda
                if (dist[succ] != rhs[succ]) {
                    pq.push({computeKey(succ), succ});
                    Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
                    if (rhs[succ] != oldRhs) {
                        Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
                    }
                }
            }
        } else {
            // Imposta dist a INF e rivaluta
            dist[u] = INF;
            parent[u] = -1;
            Stats::affected_nodes++; // BENCHMARK: Distanza del nodo cambiata
            Stats::affected_edges += graph.adj[u].size() + graph.rev_adj[u].size(); // ||δ||: archi incidenti
            
            // Ricontrolla u stesso dopo aver impostato dist a INF
            updateRhs(u);
            if (dist[u] != rhs[u]) {
                pq.push({computeKey(u), u});
                Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
            }
            
            // Aggiorna i valori rhs di tutti i successori
            for (auto& edge : graph.adj[u]) {
                int succ = edge.first;
                Stats::scanned_edges++; // BENCHMARK: Arco esaminato
                
                int oldRhs = rhs[succ];
                updateRhs(succ);
                
                // Se il successore è diventato inconsistente, aggiungilo alla coda
                if (dist[succ] != rhs[succ]) {
                    pq.push({computeKey(succ), succ});
                    Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
                    if (rhs[succ] != oldRhs) {
                        Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
                    }
                }
            }
        }
    }
}

void RamalingamReps::initialize(int source) {
    sourceVertex = source;
    int n = graph.numVertices;
    
    dist.assign(n, INF);
    rhs.assign(n, INF);
    parent.assign(n, -1);
    
    // Svuota la coda di priorità
    pq = MinHeap();
    
    // Inizializzazione della sorgente
    rhs[source] = 0;
    dist[source] = INF;  // Sarà corretto da stabilize
    pq.push({0, source});
    Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
    
    // Esegue stabilize per calcolare i cammini minimi iniziali
    stabilize();
}

void RamalingamReps::handleEdgeUpdate(int u, int v, int newWeight) {
    if (sourceVertex == -1) return; // Non inizializzato

    int oldWeight = graph.getEdgeWeight(u, v);
    if (oldWeight == -1) {
        std::cerr << "Errore: arco (" << u << ", " << v << ") non trovato nel grafo" << std::endl;
        return;
    }
    
    // Aggiorna il grafo con il nuovo peso
    graph.updateEdge(u, v, newWeight);

    // Svuota la coda di priorità per questo aggiornamento
    pq = MinHeap();
    
    // Ricalcola rhs[v] poiché un arco entrante è cambiato
    int oldRhs = rhs[v];
    updateRhs(v);
    
    // Se v è diventato inconsistente, aggiungilo alla coda
    if (dist[v] != rhs[v]) {
        pq.push({computeKey(v), v});
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
        if (rhs[v] != oldRhs) {
            Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
        }
    }
    
    // Stabilizza per propagare le modifiche
    stabilize();
}

void RamalingamReps::handleEdgeInsertion(int u, int v, int w) {
    if (sourceVertex == -1) return;

    // Aggiungi arco al grafo
    graph.addEdge(u, v, w);

    // Svuota la coda di priorità per questo aggiornamento
    pq = MinHeap();
    
    // Ricalcola rhs[v] poiché è stato aggiunto un nuovo arco entrante
    int oldRhs = rhs[v];
    updateRhs(v);
    
    // Se v è diventato inconsistente, aggiungilo alla coda
    if (dist[v] != rhs[v]) {
        pq.push({computeKey(v), v});
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
        if (rhs[v] != oldRhs) {
            Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
        }
    }
    
    // Stabilizza per propagare le modifiche
    stabilize();
}

void RamalingamReps::handleEdgeDeletion(int u, int v) {
    if (sourceVertex == -1) return;

    // Rimuovi arco dal grafo
    graph.removeEdge(u, v);

    // Svuota la coda di priorità per questo aggiornamento
    pq = MinHeap();
    
    // Ricalcola rhs[v] poiché un arco entrante è stato rimosso
    int oldRhs = rhs[v];
    updateRhs(v);
    
    // Se v è diventato inconsistente, aggiungilo alla coda
    if (dist[v] != rhs[v]) {
        pq.push({computeKey(v), v});
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
        if (rhs[v] != oldRhs) {
            Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
        }
    }
    
    // Stabilizza per propagare le modifiche
    stabilize();
}

int RamalingamReps::getDistance(int target) {
    if (target < 0 || target >= graph.numVertices) return INF;
    return dist[target];
}