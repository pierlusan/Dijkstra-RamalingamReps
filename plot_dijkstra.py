#!/usr/bin/env python3
"""
Script per plottare i risultati del doubling experiment di Dijkstra.
Verifica empiricamente la complessità O(m log n).
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

def main():
    # Leggi dati
    df = pd.read_csv('dijkstra_densi.csv')
    
    # Converti '-' a NaN per i ratio
    df['ratio'] = pd.to_numeric(df['ratio'], errors='coerce')
    df['expected_ratio'] = pd.to_numeric(df['expected_ratio'], errors='coerce')
    
    # Crea directory plots se non esiste
    os.makedirs('plots', exist_ok=True)
    
    # --- PLOT 1: Tempo vs n (log-log) ---
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    
    # Plot 1: Log-log del tempo
    ax1 = axes[0, 0]
    ax1.loglog(df['n'], df['time_us'], 'bo-', linewidth=2, markersize=8, label='Dijkstra misurato')
    
    # Fit lineare in log-log per trovare l'esponente
    log_n = np.log(df['n'])
    log_t = np.log(df['time_us'])
    slope, intercept = np.polyfit(log_n, log_t, 1)
    fitted_t = np.exp(intercept) * df['n'] ** slope
    ax1.loglog(df['n'], fitted_t, 'r--', linewidth=2, label=f'Fit: O(n^{slope:.2f})')
    
    # Linea di riferimento O(n log n)
    ref_nlogn = df['n'] * np.log2(df['n'])
    ref_nlogn_scaled = ref_nlogn * (df['time_us'].iloc[0] / ref_nlogn.iloc[0])
    ax1.loglog(df['n'], ref_nlogn_scaled, 'g:', linewidth=2, label='O(n log n) riferimento')
    
    ax1.set_xlabel('n (numero nodi)', fontsize=12)
    ax1.set_ylabel('Tempo (µs)', fontsize=12)
    ax1.set_title('Tempo di Esecuzione vs Dimensione (log-log)', fontsize=14)
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Ratio osservato vs atteso
    ax2 = axes[0, 1]
    valid_idx = df['ratio'].notna()
    ax2.plot(df.loc[valid_idx, 'n'], df.loc[valid_idx, 'ratio'], 'bo-', 
             linewidth=2, markersize=8, label='Ratio osservato T(2n)/T(n)')
    ax2.plot(df.loc[valid_idx, 'n'], df.loc[valid_idx, 'expected_ratio'], 'r--', 
             linewidth=2, label='Ratio atteso (O(n log n))')
    ax2.axhline(y=2.0, color='gray', linestyle=':', alpha=0.5, label='Ratio = 2')
    
    ax2.set_xlabel('n', fontsize=12)
    ax2.set_ylabel('Ratio T(2n) / T(n)', fontsize=12)
    ax2.set_title('Doubling Ratio', fontsize=14)
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    ax2.set_xscale('log')
    
    # Plot 3: Tempo / (n log n) - dovrebbe essere costante
    ax3 = axes[1, 0]
    normalized_time = df['time_us'] / (df['n'] * np.log2(df['n']))
    ax3.plot(df['n'], normalized_time, 'go-', linewidth=2, markersize=8)
    ax3.axhline(y=normalized_time.mean(), color='r', linestyle='--', 
                label=f'Media = {normalized_time.mean():.4f}')
    
    ax3.set_xlabel('n', fontsize=12)
    ax3.set_ylabel('Tempo / (n log₂ n)', fontsize=12)
    ax3.set_title('Tempo Normalizzato (dovrebbe essere ~costante)', fontsize=14)
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    ax3.set_xscale('log')
    
    # Plot 4: Tabella riepilogo
    ax4 = axes[1, 1]
    ax4.axis('off')
    
    # Statistiche
    mean_ratio = df['ratio'].mean()
    std_ratio = df['ratio'].std()
    cv_normalized = normalized_time.std() / normalized_time.mean() * 100
    
    summary_text = f"""
    RIEPILOGO DOUBLING EXPERIMENT
    ─────────────────────────────────
    
    Range testato: n = {df['n'].min():,} → {df['n'].max():,}
    Archi: m = 4n (grafo sparso)
    
    ANALISI COMPLESSITÀ:
    • Esponente fit log-log: {slope:.3f}
      (atteso ~1.0 per O(n log n))
    
    • Ratio medio T(2n)/T(n): {mean_ratio:.3f} ± {std_ratio:.3f}
      (atteso ~2.0-2.3 per O(n log n))
    
    • Variazione tempo normalizzato: {cv_normalized:.1f}%
      (basso = complessità confermata)
    
    CONCLUSIONE: 
    {"✓ Complessità O(m log n) CONFERMATA" if 0.9 < slope < 1.3 else "⚠ Risultati anomali"}
    """
    
    ax4.text(0.1, 0.5, summary_text, transform=ax4.transAxes, 
             fontsize=11, verticalalignment='center', fontfamily='monospace',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    
    plt.tight_layout()
    plt.savefig('plots/dijkstra_complexity.png', dpi=150, bbox_inches='tight')
    plt.savefig('plots/dijkstra_complexity.pdf', bbox_inches='tight')
    print("Grafici salvati in: plots/dijkstra_complexity.png")
    
    plt.show()

if __name__ == '__main__':
    main()
