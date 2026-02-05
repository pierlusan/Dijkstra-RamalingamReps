#!/usr/bin/env python3
"""
Script per plottare i risultati del doubling experiment di Dijkstra.
Verifica empiricamente la complessità O(m log n).
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

import sys
from pathlib import Path

def main(csv_filename):
    # Leggi dati
    print(f"Lettura dati da: {csv_filename}")
    try:
        df = pd.read_csv(csv_filename)
    except FileNotFoundError:
        print(f"Errore: File '{csv_filename}' non trovato.")
        return

    # Determina cartella di output
    # Se il CSV ha la colonna 'filename', usa la cartella di quei file
    if 'filename' in df.columns and not df.empty:
        first_file = df['filename'].iloc[0]
        # Potrebbe essere un path completo o relativo. 
        # Nel CSV di solito è scritto come 'grafi_densi/graph_n500.txt' o solo 'graph_n500.txt'
        # Se è solo nome file, controlliamo se il main l'ha salvato con path relativo
        
        # Per sicurezza, proviamo a vedere se contiene un separatore di directory
        if '/' in str(first_file) or '\\' in str(first_file):
             graph_dir = Path(first_file).parent
             # Se il path è relativo, assumiamo sia relativo alla CWD corrente o alla posizione del CSV?
             # Il main salva 'grafi_densi/graph_n500.txt' nel campo filename se chiamato con 'grafi_densi'
             output_dir = Path(graph_dir)
        else:
             # Se non c'è info sul path nel filename, usiamo la cartella del CSV
             output_dir = Path(csv_filename).parent
    else:
        output_dir = Path(csv_filename).parent
    
    # Crea folder se non esiste (può capitare se path relativo strano)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Nome base per il file di output
    base_name = Path(csv_filename).stem
    
    # ... (rest of the code) ...
    
    # Converti '-' a NaN per i ratio
    if 'ratio' in df.columns:
        df['ratio'] = pd.to_numeric(df['ratio'], errors='coerce')
    if 'expected_ratio' in df.columns:
        df['expected_ratio'] = pd.to_numeric(df['expected_ratio'], errors='coerce')
    
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
    if 'ratio' in df.columns:
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
    mean_ratio = df['ratio'].mean() if 'ratio' in df.columns else 0
    std_ratio = df['ratio'].std() if 'ratio' in df.columns else 0
    cv_normalized = normalized_time.std() / normalized_time.mean() * 100
    
    summary_text = f"""
    RIEPILOGO DOUBLING EXPERIMENT
    ─────────────────────────────────
    
    File: {base_name}
    Range testato: n = {df['n'].min():,} → {df['n'].max():,}
    
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
    
    output_png = output_dir / f'{base_name}_plot.png'
    
    plt.savefig(output_png, dpi=150, bbox_inches='tight')
    print(f"Grafici salvati in: {output_png}")
    
    plt.show()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 plot_dijkstra.py <csv_file>")
        sys.exit(1)
    
    csv_file = sys.argv[1]
    main(csv_file)
