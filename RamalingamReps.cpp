#include "RamalingamReps.h"
#include <limits>
#include <algorithm>
#include <iostream>
#include <queue>
#include "BenchmarkStats.h" // BENCHMARK INSTRUMENTATION

const int INF = std::numeric_limits<int>::max();

RamalingamReps::RamalingamReps(Graph& g) : graph(g), sourceVertex(-1) {}

void RamalingamReps::initialize(int source) {
    sourceVertex = source;
    int n = graph.numVertices;
    dist.assign(n, INF);
    parent.assign(n, -1);
    dist[source] = 0;

    std::priority_queue<std::pair<int, int>, 
                        std::vector<std::pair<int, int>>, 
                        std::greater<std::pair<int, int>>> pq;
    pq.push({0, source});
    Stats::heap_ops++; // BENCHMARK INSTRUMENTATION

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION

        if (d > dist[u]) continue;

        Stats::visited_nodes++; // BENCHMARK INSTRUMENTATION

        for (auto& edge : graph.adj[u]) {
            int v = edge.first;
            int w = edge.second;
            Stats::scanned_edges++; // BENCHMARK: Edge examined
            if (dist[u] != INF && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
                pq.push({dist[v], v});
                Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
                Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
            }
        }
    }
}

// Ricalcola la distanza di un nodo v basandosi sui suoi predecessori
// Ritorna true se la distanza è cambiata
bool RamalingamReps::recomputeNode(int v) {
    int oldDist = dist[v];
    int bestDist = INF;
    int bestPar = -1;

    // Se è la sorgente, la distanza è sempre 0
    if (v == sourceVertex) {
        bestDist = 0;
        bestPar = -1;
    } else {
        for (auto& edge : graph.rev_adj[v]) {
            int p = edge.first;
            int w = edge.second;
            Stats::scanned_edges++; // BENCHMARK: Edge examined
            if (dist[p] != INF) {
                long long newD = (long long)dist[p] + w; // evitare overflow
                if (newD < bestDist) {
                    bestDist = (int)newD;
                    bestPar = p;
                }
            }
        }
    }

    if (bestDist != oldDist) {
        dist[v] = bestDist;
        parent[v] = bestPar;
        return true;
    }
    return false;
}

void RamalingamReps::handleEdgeUpdate(int u, int v, int newWeight) {
    if (sourceVertex == -1) return; // Non inizializzato

    // Se oldWeight non è specificato, recuperalo dal grafo PRIMA di aggiornarlo
    
    int oldWeight = graph.getEdgeWeight(u, v);
    if (oldWeight == -1) {
        std::cerr << "Errore: arco (" << u << ", " << v << ") non trovato nel grafo" << std::endl;
        return;
    }
    
    // Aggiorna il grafo con il nuovo peso
    graph.updateEdge(u, v, newWeight);

    std::priority_queue<std::pair<int, int>, 
                        std::vector<std::pair<int, int>>, 
                        std::greater<std::pair<int, int>>> pq;

    // Caso 1: Decremento di peso (o arco non usato prima che ora diventa conveniente)
    // Nota: anche se il peso aumenta, se non era usato prima, potrebbe diventare conveniente? No.
    // Ma se diminuisce, gestiscilo come un normale update di Dijkstra.
    if (dist[u] != INF && dist[u] + newWeight < dist[v]) {
        dist[v] = dist[u] + newWeight;
        parent[v] = u;
        pq.push({dist[v], v});
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
        Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
    }
    
    // Caso 2: Incremento di peso su un arco usato nell'SPJO
    else if (parent[v] == u && newWeight > oldWeight) {
        // FASE 1: Invalidazione (BFS sui discendenti dell'albero dei cammini minimi)
        std::queue<int> q_inval;
        std::vector<int> affected;
        
        dist[v] = INF; 
        parent[v] = -1;
        q_inval.push(v);
        // BFS uses queue ops, not heap ops. Not counting as heap_ops.
        affected.push_back(v);
        Stats::affected_nodes++; // BENCHMARK: Node distance changed

        while(!q_inval.empty()){
            int curr = q_inval.front(); 
            q_inval.pop();
            Stats::visited_nodes++; // BENCHMARK INSTRUMENTATION: BFS visit

            for(auto& edge : graph.adj[curr]){
                int succ = edge.first;
                Stats::scanned_edges++; // BENCHMARK: Edge examined
                // Se succ è figlio di curr nell'albero dei cammini minimi, va invalidato
                if(parent[succ] == curr){
                    dist[succ] = INF; 
                    parent[succ] = -1;
                    q_inval.push(succ);
                    affected.push_back(succ);
                    Stats::affected_nodes++; // BENCHMARK: Node distance changed
                }
            }
        }

        // FASE 2: Riconnessione (Cerca percorsi alternativi dai vicini non affetti)
        for(int node : affected){
            // recomputeNode cercherà il miglior padre tra i vicini.
            // I vicini ancora a INF verranno ignorati (perché dist[p] != INF check in recomputeNode)
            if(recomputeNode(node)){
                // Se ha trovato un nuovo percorso valido (dist != INF), lo aggiungiamo alla PQ
                if(dist[node] != INF){
                    pq.push({dist[node], node});
                    Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
                    Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION (Effective relaxation)
                }
            }
        }
    }

    // FASE 3: Propagazione standard (Dijkstra)
    while (!pq.empty()) {
        int d = pq.top().first;
        int curr = pq.top().second;
        pq.pop();
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION

        if (d > dist[curr]) continue;

        Stats::visited_nodes++; // BENCHMARK INSTRUMENTATION

        for (auto& edge : graph.adj[curr]) {
            int succ = edge.first;
            int w = edge.second;
            Stats::scanned_edges++; // BENCHMARK: Edge examined

            if (dist[curr] != INF && dist[curr] + w < dist[succ]) {
                dist[succ] = dist[curr] + w;
                parent[succ] = curr;
                pq.push({dist[succ], succ});
                Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
                Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
                Stats::affected_nodes++; // BENCHMARK: Node distance changed
            }
        }
    }
}

