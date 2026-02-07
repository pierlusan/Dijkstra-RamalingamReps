#!/usr/bin/env python3
"""
Benchmark Visualization Script for Dijkstra vs Ramalingam-Reps comparison.

Generates 5 key plots:
1. Speedup vs Graph Size (N) - Boxplot distribution
2. Speedup vs Affected Nodes - The "Killer" graph showing locality impact
3. Heap Ops vs Graph Size - Queue efficiency comparison
4. Inc vs Dec Analysis - Retraction cost comparison (Box/Violin plot)
5. Affected Nodes vs Time_Dyn - Correlation validation scatter plot
"""

import pandas as pd
import matplotlib.pyplot as plt
import os
import sys
import numpy as np
from scipy import stats

# Stile professionale per i grafici
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams['figure.dpi'] = 150
plt.rcParams['font.size'] = 11
plt.rcParams['axes.titlesize'] = 14
plt.rcParams['axes.labelsize'] = 12


def setup_plots_dir():
    """Create plots directory if it doesn't exist."""
    path = 'plots'
    if not os.path.exists(path):
        os.makedirs(path)
    return path


def plot_speedup_vs_scale(df, plots_dir):
    """
    Plot 1: Speedup Distribution by Graph Size (N)
    Boxplot showing speedup distribution for each graph size.
    """
    print("  [1/5] Speedup vs Graph Size...")
    
    plt.figure(figsize=(12, 7))
    
    df['Graph_N'] = df['Graph_N'].astype(int)
    Ns = sorted(df['Graph_N'].unique())
    data_to_plot = [df[df['Graph_N'] == n]['Speedup'].values for n in Ns]
    
    # Boxplot con outliers
    bp = plt.boxplot(data_to_plot, labels=[f"{n:,}" for n in Ns], patch_artist=True)
    
    # Colora i box
    colors = plt.cm.viridis(np.linspace(0.2, 0.8, len(Ns)))
    for patch, color in zip(bp['boxes'], colors):
        patch.set_facecolor(color)
        patch.set_alpha(0.7)
    
    plt.title('Speedup Distribution by Graph Size', fontweight='bold')
    plt.xlabel('Number of Nodes (N)')
    plt.ylabel('Speedup Factor ($t_{static} / t_{dynamic}$)')
    plt.yscale('log')
    
    # Linea di riferimento a speedup = 1
    plt.axhline(y=1, color='red', linestyle='--', linewidth=1.5, alpha=0.7, label='Speedup = 1')
    plt.legend(loc='upper left')
    
    plt.grid(True, which="both", ls="--", alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(plots_dir, '1_speedup_vs_scale.png'))
    plt.close()


def plot_speedup_vs_locality(df, plots_dir):
    """
    Plot 2: Speedup vs Affected Nodes (The "Killer" Graph)
    
    Shows how speedup depends on locality - small changes = huge speedup,
    changes near source = speedup ~ 1.
    """
    print("  [2/5] Speedup vs Locality (Affected Nodes)...")
    
    plt.figure(figsize=(12, 7))
    
    # Filtra valori validi (affected_nodes > 0, speedup > 0)
    df_valid = df[(df['AffectedNodes_Dyn'] > 0) & (df['Speedup'] > 0)].copy()
    
    if df_valid.empty:
        print("    WARNING: No valid data for locality plot (AffectedNodes_Dyn all zero)")
        plt.close()
        return
    
    # Scatter plot con alpha per densità
    scatter = plt.scatter(
        df_valid['AffectedNodes_Dyn'], 
        df_valid['Speedup'],
        c=df_valid['Graph_N'],
        cmap='plasma',
        alpha=0.5,
        s=20,
        edgecolors='none'
    )
    
    # Colorbar per indicare la dimensione del grafo
    cbar = plt.colorbar(scatter)
    cbar.set_label('Graph Size (N)')
    
    # Linea di tendenza (regressione log-log)
    x_log = np.log10(df_valid['AffectedNodes_Dyn'])
    y_log = np.log10(df_valid['Speedup'])
    
    # Rimuovi infiniti/NaN
    mask = np.isfinite(x_log) & np.isfinite(y_log)
    if mask.sum() > 10:
        slope, intercept, r_value, _, _ = stats.linregress(x_log[mask], y_log[mask])
        x_fit = np.linspace(x_log[mask].min(), x_log[mask].max(), 100)
        y_fit = slope * x_fit + intercept
        plt.plot(10**x_fit, 10**y_fit, 'r-', linewidth=2, 
                 label=f'Trend: slope={slope:.2f}, R²={r_value**2:.2f}')
    
    plt.xscale('log')
    plt.yscale('log')
    
    plt.title('Speedup vs Locality: The "Killer" Graph', fontweight='bold')
    plt.xlabel('Affected Nodes (||δ||)')
    plt.ylabel('Speedup ($t_{static} / t_{dynamic}$)')
    
    # Linea di riferimento
    plt.axhline(y=1, color='gray', linestyle='--', linewidth=1, alpha=0.7, label='Speedup = 1')
    
    plt.legend(loc='upper right')
    plt.grid(True, which="both", ls="--", alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(plots_dir, '2_speedup_vs_locality.png'))
    plt.close()


