
import pandas as pd
import matplotlib.pyplot as plt
import os
import sys
import numpy as np

def setup_plots_dir(subdir=None):
    path = 'plots'
    if subdir:
        path = os.path.join(path, subdir)
    if not os.path.exists(path):
        os.makedirs(path)
    return path

def plot_legacy(df):
    print("Generating Legacy Plots...")
    plots_dir = setup_plots_dir('legacy')

    # Ensure data is sorted by UpdateID
    df = df.sort_values('UpdateID')

    # Plot 1: Speedup
    plt.figure(figsize=(10, 6))
    plt.plot(df['UpdateID'], df['Speedup'], marker='o', linestyle='-', color='b')
    plt.title('Speedup (Static / Dynamic) over Updates')
    plt.xlabel('Update ID')
    plt.ylabel('Speedup Factor')
    plt.grid(True)
    plt.savefig(os.path.join(plots_dir, 'speedup.png'))
    plt.close()

    # Plot 2: Time Comparison
    plt.figure(figsize=(10, 6))
    plt.plot(df['UpdateID'], df['Time_Static_us'], label='Static', color='r')
    plt.plot(df['UpdateID'], df['Time_Dyn_us'], label='Dynamic', color='g')
    plt.yscale('log')
    plt.title('Execution Time: Static vs Dynamic (Log Scale)')
    plt.ylabel('Time (microseconds)')
    plt.legend()
    plt.savefig(os.path.join(plots_dir, 'time_comparison.png'))
    plt.close()

    print(f"Legacy plots saved to {plots_dir}")

def plot_scientific(df):
    print("Generating Scientific Plots...")
    plots_dir = setup_plots_dir('scientific')

    # Ensure correct types
    df['Graph_N'] = df['Graph_N'].astype(int)
    
    # --- 1. Scalability: Speedup vs Graph Size (N) ---
    plt.figure(figsize=(10, 6))
    Ns = sorted(df['Graph_N'].unique())
    data_to_plot = [df[df['Graph_N'] == n]['Speedup'].values for n in Ns]
    
    plt.boxplot(data_to_plot, labels=Ns)
    plt.title('Speedup Distribution by Graph Size (N)')
    plt.xlabel('Number of Nodes (N)')
    plt.ylabel('Speedup Factor')
    plt.yscale('log')
    plt.grid(True, which="both", ls="--", alpha=0.3)
    plt.savefig(os.path.join(plots_dir, 'speedup_vs_scale.png'))
    plt.close()

    # --- 2. Time vs Scale (Log-Log) ---
    df_mean = df.groupby(['Graph_N', 'Magnitude']).agg({
        'Time_Static_ns': 'mean',
        'Time_Dyn_ns': 'mean'
    }).reset_index()

    plt.figure(figsize=(10, 6))
    
    magnitudes = df_mean['Magnitude'].unique()
    colors = {'Small': 'blue', 'Large': 'red', 'Struct': 'green'}
    markers = {'Small': 'o', 'Large': 's', 'Struct': '^'}
    
    for mag in magnitudes:
        subset = df_mean[df_mean['Magnitude'] == mag]
        plt.plot(subset['Graph_N'], subset['Time_Static_ns'], 
                 label=f'Static ({mag})', 
                 color=colors.get(mag, 'black'), 
                 marker=markers.get(mag, 'o'), 
                 linestyle='--')
        plt.plot(subset['Graph_N'], subset['Time_Dyn_ns'], 
                 label=f'Dynamic ({mag})', 
                 color=colors.get(mag, 'black'), 
                 marker=markers.get(mag, 'o'), 
                 linestyle='-')

    plt.yscale('log')
    plt.xscale('log')
    plt.title('Average Execution Time vs Graph Size (Log-Log)')
    plt.ylabel('Time (ns)')
    plt.xlabel('Graph Size (N)')
    plt.legend()
    plt.grid(True, which="both", alpha=0.3)
    plt.savefig(os.path.join(plots_dir, 'time_vs_scale_loglog.png'))
    plt.close()

    # --- 3. Impact of Update Type ---
    df_type = df.groupby('Type')['Speedup'].mean().reset_index()
    
    plt.figure(figsize=(8, 6))
    colors_bar = {'Inc': 'skyblue', 'Dec': 'salmon', 'Add': 'lightgreen', 'Del': 'orange'}
    bar_colors = [colors_bar.get(t, 'gray') for t in df_type['Type']]
    plt.bar(df_type['Type'], df_type['Speedup'], color=bar_colors)
    plt.title('Average Speedup by Update Type')
    plt.ylabel('Average Speedup')
    plt.xlabel('Update Type')
    plt.savefig(os.path.join(plots_dir, 'speedup_by_type.png'))
    plt.close()

    print(f"Scientific plots saved to {plots_dir}")

