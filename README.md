# Guida Comandi - Algoritmi SSSP

## Build

Tutti i target vengono compilati con CMake in modalità **Release** (`-O3 -DNDEBUG`) per la massima efficienza.

```bash
cd /home/pierluca/Desktop/Algorithm-Engineering/Dijkstra

# Build tutti i target
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake --build build -j4

# Build singolo target
cmake --build build --target dijkstra_main -j4
cmake --build build --target graph_generator -j4
cmake --build build --target test_ramalingam -j4
cmake --build build --target scientific_benchmark -j4
```

---

## 1. Test Dijkstra - Complessità O(m log n)

### Generazione Grafi
Lo script genera tre tipologie di grafi (Sparsi uniformi, Scale-free/Barabasi, Densi) nelle rispettive cartelle.
```bash
./build/graph_generator
```

### Esecuzione Benchmark
Il driver accetta come argomento la cartella contenente i grafi da testare.
```bash
# Esegui benchmark (redirigi output su file CSV se desiderato per i plot)
./build/dijkstra_main grafi_sparsi > risultati_sparsi/dijkstra_sparsi.csv
./build/dijkstra_main grafi_barabasi > risultati_barabasi/dijkstra_barabasi.csv
./build/dijkstra_main grafi_densi > risultati_densi/dijkstra_densi.csv
```
*Nota: I file contenenti le distanze calcolate (`distances_n*.csv`) vengono salvati automaticamente nelle cartelle dei grafi.*

### Verifica Correttezza
Confronta i risultati di Dijkstra C++ con `networkx`. Richiede come argomento la cartella dei grafi.
*Nota: lo script salta automaticamente i file grafi > 200MB (es. grafi densi molto grandi) per evitare crash dovuti all'eccessivo consumo di RAM di Python.*

```bash
./verify_dijkstra.py grafi_sparsi
./verify_dijkstra.py grafi_barabasi
./verify_dijkstra.py grafi_densi
```

### Plot Risultati
Lo script richiede il file CSV dei risultati come argomento. Il grafico verrà salvato nella stessa cartella del CSV.

```bash
python3 plot_dijkstra.py risultati_densi/dijkstra_densi.csv
python3 plot_dijkstra.py risultati_sparsi/dijkstra_sparsi.csv
python3 plot_dijkstra.py risultati_barabasi/dijkstra_barabasi.csv
```

---

## 2. Test Ramalingam-Reps - Complessità O(||δ|| log ||δ||)

```bash
# Esegue test su USA-road (W+, W-, INS, DEL)
./build/test_ramalingam

# Genera grafici
python3 plot_ramalingam.py
```

**Output:**
- `rr_complexity.csv` - metriche (||δ||, tempo, correttezza)
- `plots/rr_complexity.png`

---

## 3. Benchmark Dijkstra vs Ramalingam-Reps

```bash
# Sintassi
./build/scientific_benchmark <folder_path> [update_factor] [magnitude]

# Esempi
./build/scientific_benchmark ./grafi_vs 0.1 small > risultati_vs/results_small.csv   # 10% di N update
./build/scientific_benchmark ./grafi_vs 0.1 large > risultati_vs/results_large.csv   # N update (Default)
./build/scientific_benchmark ./grafi_vs 0.1 mixed > risultati_vs/results_mixed.csv   # 2*N update

# Genera grafici
python3 plot_results.py risultati_vs/results_small.csv
```

**Parametri:**
| Parametro | Descrizione | Default |
|-----------|-------------|---------|
| `folder_path` | Cartella con grafi (.txt o .gr) | - |
| `update_factor` | Moltiplicatore Update ($K = N \times factor$) | 1.0 |
| `magnitude` | `small` (±10%), `large` (×2), `mixed` | large |

**Output CSV:**
- `Graph_N, Graph_M, Update_ID, Type, Magnitude`
- `Time_Static_ns, HeapOps_Static, ...`
- `Time_Dyn_ns, HeapOps_Dyn, ...`
- `Speedup`

---

## Riepilogo Target

| Target                 | Descrizione                       |
|------------------------|-----------------------------------|
| `graph_generator`      | Generatore grafi (sparsi, BA, densi) |
| `dijkstra_main`        | Doubling experiment Dijkstra      |
| `test_ramalingam`      | Test RR con tutte le operazioni   |
| `scientific_benchmark` | Confronto Dijkstra vs RR          |
