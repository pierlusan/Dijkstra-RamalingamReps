import networkx as nx
import random
import os
from typing import Dict, Tuple, List

# Configuration
NUM_NODES = 1000
EDGE_PROB = 0.2
MAX_WEIGHT = 100
SOURCE_NODE = 0
SEED = 42  # For reproducibility

# Output files
FILE_GRAPH = "test_case_graph.txt"
FILE_INIT_DIST = "expected_init_dist.txt"
FILE_UPDATE = "test_case_update.txt"
FILE_FINAL_DIST = "expected_final_dist.txt"

def generate_weighted_graph(num_nodes: int, probability: float, max_weight: int) -> nx.DiGraph:
    """Generates a random directed graph with positive integer weights."""
    print(f"Generating random graph (Nodes: {num_nodes}, Prob: {probability})...")
    G = nx.fast_gnp_random_graph(num_nodes, probability, directed=True, seed=SEED)
    
    # Assign random weights
    random.seed(SEED)
    for (u, v) in G.edges():
        G.edges[u, v]['weight'] = random.randint(1, max_weight)
    
    return G

def export_graph(G: nx.DiGraph, filename: str):
    """Exports graph in 'u v w' format."""
    print(f"Exporting graph to {filename}...")
    with open(filename, 'w') as f:
        # Write number of vertices first, as expected by Graph::loadFromFile
        f.write(f"{G.number_of_nodes()}\n")
        for u, v, data in G.edges(data=True):
            f.write(f"{u} {v} {data['weight']}\n")

def compute_and_export_sssp(G: nx.DiGraph, source: int, filename: str):
    """Computes SSSP using Dijkstra and exports expected distances."""
    print(f"Computing SSSP from node {source}...")
    try:
        lengths = nx.single_source_dijkstra_path_length(G, source)
    except nx.NetworkXNoPath:
        lengths = {source: 0} # Should not happen usually if graph is connected enough or just handling isolated

    print(f"Exporting distances to {filename}...")
    with open(filename, 'w') as f:
        for node_id in range(G.number_of_nodes()):
            dist = lengths.get(node_id, -1) # -1 or INF for unreachable
            # Use -1 for unreachable as per common practice in competitive programming/simple impls, 
            # but user mentioned "Use -1 or INF". Let's stick to -1 for simplicity in parsing.
            if dist == -1:
                # If networkx doesn't return it, it's unreachable.
                # But actually networkx omit unreachable nodes.
                pass
            f.write(f"{node_id} {dist}\n")

def generate_update(G: nx.DiGraph, max_weight: int) -> Tuple[int, int, int, int]:
    """
    Selects a random edge and updates its weight.
    Returns (u, v, old_weight, new_weight).
    """
    edges = list(G.edges(data=True))
    if not edges:
        raise ValueError("Graph has no edges to update!")
    
    u, v, data = random.choice(edges)
    old_weight = data['weight']
    
    # Randomly decide to increase or decrease, but keep within [1, MAX_WEIGHT + buffer]
    # To ensure we test Ramalingam-Reps properly, we might want significant changes.
    # Let's just pick a completely new random weight.
    new_weight = random.randint(1, max_weight)
    
    # Ensure it's actually different
    while new_weight == old_weight:
         new_weight = random.randint(1, max_weight)
         
    # Update graph in place
    G.edges[u, v]['weight'] = new_weight
    
    return u, v, old_weight, new_weight

def export_update(update: Tuple[int, int, int, int], filename: str):
    """Exports the update in 'u v old_w new_w' format."""
    u, v, old_w, new_w = update
    print(f"Exporting update ({u}->{v}: {old_w} -> {new_w}) to {filename}...")
    with open(filename, 'w') as f:
        f.write(f"{u} {v} {old_w} {new_w}\n")

def main():
    # 1. Generate Graph
    G = generate_weighted_graph(NUM_NODES, EDGE_PROB, MAX_WEIGHT)
    
    # 2. Phase 1: Initial Static Test
    export_graph(G, FILE_GRAPH)
    compute_and_export_sssp(G, SOURCE_NODE, FILE_INIT_DIST)
    
    # 3. Phase 2: Dynamic Update Test
    update = generate_update(G, MAX_WEIGHT)
    export_update(update, FILE_UPDATE)
    
    # Re-compute SSSP with updated graph
    compute_and_export_sssp(G, SOURCE_NODE, FILE_FINAL_DIST)
    
    print("\nFuzzing data generation complete.")
    print(f"1. {FILE_GRAPH}")
    print(f"2. {FILE_INIT_DIST}")
    print(f"3. {FILE_UPDATE}")
    print(f"4. {FILE_FINAL_DIST}")

if __name__ == "__main__":
    # Check for networkx
    try:
        import networkx
    except ImportError:
        print("Error: 'networkx' library is not installed.")
        print("Please install it running: pip install networkx")
        exit(1)
        
    main()
