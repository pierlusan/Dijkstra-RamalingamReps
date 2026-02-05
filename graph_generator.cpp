/**
 * Graph Generator
 * Genera tre tipi di grafi per testing Dijkstra:
 * 1. Sparsi uniformi (m = 4n)
 * 2. Sparsi non-uniformi (Barabási-Albert, scale-free)
 * 3. Densi (m ≈ n²/4)
 * 
 * Output: cartelle grafi_sparsi/, grafi_barabasi/, grafi_densi/
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <sys/stat.h>
#include "Graph.h"

// ============================================================================
// GENERATORI DI GRAFI
// ============================================================================

/**
 * Grafo sparso uniforme: m = 4n archi distribuiti casualmente
 */
void generateSparseUniform(Graph& g, int n, std::mt19937& rng, int maxWeight = 1000) {
    std::uniform_int_distribution<int> nodeDist(0, n - 1);
    std::uniform_int_distribution<int> weightDist(1, maxWeight);
    
    int m = 4 * n;
    for (int i = 0; i < m; ++i) {
        int u = nodeDist(rng);
        int v = nodeDist(rng);
        if (u != v) {
            int w = weightDist(rng);
            g.addEdge(u, v, w);
        }
    }
}

/**
 * Grafo Barabási-Albert (scale-free, preferential attachment)
 * Genera grafi con distribuzione power-law dei gradi
 */
void generateBarabasiAlbertEfficient(Graph& g, int n, int m_attach, std::mt19937& rng, int maxWeight = 1000) {
    if (m_attach < 1 || m_attach >= n) return;

    std::uniform_int_distribution<int> weightDist(1, maxWeight);
    std::vector<int> edge_list;
    edge_list.reserve(2 * n * m_attach);

    // Nucleo iniziale: clique di m_attach + 1 nodi
    for (int i = 0; i <= m_attach; ++i) {
        for (int j = i + 1; j <= m_attach; ++j) {
            int w = weightDist(rng);
            g.addEdge(i, j, w);
            g.addEdge(j, i, w);
            edge_list.push_back(i);
            edge_list.push_back(j);
        }
    }

    // Crescita con preferential attachment
    for (int i = m_attach + 1; i < n; ++i) {
        std::vector<int> targets;
        while (targets.size() < (size_t)m_attach) {
            int t = edge_list[std::uniform_int_distribution<int>(0, edge_list.size() - 1)(rng)];
            if (t != i && std::find(targets.begin(), targets.end(), t) == targets.end()) {
                targets.push_back(t);
            }
        }

        for (int t : targets) {
            int w = weightDist(rng);
            g.addEdge(i, t, w);
            g.addEdge(t, i, w);
            edge_list.push_back(i);
            edge_list.push_back(t);
        }
    }
}

/**
 * Grafo denso: ogni coppia di nodi ha probabilità density di essere connessa
 * Default density = 0.25 -> m ≈ n²/4
 */
void generateDense(Graph& g, int n, std::mt19937& rng, int maxWeight = 1000, double density = 0.25) {
    std::uniform_real_distribution<double> probDist(0.0, 1.0);
    std::uniform_int_distribution<int> weightDist(1, maxWeight);
    
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            if (probDist(rng) < density) {
                int w = weightDist(rng);
                g.addEdge(u, v, w);
                g.addEdge(v, u, w);
            }
        }
    }
}

// ============================================================================
// UTILITY
// ============================================================================

void createDirectory(const std::string& path) {
    mkdir(path.c_str(), 0755);
}

void saveGraphToFile(Graph& g, const std::string& filename) {
    std::ofstream out(filename);
    out << g.numVertices << std::endl;
    for (int u = 0; u < g.numVertices; ++u) {
        for (auto& edge : g.adj[u]) {
            int v = edge.first;
            int w = edge.second;
            out << u << " " << v << " " << w << std::endl;
        }
    }
    out.close();
}

int countEdges(Graph& g) {
    int count = 0;
    for (int u = 0; u < g.numVertices; ++u) {
        count += g.adj[u].size();
    }
    return count;
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "=== GRAPH GENERATOR ===" << std::endl;
    std::cout << "Genera grafi per testing Dijkstra" << std::endl << std::endl;

    // Seed fisso per riproducibilità
    std::mt19937 rng(42);

    // Crea directory di output
    createDirectory("grafi_sparsi");
    createDirectory("grafi_barabasi");
    createDirectory("grafi_densi");

    // Range di n 
    // Per grafi sparsi e Barabási: fino a 1M nodi
    // Per grafi densi: più piccoli (m = O(n²))
    std::vector<int> sizes_sparse = {1000, 2000, 4000, 8000, 16000, 32000, 
                                      64000, 128000, 256000, 512000, 1024000};
    std::vector<int> sizes_dense = {500, 1000, 2000, 4000, 8000, 16000};

    // Parametro Barabási-Albert: numero di archi per nuovo nodo
    int m_attach = 4;  // Così m ≈ 4n come nei grafi sparsi uniformi

    // ========================================================================
    // 1. GRAFI SPARSI UNIFORMI
    // ========================================================================
    std::cout << "--- Generazione Grafi Sparsi Uniformi (m = 4n) ---" << std::endl;
    for (int n : sizes_sparse) {
        Graph g(n);
        generateSparseUniform(g, n, rng);
        
        std::ostringstream filename;
        filename << "grafi_sparsi/graph_n" << n << ".txt";
        saveGraphToFile(g, filename.str());
        
        int m = countEdges(g);
        std::cout << "  n=" << n << ", m=" << m << " -> " << filename.str() << std::endl;
    }
    std::cout << std::endl;

    // ========================================================================
    // 2. GRAFI BARABÁSI-ALBERT (Scale-Free)
    // ========================================================================
    std::cout << "--- Generazione Grafi Barabási-Albert (m_attach=" << m_attach << ") ---" << std::endl;
    for (int n : sizes_sparse) {
        Graph g(n);
        generateBarabasiAlbertEfficient(g, n, m_attach, rng);
        
        std::ostringstream filename;
        filename << "grafi_barabasi/graph_n" << n << ".txt";
        saveGraphToFile(g, filename.str());
        
        int m = countEdges(g);
        std::cout << "  n=" << n << ", m=" << m << " -> " << filename.str() << std::endl;
    }
    std::cout << std::endl;

    // ========================================================================
    // 3. GRAFI DENSI (m ≈ n²/4)
    // ========================================================================
    std::cout << "--- Generazione Grafi Densi (density=0.25) ---" << std::endl;
    for (int n : sizes_dense) {
        Graph g(n);
        generateDense(g, n, rng);
        
        std::ostringstream filename;
        filename << "grafi_densi/graph_n" << n << ".txt";
        saveGraphToFile(g, filename.str());
        
        int m = countEdges(g);
        long long expected_m = (long long)n * (n - 1) / 4;  // density = 0.25, bidirezionale
        std::cout << "  n=" << n << ", m=" << m << " (expected ~" << expected_m << ") -> " << filename.str() << std::endl;
    }
    std::cout << std::endl;

    std::cout << "=== GENERAZIONE COMPLETATA ===" << std::endl;
    std::cout << "Grafi salvati in:" << std::endl;
    std::cout << "  - grafi_sparsi/    (sparsi uniformi, m=4n)" << std::endl;
    std::cout << "  - grafi_barabasi/  (scale-free, power-law)" << std::endl;
    std::cout << "  - grafi_densi/     (densi, m≈n²/4)" << std::endl;

    return 0;
}
