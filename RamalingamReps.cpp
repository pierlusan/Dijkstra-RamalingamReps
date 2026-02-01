#include "RamalingamReps.h"
#include <limits>
#include <algorithm>
#include <iostream>
#include <queue>
#include "BenchmarkStats.h" // BENCHMARK INSTRUMENTATION

const int INF = std::numeric_limits<int>::max();

RamalingamReps::RamalingamReps(Graph& g) : graph(g), sourceVertex(-1) {}

// Compute the key for priority queue ordering: min(dist[u], rhs[u])
int RamalingamReps::computeKey(int u) {
    return std::min(dist[u], rhs[u]);
}

// Recompute rhs[u] based on predecessors
// rhs[u] = min over all predecessors p of (dist[p] + weight(p, u))
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
        Stats::scanned_edges++; // BENCHMARK: Edge examined
        
        if (dist[pred] != INF) {
            long long newD = (long long)dist[pred] + w; // avoid overflow
            if (newD < best) {
                best = (int)newD;
                bestPar = pred;
            }
        }
    }
    
    rhs[u] = best;
    // Update parent for path reconstruction
    if (best != INF) {
        parent[u] = bestPar;
    }
}

// Main stabilization loop: process inconsistent nodes until all are consistent
// A node is consistent when dist[u] == rhs[u]
// Over-consistent: dist[u] > rhs[u] -> reduce dist to rhs
// Under-consistent: dist[u] < rhs[u] -> raise dist to INF
void RamalingamReps::stabilize() {
    while (!pq.empty()) {
        auto [key, u] = pq.top();
        pq.pop();
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
        
        // Skip stale entries
        int currentKey = computeKey(u);
        if (key > currentKey) continue;
        
        // Skip already consistent nodes
        if (dist[u] == rhs[u]) continue;
        
        Stats::visited_nodes++; // BENCHMARK INSTRUMENTATION
        
        if (dist[u] > rhs[u]) {
            // Over-consistent: lower dist to match rhs
            dist[u] = rhs[u];
            Stats::affected_nodes++; // BENCHMARK: Node distance changed
            Stats::affected_edges += graph.adj[u].size() + graph.rev_adj[u].size(); // ||δ||: incident edges
            
            // Update all successors' rhs values
            for (auto& edge : graph.adj[u]) {
                int succ = edge.first;
                Stats::scanned_edges++; // BENCHMARK: Edge examined
                
                int oldRhs = rhs[succ];
                updateRhs(succ);
                
                // If successor became inconsistent, add to queue
                if (dist[succ] != rhs[succ]) {
                    pq.push({computeKey(succ), succ});
                    Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
                    if (rhs[succ] != oldRhs) {
                        Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
                    }
                }
            }
        } else {
            // Set dist to INF and re-evaluate
            dist[u] = INF;
            parent[u] = -1;
            Stats::affected_nodes++; // BENCHMARK: Node distance changed
            Stats::affected_edges += graph.adj[u].size() + graph.rev_adj[u].size(); // ||δ||: incident edges
            
            // Re-check u itself after setting dist to INF
            updateRhs(u);
            if (dist[u] != rhs[u]) {
                pq.push({computeKey(u), u});
                Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
            }
            
            // Update all successors' rhs values
            for (auto& edge : graph.adj[u]) {
                int succ = edge.first;
                Stats::scanned_edges++; // BENCHMARK: Edge examined
                
                int oldRhs = rhs[succ];
                updateRhs(succ);
                
                // If successor became inconsistent, add to queue
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
    
    // Clear priority queue
    pq = MinHeap();
    
    // Source initialization
    rhs[source] = 0;
    dist[source] = INF;  // Will be fixed by stabilize
    pq.push({0, source});
    Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
    
    // Run stabilize to compute initial shortest paths
    stabilize();
}

void RamalingamReps::handleEdgeUpdate(int u, int v, int newWeight) {
    if (sourceVertex == -1) return; // Not initialized

    int oldWeight = graph.getEdgeWeight(u, v);
    if (oldWeight == -1) {
        std::cerr << "Errore: arco (" << u << ", " << v << ") non trovato nel grafo" << std::endl;
        return;
    }
    
    // Update the graph with new weight
    graph.updateEdge(u, v, newWeight);

    // Clear priority queue for this update
    pq = MinHeap();
    
    // Recompute rhs[v] since an incoming edge changed
    int oldRhs = rhs[v];
    updateRhs(v);
    
    // If v became inconsistent, add to queue
    if (dist[v] != rhs[v]) {
        pq.push({computeKey(v), v});
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
        if (rhs[v] != oldRhs) {
            Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
        }
    }
    
    // Stabilize to propagate changes
    stabilize();
}

void RamalingamReps::handleEdgeInsertion(int u, int v, int w) {
    if (sourceVertex == -1) return;

    // Add edge to graph
    graph.addEdge(u, v, w);

    // Clear priority queue for this update
    pq = MinHeap();
    
    // Recompute rhs[v] since a new incoming edge was added
    int oldRhs = rhs[v];
    updateRhs(v);
    
    // If v became inconsistent, add to queue
    if (dist[v] != rhs[v]) {
        pq.push({computeKey(v), v});
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
        if (rhs[v] != oldRhs) {
            Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
        }
    }
    
    // Stabilize to propagate changes
    stabilize();
}

void RamalingamReps::handleEdgeDeletion(int u, int v) {
    if (sourceVertex == -1) return;

    // Remove edge from graph
    graph.removeEdge(u, v);

    // Clear priority queue for this update
    pq = MinHeap();
    
    // Recompute rhs[v] since an incoming edge was removed
    int oldRhs = rhs[v];
    updateRhs(v);
    
    // If v became inconsistent, add to queue
    if (dist[v] != rhs[v]) {
        pq.push({computeKey(v), v});
        Stats::heap_ops++; // BENCHMARK INSTRUMENTATION
        if (rhs[v] != oldRhs) {
            Stats::relaxed_edges++; // BENCHMARK INSTRUMENTATION
        }
    }
    
    // Stabilize to propagate changes
    stabilize();
}

int RamalingamReps::getDistance(int target) {
    if (target < 0 || target >= graph.numVertices) return INF;
    return dist[target];
}