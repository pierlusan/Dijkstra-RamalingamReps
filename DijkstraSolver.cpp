#include "DijkstraSolver.h"
#include <queue>
#include <algorithm> // per reverse
#include <limits>    // per numeric_limits
#include "BenchmarkStats.h" // BENCHMARK INSTRUMENTATION

const int INF = std::numeric_limits<int>::max();

DijkstraSolver::DijkstraSolver(Graph& g) : graph(g) {
    // Il costruttore inizializza solo il riferimento, i vettori vengono ridimensionati in compute
}

void DijkstraSolver::compute(int source) {
    int n = graph.numVertices;
    dist.assign(n, INF);
    parent.assign(n, -1);

    dist[source] = 0;

    // Min-priority queue: (distanza, vertice)
    // std::priority_queue di base è Max-Queue, quindi usiamo greater
    std::priority_queue<std::pair<int, int>, 
                        std::vector<std::pair<int, int>>, 
                        std::greater<std::pair<int, int>>> pq;

   
    pq.push({0, source});
    Stats::heap_ops++; // BENCHMARK INSTRUMENTATION: Push

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION: Pop

        // Se abbiamo trovato un percorso più breve per u prima di estrarlo, ignoriamo
        if (d > dist[u]) continue;

        Stats::visited_nodes++; // BENCHMARK INSTRUMENTATION: Node extracted and processed

        // Itera sui vicini
        for (auto& edge : graph.adj[u]) {
            int v = edge.first;
            int weight = edge.second;
            Stats::scanned_edges++; // BENCHMARK: Edge examined

            if (dist[u] != INF && dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                parent[v] = u;
                pq.push({dist[v], v});
                Stats::heap_ops++; // BENCHMARK INSTRUMENTATION: Push (Decrease Key simulated)
                Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION: Edge relaxed
            }
        }
    }
}

std::vector<int> DijkstraSolver::getPath(int target) {
    std::vector<int> path;
    if (target < 0 || target >= graph.numVertices) return path; // o throw
    if (dist[target] == INF) return path; // Non raggiungibile

    for (int v = target; v != -1; v = parent[v]) {
        path.push_back(v);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

int DijkstraSolver::getDistance(int target) {
    if (target < 0 || target >= graph.numVertices) return INF;
    return dist[target];
}