def plot_metrics(df):
    """Plot all detailed metrics (new format with individual counters)"""
    print("Generating Detailed Metrics Plots...")
    plots_dir = setup_plots_dir('metrics')
    
    df['Graph_N'] = df['Graph_N'].astype(int)
    Ns = sorted(df['Graph_N'].unique())
    
    # --- 1. Heap Operations Comparison ---
    df_mean = df.groupby('Graph_N').agg({
        'HeapOps_Static': 'mean',
        'HeapOps_Dyn': 'mean'
    }).reset_index()
    
    plt.figure(figsize=(10, 6))
    plt.plot(df_mean['Graph_N'], df_mean['HeapOps_Static'], 'r-o', label='Static (Dijkstra)')
    plt.plot(df_mean['Graph_N'], df_mean['HeapOps_Dyn'], 'g-o', label='Dynamic (RR)')
    plt.yscale('log')
    plt.xscale('log')
    plt.title('Heap Operations vs Graph Size')
    plt.xlabel('Number of Nodes (N)')
    plt.ylabel('Heap Operations (push/pop)')
    plt.legend()
    plt.grid(True, which="both", alpha=0.3)
    plt.savefig(os.path.join(plots_dir, 'heap_ops_comparison.png'))
    plt.close()
    
    # --- 2. Scanned Edges Comparison ---
    df_mean = df.groupby('Graph_N').agg({
        'ScannedEdges_Static': 'mean',
        'ScannedEdges_Dyn': 'mean'
    }).reset_index()
    
    plt.figure(figsize=(10, 6))
    plt.plot(df_mean['Graph_N'], df_mean['ScannedEdges_Static'], 'r-s', label='Static (Dijkstra)')
    plt.plot(df_mean['Graph_N'], df_mean['ScannedEdges_Dyn'], 'g-s', label='Dynamic (RR)')
    plt.yscale('log')
    plt.xscale('log')
    plt.title('Scanned Edges vs Graph Size')
    plt.xlabel('Number of Nodes (N)')
    plt.ylabel('Edges Examined')
    plt.legend()
    plt.grid(True, which="both", alpha=0.3)
    plt.savefig(os.path.join(plots_dir, 'scanned_edges_comparison.png'))
    plt.close()
    
    # --- 3. Visited Nodes Comparison ---
    df_mean = df.groupby('Graph_N').agg({
        'VisitedNodes_Static': 'mean',
        'VisitedNodes_Dyn': 'mean'
    }).reset_index()
    
    plt.figure(figsize=(10, 6))
    plt.plot(df_mean['Graph_N'], df_mean['VisitedNodes_Static'], 'r-^', label='Static (Dijkstra)')
    plt.plot(df_mean['Graph_N'], df_mean['VisitedNodes_Dyn'], 'g-^', label='Dynamic (RR)')
    plt.yscale('log')
    plt.xscale('log')
    plt.title('Visited Nodes vs Graph Size')
    plt.xlabel('Number of Nodes (N)')
    plt.ylabel('Nodes Visited')
    plt.legend()
    plt.grid(True, which="both", alpha=0.3)
    plt.savefig(os.path.join(plots_dir, 'visited_nodes_comparison.png'))
    plt.close()
    
    # --- 4. Relaxed Edges Comparison ---
    df_mean = df.groupby('Graph_N').agg({
        'RelaxedEdges_Static': 'mean',
        'RelaxedEdges_Dyn': 'mean'
    }).reset_index()
    
    plt.figure(figsize=(10, 6))
    plt.plot(df_mean['Graph_N'], df_mean['RelaxedEdges_Static'], 'r-d', label='Static (Dijkstra)')
    plt.plot(df_mean['Graph_N'], df_mean['RelaxedEdges_Dyn'], 'g-d', label='Dynamic (RR)')
    plt.yscale('log')
    plt.xscale('log')
    plt.title('Relaxed Edges vs Graph Size')
    plt.xlabel('Number of Nodes (N)')
    plt.ylabel('Edges Relaxed')
    plt.legend()
    plt.grid(True, which="both", alpha=0.3)
    plt.savefig(os.path.join(plots_dir, 'relaxed_edges_comparison.png'))
    plt.close()
    
    # --- 5. Affected Nodes (Dynamic only) ---
    df_mean = df.groupby('Graph_N').agg({
        'AffectedNodes_Dyn': 'mean'
    }).reset_index()
    
    plt.figure(figsize=(10, 6))
    plt.plot(df_mean['Graph_N'], df_mean['AffectedNodes_Dyn'], 'b-o', label='Affected Nodes (RR)')
    plt.yscale('log')
    plt.xscale('log')
    plt.title('Affected Nodes in Dynamic Updates vs Graph Size')
    plt.xlabel('Number of Nodes (N)')
    plt.ylabel('Affected Nodes')
    plt.legend()
    plt.grid(True, which="both", alpha=0.3)
    plt.savefig(os.path.join(plots_dir, 'affected_nodes.png'))
    plt.close()
    
    # --- 6. All Metrics Ratio (Static / Dynamic) ---
    df_ratio = df.groupby('Graph_N').agg({
        'HeapOps_Static': 'mean',
        'HeapOps_Dyn': 'mean',
        'ScannedEdges_Static': 'mean',
        'ScannedEdges_Dyn': 'mean',
        'VisitedNodes_Static': 'mean',
        'VisitedNodes_Dyn': 'mean',
        'RelaxedEdges_Static': 'mean',
        'RelaxedEdges_Dyn': 'mean'
    }).reset_index()
    
    # Avoid division by zero
    df_ratio['HeapOps_Ratio'] = df_ratio['HeapOps_Static'] / df_ratio['HeapOps_Dyn'].replace(0, np.nan)
    df_ratio['ScannedEdges_Ratio'] = df_ratio['ScannedEdges_Static'] / df_ratio['ScannedEdges_Dyn'].replace(0, np.nan)
    df_ratio['VisitedNodes_Ratio'] = df_ratio['VisitedNodes_Static'] / df_ratio['VisitedNodes_Dyn'].replace(0, np.nan)
    df_ratio['RelaxedEdges_Ratio'] = df_ratio['RelaxedEdges_Static'] / df_ratio['RelaxedEdges_Dyn'].replace(0, np.nan)
    
    plt.figure(figsize=(12, 6))
    plt.plot(df_ratio['Graph_N'], df_ratio['HeapOps_Ratio'], 'r-o', label='Heap Ops Ratio')
    plt.plot(df_ratio['Graph_N'], df_ratio['ScannedEdges_Ratio'], 'g-s', label='Scanned Edges Ratio')
    plt.plot(df_ratio['Graph_N'], df_ratio['VisitedNodes_Ratio'], 'b-^', label='Visited Nodes Ratio')
    plt.plot(df_ratio['Graph_N'], df_ratio['RelaxedEdges_Ratio'], 'm-d', label='Relaxed Edges Ratio')
    plt.yscale('log')
    plt.xscale('log')
    plt.title('Metrics Ratio (Static / Dynamic) vs Graph Size')
    plt.xlabel('Number of Nodes (N)')
    plt.ylabel('Ratio (Static / Dynamic)')
    plt.legend()
    plt.grid(True, which="both", alpha=0.3)
    plt.savefig(os.path.join(plots_dir, 'metrics_ratio.png'))
    plt.close()
    
    # --- 7. Metrics by Update Type ---
    df_type = df.groupby('Type').agg({
        'HeapOps_Dyn': 'mean',
        'ScannedEdges_Dyn': 'mean',
        'VisitedNodes_Dyn': 'mean',
        'AffectedNodes_Dyn': 'mean'
    }).reset_index()
    
    x = np.arange(len(df_type['Type']))
    width = 0.2
    
    fig, ax = plt.subplots(figsize=(12, 6))
    ax.bar(x - 1.5*width, df_type['HeapOps_Dyn'], width, label='Heap Ops', color='red')
    ax.bar(x - 0.5*width, df_type['ScannedEdges_Dyn'], width, label='Scanned Edges', color='green')
    ax.bar(x + 0.5*width, df_type['VisitedNodes_Dyn'], width, label='Visited Nodes', color='blue')
    ax.bar(x + 1.5*width, df_type['AffectedNodes_Dyn'], width, label='Affected Nodes', color='orange')
    
    ax.set_xlabel('Update Type')
    ax.set_ylabel('Count (avg)')
    ax.set_title('Dynamic Algorithm Metrics by Update Type')
    ax.set_xticks(x)
    ax.set_xticklabels(df_type['Type'])
    ax.legend()
    ax.set_yscale('log')
    plt.grid(True, axis='y', alpha=0.3)
    plt.savefig(os.path.join(plots_dir, 'metrics_by_type.png'))
    plt.close()
    
    # --- 8. Metrics by Magnitude ---
    df_mag = df.groupby('Magnitude').agg({
        'HeapOps_Dyn': 'mean',
        'ScannedEdges_Dyn': 'mean',
        'VisitedNodes_Dyn': 'mean',
        'AffectedNodes_Dyn': 'mean',
        'Speedup': 'mean'
    }).reset_index()
    
    x = np.arange(len(df_mag['Magnitude']))
    width = 0.2
    
    fig, ax = plt.subplots(figsize=(12, 6))
    ax.bar(x - 1.5*width, df_mag['HeapOps_Dyn'], width, label='Heap Ops', color='red')
    ax.bar(x - 0.5*width, df_mag['ScannedEdges_Dyn'], width, label='Scanned Edges', color='green')
    ax.bar(x + 0.5*width, df_mag['VisitedNodes_Dyn'], width, label='Visited Nodes', color='blue')
    ax.bar(x + 1.5*width, df_mag['AffectedNodes_Dyn'], width, label='Affected Nodes', color='orange')
    
    ax.set_xlabel('Magnitude')
    ax.set_ylabel('Count (avg)')
    ax.set_title('Dynamic Algorithm Metrics by Magnitude')
    ax.set_xticks(x)
    ax.set_xticklabels(df_mag['Magnitude'])
    ax.legend()
    ax.set_yscale('log')
    plt.grid(True, axis='y', alpha=0.3)
    plt.savefig(os.path.join(plots_dir, 'metrics_by_magnitude.png'))
    plt.close()
    
    print(f"Detailed metrics plots saved to {plots_dir}")

