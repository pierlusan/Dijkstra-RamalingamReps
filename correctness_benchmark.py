
import os
import time
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import random
import heapq
import sys

# CONFIGURATION
# Reduced scales slightly as Python Dijkstra is slower than NetworkX C-extensions
SCALES = [100, 500, 1000, 2000, 5000, 10000] 
EDGE_PROB = 0.05 
MAX_WEIGHT = 100
SOURCE_NODE = 0
BINARY_PATH = "./cmake-build-debug/dijkstra_runner"
RESULTS_FILE = "correctness_results.csv"

def generate_graph_file(num_nodes, filename):
    # Generates a random graph (Adjacency List)
    # Returns adj: dict {u: [(v, w), ...]}
    adj = {i: [] for i in range(num_nodes)}
    
    random.seed(42)
    # Using probability to generate edges
    # For denser graphs this loop is slow in Python, but for 10k nodes * 0.05 prob it's manageable
    # Optimization: iterate possible edges? No, 10k^2 is too big.
    # Iterate nodes and pick random neighbors?
    
    # Approx n * n * p edges.
    # For N=10000, 100M pairs. Too slow for pure python loops if not careful.
    # Let's use a fixed average degree instead to stay fast?
    # Or just loop nicely.
    
    avg_degree =  max(5, int(num_nodes * EDGE_PROB))
    
    with open(filename, 'w') as f:
        f.write(f"{num_nodes}\n")
        
        for u in range(num_nodes):
            # Pick k random neighbors
            # To avoid self-loops and duplicates
            possible = random.sample(range(num_nodes), k=avg_degree)
            for v in possible:
                if u == v: continue
                w = random.randint(1, MAX_WEIGHT)
                adj[u].append((v, w))
                f.write(f"{u} {v} {w}\n")
    return adj

def python_dijkstra(adj, num_nodes, source):
    dist = {i: -1 for i in range(num_nodes)}
    dist[source] = 0
    pq = [(0, source)]
    
    while pq:
        d, u = heapq.heappop(pq)
        
        if dist[u] != -1 and d > dist[u]:
            continue
        
        for v, w in adj[u]:
            if dist[v] == -1 or dist[u] + w < dist[v]:
                dist[v] = dist[u] + w
                heapq.heappush(pq, (dist[v], v))
    return dist

def run_cpp_dijkstra(graph_file, output_file, source):
    cmd = [BINARY_PATH, graph_file, output_file, str(source)]
    start = time.time()
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running C++ binary: {e}")
        return 0
    end = time.time()
    return (end - start) * 1000 # ms

def check_correctness(py_dists, cpp_output_file, num_nodes):
    cpp_dists = {}
    try:
        with open(cpp_output_file, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) == 2:
                    u, d = int(parts[0]), int(parts[1])
                    cpp_dists[u] = d
    except FileNotFoundError:
        print("Error: C++ output file not found.")
        return False
    
    mismatches = 0
    
    for u in range(num_nodes):
        py_d = py_dists.get(u, -1)
        cpp_d = cpp_dists.get(u, -2) 
        
        # Normalize -1
        if py_d == -1 and cpp_d == -1: continue 
        
        if py_d != cpp_d:
            # print(f"Mismatch Node {u}: Py={py_d}, CPP={cpp_d}")
            mismatches += 1
        
    return mismatches == 0

def main():
    if not os.path.exists(BINARY_PATH):
        print(f"Error: Binary {BINARY_PATH} not found. Build it first.")
        return

    results = []
    
    print("Starting Correctness Benchmark (Pure Python Oracle)...")
    print("N, Time_Py(s), Time_CPP(ms), Match")
    
    for n in SCALES:
        graph_file = f"temp_graph_{n}.txt"
        cpp_out_file = f"temp_dist_{n}.txt"
        
        # 1. Generate & Oracle
        start_py = time.time()
        adj = generate_graph_file(n, graph_file)
        py_dists = python_dijkstra(adj, n, SOURCE_NODE)
        time_py = time.time() - start_py
        
        # 2. Run C++
        time_cpp = run_cpp_dijkstra(graph_file, cpp_out_file, SOURCE_NODE)
        
        # 3. Check
        is_correct = check_correctness(py_dists, cpp_out_file, n)
        
        print(f"{n}, {time_py:.4f}, {time_cpp:.2f}, {is_correct}")
        results.append({
            "N": n,
            "Time_Py_sec": time_py,
            "Time_CPP_ms": time_cpp,
            "Correct": is_correct
        })
        
        # Cleanup
        if os.path.exists(graph_file): os.remove(graph_file)
        if os.path.exists(cpp_out_file): os.remove(cpp_out_file)

    # Save Results
    df = pd.DataFrame(results)
    df.to_csv(RESULTS_FILE, index=False)
    
    # Plotting
    fig, ax1 = plt.subplots(figsize=(10, 6))
    
    color = 'tab:blue'
    ax1.set_xlabel('Graph Size (N)')
    ax1.set_ylabel('C++ Time (ms)', color=color)
    ax1.plot(df['N'], df['Time_CPP_ms'], color=color, marker='o', label='C++ Dijkstra')
    ax1.tick_params(axis='y', labelcolor=color)
    ax1.grid(True)
    
    ax2 = ax1.twinx() 
    color = 'tab:orange'
    ax2.set_ylabel('Python Time (sec)', color=color) 
    ax2.plot(df['N'], df['Time_Py_sec'], color=color, marker='x', linestyle='--', label='Python Oracle')
    ax2.tick_params(axis='y', labelcolor=color)
    
    plt.title('Dijkstra Correctness Benchmark: C++ vs Python Oracle')
    fig.tight_layout() 
    
    if not os.path.exists('plots'): os.makedirs('plots')
    plt.savefig('plots/correctness_benchmark.png')
    print("Saved plot to plots/correctness_benchmark.png")

if __name__ == "__main__":
    main()