def plot_heap_ops_efficiency(df, plots_dir):
    """
    Plot 3: Heap Operations vs Graph Size
    
    Shows queue efficiency - Dijkstra grows as O(k * M log N),
    Ramalingam-Reps should be much flatter (proportional to ||δ||).
    """
    print("  [3/5] Heap Ops Efficiency...")
    
    plt.figure(figsize=(12, 7))
    
    # Raggruppa per dimensione del grafo e calcola media
    df_mean = df.groupby(['Graph_N', 'Graph_M']).agg({
        'HeapOps_Static': 'mean',
        'HeapOps_Dyn': 'mean'
    }).reset_index()
    
    df_mean = df_mean.sort_values('Graph_N')
    
    # Plot delle due linee
    plt.plot(df_mean['Graph_N'], df_mean['HeapOps_Static'], 
             'r-o', linewidth=2, markersize=8, label='Static Dijkstra', alpha=0.8)
    plt.plot(df_mean['Graph_N'], df_mean['HeapOps_Dyn'], 
             'g-s', linewidth=2, markersize=8, label='Dynamic Ramalingam-Reps', alpha=0.8)
    
    # Aggiungi annotazioni per il rapporto
    for i, row in df_mean.iterrows():
        if row['HeapOps_Dyn'] > 0:
            ratio = row['HeapOps_Static'] / row['HeapOps_Dyn']
            plt.annotate(f'{ratio:.1f}x', 
                        xy=(row['Graph_N'], row['HeapOps_Static']),
                        xytext=(5, 5), textcoords='offset points',
                        fontsize=8, alpha=0.7)
    
    plt.xscale('log')
    plt.yscale('log')
    
    plt.title('Queue Efficiency: Heap Operations vs Graph Size', fontweight='bold')
    plt.xlabel('Number of Nodes (N)')
    plt.ylabel('Average Heap Operations (per update)')
    plt.legend(loc='upper left')
    
    plt.grid(True, which="both", ls="--", alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(plots_dir, '3_heap_ops_efficiency.png'))
    plt.close()


