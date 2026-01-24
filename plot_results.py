
import pandas as pd
import matplotlib.pyplot as plt
import os

def main():
    # Read the results file
    csv_file = 'results.csv'
    if not os.path.exists(csv_file):
        print(f"Error: {csv_file} not found.")
        return

    try:
        df = pd.read_csv(csv_file)
    except Exception as e:
        print(f"Error reading {csv_file}: {e}")
        return

    # Create plots directory
    if not os.path.exists('plots'):
        os.makedirs('plots')

    # Ensure data is sorted by UpdateID just in case
    df = df.sort_values('UpdateID')

    # --- Plot 1: Speedup ---
    plt.figure(figsize=(10, 6))
    plt.plot(df['UpdateID'], df['Speedup'], marker='o', linestyle='-', color='b')
    plt.title('Speedup (Static / Dynamic) over Updates')
    plt.xlabel('Update ID')
    plt.ylabel('Speedup Factor')
    plt.grid(True)
    plt.tight_layout()
    plt.savefig('plots/speedup.png')
    print("Saved plots/speedup.png")
    plt.close()

    # --- Plot 2: Time Comparison (Log Scale) ---
    plt.figure(figsize=(10, 6))
    plt.plot(df['UpdateID'], df['Time_Static_us'], label='Static', color='r', alpha=0.7)
    plt.plot(df['UpdateID'], df['Time_Dyn_us'], label='Dynamic', color='g', alpha=0.7)
    plt.yscale('log')
    plt.title('Execution Time: Static vs Dynamic (Log Scale)')
    plt.xlabel('Update ID')
    plt.ylabel('Time (microseconds)')
    plt.legend()
    plt.grid(True, which="both", ls="-", alpha=0.2)
    plt.tight_layout()
    plt.savefig('plots/time_comparison.png')
    print("Saved plots/time_comparison.png")
    plt.close()

    # --- Plot 3: Heap Ops Comparison (Log Scale) ---
    plt.figure(figsize=(10, 6))
    plt.plot(df['UpdateID'], df['HeapOps_Static'], label='Static', color='r', linestyle='--', alpha=0.7)
    plt.plot(df['UpdateID'], df['HeapOps_Dyn'], label='Dynamic', color='g', linestyle='--', alpha=0.7)
    plt.yscale('log')
    plt.title('Heap Operations: Static vs Dynamic (Log Scale)')
    plt.xlabel('Update ID')
    plt.ylabel('Number of Heap Operations')
    plt.legend()
    plt.grid(True, which="both", ls="-", alpha=0.2)
    plt.tight_layout()
    plt.savefig('plots/heap_ops.png')
    print("Saved plots/heap_ops.png")
    plt.close()
    
    # --- Plot 4: Relaxed Comparison (Log Scale) ---
    plt.figure(figsize=(10, 6))
    plt.plot(df['UpdateID'], df['Relax_Static'], label='Static', color='orange', linestyle='--', alpha=0.7)
    plt.plot(df['UpdateID'], df['Relax_Dyn'], label='Dynamic', color='purple', linestyle='--', alpha=0.7)
    plt.yscale('log')
    plt.title('Relaxed Edges: Static vs Dynamic (Log Scale)')
    plt.xlabel('Update ID')
    plt.ylabel('Number of Relaxed Edges')
    plt.legend()
    plt.grid(True, which="both", ls="-", alpha=0.2)
    plt.tight_layout()
    plt.savefig('plots/relaxed_comparison.png')
    print("Saved plots/relaxed_comparison.png")
    plt.close()

if __name__ == "__main__":
    main()
