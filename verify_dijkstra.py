#!/usr/bin/env python3
"""
Verifica la correttezza dei risultati di Dijkstra C++ confrontandoli con networkx.
Usa gli stessi grafi generati dal doubling experiment.
"""

import networkx as nx
import os
import sys
from pathlib import Path

def load_graph_from_file(filename: str) -> nx.DiGraph:
    """Carica un grafo dal formato usato da main.cpp.
    Se ci sono archi multipli, tiene il peso minimo (come fa Dijkstra C++)."""
    G = nx.DiGraph()
    with open(filename, 'r') as f:
        num_vertices = int(f.readline().strip())
        G.add_nodes_from(range(num_vertices))
        for line in f:
            parts = line.strip().split()
            if len(parts) == 3:
                u, v, w = int(parts[0]), int(parts[1]), int(parts[2])
                # Se l'arco esiste già, tieni il peso minimo
                if G.has_edge(u, v):
                    G.edges[u, v]['weight'] = min(G.edges[u, v]['weight'], w)
                else:
                    G.add_edge(u, v, weight=w)
    return G


def load_distances_from_csv(filename: str) -> dict:
    """Carica le distanze dal CSV generato da C++."""
    distances = {}
    with open(filename, 'r') as f:
        header = f.readline()  # Skip header "node,distance"
        for line in f:
            parts = line.strip().split(',')
            if len(parts) == 2:
                node = int(parts[0])
                dist = int(parts[1])
                distances[node] = dist
    return distances

def compute_sssp_networkx(G: nx.DiGraph, source: int) -> dict:
    """Calcola SSSP con networkx Dijkstra."""
    try:
        lengths = dict(nx.single_source_dijkstra_path_length(G, source))
    except nx.NetworkXNoPath:
        lengths = {source: 0}
    
    # Normalizza: nodi irraggiungibili = -1
    result = {}
    for node in G.nodes():
        result[node] = lengths.get(node, -1)
    return result

def verify_results(results_dir: str = "dijkstra_results"):
    """Verifica tutti i risultati nella directory."""
    results_path = Path(results_dir)
    
    if not results_path.exists():
        print(f"ERRORE: Directory '{results_dir}' non trovata.")
        print("Esegui prima ./build/dijkstra_main per generare i dati.")
        return False
    
    # Trova tutti i file di grafo
    graph_files = sorted(results_path.glob("graph_n*.txt"))
    
    if not graph_files:
        print(f"ERRORE: Nessun file graph_n*.txt trovato in {results_dir}")
        return False
    
    print(f"{'n':>10} | {'Nodi verificati':>15} | {'Errori':>8} | {'Risultato':>10}")
    print("-" * 55)
    
    all_passed = True
    
    for graph_file in graph_files:
        # Estrai n dal nome del file
        n = int(graph_file.stem.replace("graph_n", ""))
        
        dist_file = results_path / f"distances_n{n}.csv"
        
        if not dist_file.exists():
            print(f"{n:>10} | {'?':>15} | {'?':>8} | {'SKIP - no dist file':>10}")
            continue
        
        # Carica grafo e distanze C++
        G = load_graph_from_file(str(graph_file))
        cpp_distances = load_distances_from_csv(str(dist_file))
        
        # Calcola con networkx
        nx_distances = compute_sssp_networkx(G, source=0)
        
        # Confronta
        errors = 0
        for node in range(n):
            cpp_dist = cpp_distances.get(node, -999)
            nx_dist = nx_distances.get(node, -999)
            if cpp_dist != nx_dist:
                errors += 1
                if errors <= 3:  # Mostra solo i primi errori
                    print(f"  MISMATCH nodo {node}: C++={cpp_dist}, networkx={nx_dist}")
        
        status = "✓ PASS" if errors == 0 else f"✗ FAIL"
        print(f"{n:>10} | {n:>15} | {errors:>8} | {status:>10}")
        
        if errors > 0:
            all_passed = False
    
    print("-" * 55)
    if all_passed:
        print("✅ TUTTI I TEST PASSATI - Dijkstra C++ è corretto!")
    else:
        print("❌ ALCUNI TEST FALLITI - Verificare l'implementazione.")
    
    return all_passed

if __name__ == "__main__":
    try:
        import networkx
    except ImportError:
        print("Errore: 'networkx' non installato.")
        print("Installa con: pip install networkx")
        sys.exit(1)
    
    results_dir = "dijkstra_results"
    if len(sys.argv) > 1:
        results_dir = sys.argv[1]
        
    print(f"Verifica risultati in: {results_dir}")
    success = verify_results(results_dir)
    sys.exit(0 if success else 1)