def plot_retraction_analysis(df, plots_dir):
    """
    Plot 4: Weight Increase vs Decrease Analysis (Retraction Cost)
    
    Clear bar chart with jittered points comparing Inc vs Dec operations.
    Retraction (weight increase) is typically more expensive.
    """
    print("  [4/5] Retraction Analysis (Inc vs Dec)...")
    
    # Filtra solo Inc e Dec (esclude Add/Del che sono strutturali)
    df_weight = df[df['Type'].isin(['Inc', 'Dec'])].copy()
    
    if df_weight.empty:
        print("    WARNING: No Inc/Dec data for retraction analysis")
        return
    
    # Calcola statistiche
    inc_time = df_weight[df_weight['Type'] == 'Inc']['Time_Dyn_ns'].values / 1000  # to μs
    dec_time = df_weight[df_weight['Type'] == 'Dec']['Time_Dyn_ns'].values / 1000
    inc_edges = df_weight[df_weight['Type'] == 'Inc']['ScannedEdges_Dyn'].values
    dec_edges = df_weight[df_weight['Type'] == 'Dec']['ScannedEdges_Dyn'].values
    
    if len(inc_time) == 0 or len(dec_time) == 0:
        print("    WARNING: Insufficient data for comparison")
        return
    
    # Statistiche aggregate
    stats_data = {
        'Time (μs)': {
            'Dec': {'median': np.median(dec_time), 'q25': np.percentile(dec_time, 25), 'q75': np.percentile(dec_time, 75)},
            'Inc': {'median': np.median(inc_time), 'q25': np.percentile(inc_time, 25), 'q75': np.percentile(inc_time, 75)}
        },
        'Scanned Edges': {
            'Dec': {'median': np.median(dec_edges), 'q25': np.percentile(dec_edges, 25), 'q75': np.percentile(dec_edges, 75)},
            'Inc': {'median': np.median(inc_edges), 'q25': np.percentile(inc_edges, 25), 'q75': np.percentile(inc_edges, 75)}
        }
    }
    
    fig, axes = plt.subplots(1, 3, figsize=(15, 6))
    
    colors = {'Dec': '#27ae60', 'Inc': '#c0392b'}  # Verde per Dec, Rosso per Inc
    
    # --- Subplot 1: Execution Time Bar Chart ---
    ax1 = axes[0]
    
    x_pos = [0, 1]
    labels = ['Decrease\n(Propagation)', 'Increase\n(Retraction)']
    medians = [stats_data['Time (μs)']['Dec']['median'], stats_data['Time (μs)']['Inc']['median']]
    errors_low = [medians[0] - stats_data['Time (μs)']['Dec']['q25'], 
                  medians[1] - stats_data['Time (μs)']['Inc']['q25']]
    errors_high = [stats_data['Time (μs)']['Dec']['q75'] - medians[0], 
                   stats_data['Time (μs)']['Inc']['q75'] - medians[1]]
    
    bars1 = ax1.bar(x_pos, medians, color=[colors['Dec'], colors['Inc']], 
                    edgecolor='black', linewidth=1.2, alpha=0.8, width=0.6)
    ax1.errorbar(x_pos, medians, yerr=[errors_low, errors_high], 
                 fmt='none', color='black', capsize=8, capthick=2, linewidth=2)
    
    # Aggiungi punti jittered
    jitter_dec = np.random.normal(0, 0.08, len(dec_time))
    jitter_inc = np.random.normal(1, 0.08, len(inc_time))
    ax1.scatter(jitter_dec, dec_time, color=colors['Dec'], alpha=0.3, s=8, zorder=5)
    ax1.scatter(jitter_inc, inc_time, color=colors['Inc'], alpha=0.3, s=8, zorder=5)
    
    ax1.set_xticks(x_pos)
    ax1.set_xticklabels(labels, fontsize=11)
    ax1.set_ylabel('Execution Time (μs)', fontsize=12)
    ax1.set_title('Execution Time', fontweight='bold', fontsize=13)
    ax1.set_yscale('log')
    ax1.grid(True, axis='y', alpha=0.3, linestyle='--')
    
    # Annotazione valori
    for i, (bar, med) in enumerate(zip(bars1, medians)):
        ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() * 1.1, 
                f'{med:.1f}', ha='center', va='bottom', fontweight='bold', fontsize=10)
    
    # --- Subplot 2: Scanned Edges Bar Chart ---
    ax2 = axes[1]
    
    medians_e = [stats_data['Scanned Edges']['Dec']['median'], stats_data['Scanned Edges']['Inc']['median']]
    errors_low_e = [medians_e[0] - stats_data['Scanned Edges']['Dec']['q25'], 
                    medians_e[1] - stats_data['Scanned Edges']['Inc']['q25']]
    errors_high_e = [stats_data['Scanned Edges']['Dec']['q75'] - medians_e[0], 
                     stats_data['Scanned Edges']['Inc']['q75'] - medians_e[1]]
    
    bars2 = ax2.bar(x_pos, medians_e, color=[colors['Dec'], colors['Inc']], 
                    edgecolor='black', linewidth=1.2, alpha=0.8, width=0.6)
    ax2.errorbar(x_pos, medians_e, yerr=[errors_low_e, errors_high_e], 
                 fmt='none', color='black', capsize=8, capthick=2, linewidth=2)
    
    # Punti jittered
    ax2.scatter(jitter_dec[:len(dec_edges)], dec_edges, color=colors['Dec'], alpha=0.3, s=8, zorder=5)
    ax2.scatter(jitter_inc[:len(inc_edges)], inc_edges, color=colors['Inc'], alpha=0.3, s=8, zorder=5)
    
    ax2.set_xticks(x_pos)
    ax2.set_xticklabels(labels, fontsize=11)
    ax2.set_ylabel('Scanned Edges', fontsize=12)
    ax2.set_title('Edges Examined', fontweight='bold', fontsize=13)
    ax2.set_yscale('log')
    ax2.grid(True, axis='y', alpha=0.3, linestyle='--')
    
    for i, (bar, med) in enumerate(zip(bars2, medians_e)):
        ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() * 1.1, 
                f'{med:.0f}', ha='center', va='bottom', fontweight='bold', fontsize=10)
    
    # --- Subplot 3: Ratio Comparison ---
    ax3 = axes[2]
    
    time_ratio = stats_data['Time (μs)']['Inc']['median'] / max(stats_data['Time (μs)']['Dec']['median'], 1e-9)
    edges_ratio = stats_data['Scanned Edges']['Inc']['median'] / max(stats_data['Scanned Edges']['Dec']['median'], 1)
    
    metrics = ['Time', 'Edges']
    ratios = [time_ratio, edges_ratio]
    bar_colors = ['#3498db', '#9b59b6']
    
    bars3 = ax3.barh(metrics, ratios, color=bar_colors, edgecolor='black', linewidth=1.2, height=0.5)
    
    # Linea di riferimento a ratio = 1
    ax3.axvline(x=1, color='red', linestyle='--', linewidth=2, alpha=0.7, label='Equal Cost')
    
    ax3.set_xlabel('Ratio (Increase / Decrease)', fontsize=12)
    ax3.set_title('Cost Ratio: Inc vs Dec', fontweight='bold', fontsize=13)
    ax3.set_xlim(0, max(ratios) * 1.3)
    ax3.grid(True, axis='x', alpha=0.3, linestyle='--')
    
    # Annotazione
    for bar, ratio in zip(bars3, ratios):
        width = bar.get_width()
        ax3.text(width + 0.1, bar.get_y() + bar.get_height()/2, 
                f'{ratio:.1f}x', ha='left', va='center', fontweight='bold', fontsize=12)
    
    # Interpretazione
    if time_ratio > 2:
        verdict = "⚠ Retraction significantly more expensive"
        verdict_color = '#e74c3c'
    elif time_ratio > 1.2:
        verdict = "⚡ Retraction moderately more expensive"
        verdict_color = '#f39c12'
    else:
        verdict = "✓ Similar cost for both operations"
        verdict_color = '#27ae60'
    
    ax3.text(0.5, -0.15, verdict, transform=ax3.transAxes, ha='center', 
             fontsize=11, fontweight='bold', color=verdict_color)
    
    plt.suptitle('Retraction Analysis: Weight Increase vs Decrease', 
                 fontsize=15, fontweight='bold', y=1.02)
    plt.tight_layout()
    plt.savefig(os.path.join(plots_dir, '4_retraction_analysis.png'), bbox_inches='tight', dpi=150)
    plt.close()