void RamalingamReps::handleEdgeInsertion(int u, int v, int w) {
    if (sourceVertex == -1) return;

    // Inserire un arco è come aggiornarlo da INF a w.
    // Ovvero: è un decremento di peso.
    
    // 1. Modifica strutturale del grafo
    graph.addEdge(u, v, w);

    // 2. Propagazione (simile a Case 1 di handleEdgeUpdate)
    std::priority_queue<std::pair<int, int>, 
                        std::vector<std::pair<int, int>>, 
                        std::greater<std::pair<int, int>>> pq;

    if (dist[u] != INF && dist[u] + w < dist[v]) {
        dist[v] = dist[u] + w;
        parent[v] = u;
        pq.push({dist[v], v});
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
        Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
    }

    // Propagazione standard
    while (!pq.empty()) {
        int d = pq.top().first;
        int curr = pq.top().second;
        pq.pop();
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION

        if (d > dist[curr]) continue;

        Stats::visited_nodes++; // BENCHMARK INSTRUMENTATION

        for (auto& edge : graph.adj[curr]) {
            int succ = edge.first;
            int weight = edge.second;
            Stats::scanned_edges++; // BENCHMARK: Edge examined
            if (dist[curr] != INF && dist[curr] + weight < dist[succ]) {
                dist[succ] = dist[curr] + weight;
                parent[succ] = curr;
                pq.push({dist[succ], succ});
                Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
                Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
                Stats::affected_nodes++; // BENCHMARK: Node distance changed
            }
        }
    }
}

void RamalingamReps::handleEdgeDeletion(int u, int v) {
    if (sourceVertex == -1) return;

    // Rimuovere un arco è come aggiornarlo da w a INF.
    // Ovvero: è un incremento di peso (Case 2).

    // 1. Verifica se l'arco era usato nello Shortest Path Tree (SPJO)
    bool wasUsed = (parent[v] == u);

    // 2. Modifica strutturale del grafo
    graph.removeEdge(u, v);

    // 3. Se non era usato, non c'è nulla da aggiornare
    if (!wasUsed) return;

    // 4. Se era usato, dobbiamo invalidare e ricalcolare (Logica a 2 fasi)
    std::priority_queue<std::pair<int, int>, 
                        std::vector<std::pair<int, int>>, 
                        std::greater<std::pair<int, int>>> pq;
    
    // FASE 1: Invalidazione (BFS)
    std::queue<int> q_inval;
    std::vector<int> affected;
    
    dist[v] = INF; 
    parent[v] = -1;
    q_inval.push(v);
    affected.push_back(v);
    Stats::affected_nodes++; // BENCHMARK: Node distance changed

    while(!q_inval.empty()){
        int curr = q_inval.front(); 
        q_inval.pop();
        Stats::visited_nodes++; // BENCHMARK INSTRUMENTATION (BFS)

        for(auto& edge : graph.adj[curr]){
            int succ = edge.first;
            Stats::scanned_edges++; // BENCHMARK: Edge examined
            if(parent[succ] == curr){
                dist[succ] = INF; 
                parent[succ] = -1;
                q_inval.push(succ);
                affected.push_back(succ);
                Stats::affected_nodes++; // BENCHMARK: Node distance changed
            }
        }
    }

    // FASE 2: Riconnessione
    for(int node : affected){
        if(recomputeNode(node)){
            if(dist[node] != INF){
                pq.push({dist[node], node});
                Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
                Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
            }
        }
    }

    // FASE 3: Propagazione
    while (!pq.empty()) {
        int d = pq.top().first;
        int curr = pq.top().second;
        pq.pop();
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION

        if (d > dist[curr]) continue;
        
        Stats::visited_nodes++; // BENCHMARK INSTRUMENTATION

        for (auto& edge : graph.adj[curr]) {
            int succ = edge.first;
            int weight = edge.second;
            Stats::scanned_edges++; // BENCHMARK: Edge examined
            if (dist[curr] != INF && dist[curr] + weight < dist[succ]) {
                dist[succ] = dist[curr] + weight;
                parent[succ] = curr;
                pq.push({dist[succ], succ});
                Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
                Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
                Stats::affected_nodes++; // BENCHMARK: Node distance changed
            }
        }
    }
}

int RamalingamReps::getDistance(int target) {
    if (target < 0 || target >= graph.numVertices) return INF;
    return dist[target];
}