def main():
    # Try to find the scientific file first, then legacy
    files = ['results_scientific.csv', 'results.csv']
    csv_file = None
    
    # Allow command line argument to override
    if len(sys.argv) > 1:
        if os.path.exists(sys.argv[1]):
            csv_file = sys.argv[1]
        else:
            print(f"Provided file {sys.argv[1]} does not exist.")
            return
    else:
        for f in files:
            if os.path.exists(f):
                csv_file = f
                break
    
    if not csv_file:
        print("No result CSV files found in directory.")
        return

    print(f"Processing {csv_file}...")
    try:
        df = pd.read_csv(csv_file)
    except Exception as e:
        print(f"Error reading CSV: {e}")
        return

    # Detection logic based on columns
    has_new_metrics = 'HeapOps_Static' in df.columns and 'ScannedEdges_Static' in df.columns
    has_old_format = 'Ops_Static' in df.columns
    has_legacy = 'Time_Static_us' in df.columns
    
    if has_new_metrics:
        print("Detected Format: New Scientific Benchmark (with detailed metrics)")
        plot_scientific(df)
        plot_metrics(df)
    elif has_old_format:
        print("Detected Format: Old Scientific Benchmark")
        plot_scientific(df)
    elif has_legacy:
        print("Detected Format: Legacy Benchmark")
        plot_legacy(df)
    else:
        print("Unknown CSV format. Columns found:", df.columns.tolist())

if __name__ == "__main__":
    main()