def plot_correlation_validation(df, plots_dir):
    """
    Plot 5: Affected Nodes vs Time_Dyn (Correlation Heatmap/Scatter)
    
    Validates implementation: if points form a clean line,
    the algorithm is truly output-bounded.
    """
    print("  [5/5] Correlation Validation (Affected Nodes vs Time)...")
    
    plt.figure(figsize=(12, 7))
    
    # Filtra dati validi
    df_valid = df[(df['AffectedNodes_Dyn'] > 0) & (df['Time_Dyn_ns'] > 0)].copy()
    
    if df_valid.empty:
        print("    WARNING: No valid data for correlation plot")
        plt.close()
        return
    
    # Calcola densità per colorazione (hex bin style con scatter)
    x = df_valid['AffectedNodes_Dyn']
    y = df_valid['Time_Dyn_ns'] / 1000  # Convert to μs
    
    # Scatter plot colorato per tipo di operazione
    types = df_valid['Type'].unique()
    colors_map = {'Inc': '#e74c3c', 'Dec': '#2ecc71', 'Add': '#3498db', 'Del': '#9b59b6'}
    
    for t in types:
        mask = df_valid['Type'] == t
        plt.scatter(x[mask], y[mask], 
                   c=colors_map.get(t, 'gray'),
                   alpha=0.4, s=15, label=t, edgecolors='none')
    
    # Regressione lineare su scala log-log
    x_log = np.log10(x)
    y_log = np.log10(y)
    mask = np.isfinite(x_log) & np.isfinite(y_log)
    
    if mask.sum() > 10:
        slope, intercept, r_value, _, std_err = stats.linregress(x_log[mask], y_log[mask])
        
        x_fit = np.linspace(x_log[mask].min(), x_log[mask].max(), 100)
        y_fit = slope * x_fit + intercept
        
        plt.plot(10**x_fit, 10**y_fit, 'k-', linewidth=2.5, 
                 label=f'Linear Fit (R² = {r_value**2:.3f})')
        
        # Bande di confidenza
        y_upper = (slope + std_err) * x_fit + intercept
        y_lower = (slope - std_err) * x_fit + intercept
        plt.fill_between(10**x_fit, 10**y_lower, 10**y_upper, alpha=0.2, color='gray')
        
        # Annotazione con interpretazione
        if r_value**2 > 0.8:
            interpretation = "✓ Strong correlation: Output-bounded behavior confirmed"
            color = 'green'
        elif r_value**2 > 0.5:
            interpretation = "⚠ Moderate correlation: Some overhead present"
            color = 'orange'
        else:
            interpretation = "✗ Weak correlation: Check for bottlenecks"
            color = 'red'
        
        plt.text(0.02, 0.98, f'Slope: {slope:.2f}\nR²: {r_value**2:.3f}\n{interpretation}',
                transform=plt.gca().transAxes, fontsize=10, verticalalignment='top',
                bbox=dict(boxstyle='round', facecolor='white', edgecolor=color, alpha=0.9))
    
    plt.xscale('log')
    plt.yscale('log')
    
    plt.title('Algorithm Validation: Time vs Affected Nodes', fontweight='bold')
    plt.xlabel('Affected Nodes (||δ||)')
    plt.ylabel('Dynamic Algorithm Time (μs)')
    plt.legend(loc='lower right')
    
    plt.grid(True, which="both", ls="--", alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(plots_dir, '5_correlation_validation.png'))
    plt.close()


