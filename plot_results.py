
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
    # Matplotlib boxplot requires a list of arrays
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
    # Aggregate mean time per N to clean up the plot
    df_mean = df.groupby(['Graph_N', 'Magnitude']).agg({
        'Time_Static_ns': 'mean',
        'Time_Dyn_ns': 'mean'
    }).reset_index()

    plt.figure(figsize=(10, 6))
    
    # Iterate over Magnitudes (Small, Large)
    magnitudes = df_mean['Magnitude'].unique()
    colors = {'Small': 'blue', 'Large': 'red'}
    markers = {'Small': 'o', 'Large': 's'}
    
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

    # --- 3. Operational Efficiency (Ops vs Scale) ---
    df_mean_ops = df.groupby('Graph_N').agg({
        'Ops_Static': 'mean',
        'Ops_Dyn': 'mean'
    }).reset_index()

    plt.figure(figsize=(10, 6))
    plt.plot(df_mean_ops['Graph_N'], df_mean_ops['Ops_Static'], 'r-o', label='Static Ops')
    plt.plot(df_mean_ops['Graph_N'], df_mean_ops['Ops_Dyn'], 'g-o', label='Dynamic Ops')
    plt.yscale('log')
    plt.xscale('log')
    plt.title('Algorithmic Operations vs Graph Size')
    plt.xlabel('Nodes (N)')
    plt.ylabel('Operations (Count)')
    plt.legend()
    plt.grid(True)
    plt.savefig(os.path.join(plots_dir, 'efficiency_ops.png'))
    plt.close()

    # --- 4. Impact of Update Type (Inc vs Dec) ---
    # Simple bar chart of averages
    df_type = df.groupby('Type')['Speedup'].mean().reset_index()
    
    plt.figure(figsize=(8, 6))
    plt.bar(df_type['Type'], df_type['Speedup'], color=['skyblue', 'salmon'])
    plt.title('Average Speedup: Incremental vs Decremental')
    plt.ylabel('Average Speedup')
    plt.savefig(os.path.join(plots_dir, 'speedup_inc_dec.png'))
    plt.close()

    print(f"Scientific plots saved to {plots_dir}")

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

    # Detection logic
    if 'Graph_N' in df.columns and 'Magnitude' in df.columns:
        print("Detected Format: Scientific Benchmark")
        plot_scientific(df)
    elif 'Time_Static_us' in df.columns:
        print("Detected Format: Legacy Benchmark")
        plot_legacy(df)
    else:
        print("Unknown CSV format. Columns found:", df.columns)

if __name__ == "__main__":
    main()
