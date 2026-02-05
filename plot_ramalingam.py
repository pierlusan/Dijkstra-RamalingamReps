#!/usr/bin/env python3
"""
Plot dei risultati del test Ramalingam-Reps.
Verifica complessità O(||δ|| log ||δ||).
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

def main():
    # Leggi dati
    df = pd.read_csv('rr_complexity.csv')
    
    # Filtra righe con delta_size > 0
    df = df[df['delta_size'] > 1]
    
    if len(df) == 0:
        print("Nessun dato valido trovato!")
        return
    
    # Crea directory plots
    os.makedirs('plots', exist_ok=True)
    
    # --- GRAFICI ---
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    
    # Plot 1: Tempo vs ||δ|| (log-log)
    ax1 = axes[0, 0]
    ax1.scatter(df['delta_size'], df['time_us'], alpha=0.6, s=30, c='blue')
    
    # Fit lineare log-log
    valid = (df['delta_size'] > 0) & (df['time_us'] > 0)
    if valid.sum() > 2:
        log_delta = np.log(df.loc[valid, 'delta_size'])
        log_time = np.log(df.loc[valid, 'time_us'])
        slope, intercept = np.polyfit(log_delta, log_time, 1)
        
        x_fit = np.linspace(df['delta_size'].min(), df['delta_size'].max(), 100)
        y_fit = np.exp(intercept) * x_fit ** slope
        ax1.plot(x_fit, y_fit, 'r--', linewidth=2, label=f'Fit: O(||δ||^{slope:.2f})')
    
    ax1.set_xscale('log')
    ax1.set_yscale('log')
    ax1.set_xlabel('||δ|| (nodi + archi incidenti)', fontsize=12)
    ax1.set_ylabel('Tempo (µs)', fontsize=12)
    ax1.set_title('Tempo vs ||δ|| (log-log)', fontsize=14)
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Tempo normalizzato vs ||δ||
    ax2 = axes[0, 1]
    df['normalized'] = df['time_us'] / (df['delta_size'] * np.log2(df['delta_size']))
    ax2.scatter(df['delta_size'], df['normalized'], alpha=0.6, s=30, c='green')
    ax2.axhline(y=df['normalized'].median(), color='r', linestyle='--', 
                label=f'Median = {df["normalized"].median():.4f}')
    
    ax2.set_xscale('log')
    ax2.set_xlabel('||δ||', fontsize=12)
    ax2.set_ylabel('Tempo / (||δ|| log ||δ||)', fontsize=12)
    ax2.set_title('Tempo Normalizzato (dovrebbe essere ~costante)', fontsize=14)
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # Plot 3: Distribuzione ||δ|| per Categorie (Bar Chart)
    ax3 = axes[1, 0]
    
    # Definisci bin e etichette
    bins = [0, 10, 100, 1000, 10000, float('inf')]
    labels = ['Tiny\n(<10)', 'Small\n(10-100)', 'Medium\n(100-1k)', 'Large\n(1k-10k)', 'Massive\n(>10k)']
    
    # Crea categorie e conta
    df['category'] = pd.cut(df['delta_size'], bins=bins, labels=labels, right=False)
    counts = df['category'].value_counts().sort_index()
    
    # Plot bars
    bars = ax3.bar(labels, counts, color=['#cec2eb', '#b5a3e1', '#9c84d7', '#8265cc', '#6946c2'], 
                  edgecolor='black', alpha=0.8)
    
    # Aggiungi etichette sopra le barre
    for bar in bars:
        height = bar.get_height()
        if height > 0:
            ax3.text(bar.get_x() + bar.get_width()/2., height,
                    f'{int(height)}',
                    ha='center', va='bottom')
    
    ax3.set_ylabel('Numero di Update', fontsize=12)
    ax3.set_title('Distribuzione Dimensione Update (||δ||)', fontsize=14)
    ax3.grid(True, axis='y', alpha=0.3)
    
    # Plot 4: Riepilogo
    ax4 = axes[1, 1]
    ax4.axis('off')
    
    # Statistiche
    correct_pct = df['dijkstra_match'].mean() * 100
    cv = df['normalized'].std() / df['normalized'].mean() * 100 if df['normalized'].mean() > 0 else 0
    
    summary_text = f"""
    RIEPILOGO TEST RAMALINGAM-REPS
    ─────────────────────────────────
    
    Update testati: {len(df)}
    Range ||δ||: {df['delta_size'].min():,} → {df['delta_size'].max():,}
    
    ANALISI COMPLESSITÀ:
    • Esponente fit log-log: {slope:.3f}
      (atteso ~1.0 per O(||δ|| log ||δ||))
    
    • Coefficiente variazione normalizzato: {cv:.1f}%
      (basso = complessità confermata)
    
    CORRETTEZZA:
    • Match con Dijkstra: {correct_pct:.1f}%
    
    CONCLUSIONE: 
    {"✓ Complessità O(||δ|| log ||δ||) CONFERMATA" if 0.8 < slope < 1.4 else "⚠ Risultati anomali"}
    {"✓ Correttezza VERIFICATA" if correct_pct == 100 else "⚠ Errori rilevati"}
    """
    
    ax4.text(0.1, 0.5, summary_text, transform=ax4.transAxes, 
             fontsize=11, verticalalignment='center', fontfamily='monospace',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    
    plt.tight_layout()
    plt.savefig('plots/rr_complexity.png', dpi=150, bbox_inches='tight')
    plt.savefig('plots/rr_complexity.pdf', bbox_inches='tight')
    print("Grafici salvati in: plots/rr_complexity.png")
    
    plt.show()

if __name__ == '__main__':
    main()