def main():
    """Main entry point."""
    print("=" * 60)
    print("  Dijkstra vs Ramalingam-Reps Benchmark Visualization")
    print("=" * 60)
    
    # Trova il file CSV
    default_files = ['results_scientific.csv', 'results.csv']
    csv_file = None
    
    if len(sys.argv) > 1:
        if os.path.exists(sys.argv[1]):
            csv_file = sys.argv[1]
        else:
            print(f"ERROR: File '{sys.argv[1]}' not found.")
            return 1
    else:
        for f in default_files:
            if os.path.exists(f):
                csv_file = f
                break
    
    if not csv_file:
        print("ERROR: No CSV file found. Usage: python plot_results.py <results.csv>")
        return 1
    
    print(f"\nLoading: {csv_file}")
    
    try:
        df = pd.read_csv(csv_file)
    except Exception as e:
        print(f"ERROR reading CSV: {e}")
        return 1
    
    # Verifica colonne richieste
    required_cols = ['Graph_N', 'Speedup', 'HeapOps_Static', 'HeapOps_Dyn', 
                     'AffectedNodes_Dyn', 'Time_Dyn_ns', 'Type']
    missing = [c for c in required_cols if c not in df.columns]
    
    if missing:
        print(f"ERROR: Missing required columns: {missing}")
        print(f"Available columns: {df.columns.tolist()}")
        return 1
    
    print(f"Loaded {len(df)} rows, {df['Graph_N'].nunique()} unique graph sizes")
    print(f"Update types: {df['Type'].value_counts().to_dict()}")
    
    # Crea directory per i plot
    plots_dir = setup_plots_dir()
    print(f"\nOutput directory: {plots_dir}/")
    print("\nGenerating plots:")
    
    # Genera i 5 grafici
    plot_speedup_vs_scale(df, plots_dir)
    plot_speedup_vs_locality(df, plots_dir)
    plot_heap_ops_efficiency(df, plots_dir)
    plot_retraction_analysis(df, plots_dir)
    plot_correlation_validation(df, plots_dir)
    
    print("\n" + "=" * 60)
    print(f"  ✓ All plots saved to: {plots_dir}/")
    print("=" * 60)
    
    # Lista file generati
    for f in sorted(os.listdir(plots_dir)):
        if f.endswith('.png'):
            print(f"    • {f}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
