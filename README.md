# Guida Comandi - Algoritmi SSSP

## Build

```bash
cd /home/pierluca/Desktop/Algorithm-Engineering/Dijkstra

# Build tutti i target
cmake -B build -S . && cmake --build build -j4

# Build singolo target
cmake --build build --target dijkstra_main -j4
cmake --build build --target test_ramalingam -j4
cmake --build build --target scientific_benchmark -j4
```

---

## 1. Test Dijkstra - Complessità O(m log n)

```bash
# Esegue doubling experiment (n = 1K → 1M)
./build/dijkstra_main

# Verifica correttezza con networkx
python3 verify_dijkstra.py

# Genera grafici
python3 plot_dijkstra.py
```

**Output:**
- `dijkstra_doubling.csv` - tempi
- `dijkstra_results/` - grafi e distanze
- `plots/dijkstra_complexity.png`

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
./build/scientific_benchmark <folder_path> [num_updates] [magnitude]

# Esempi
./build/scientific_benchmark ./grafi 100 small > results.csv
./build/scientific_benchmark ./grafi 1000 large > results.csv
./build/scientific_benchmark ./grafi 500 mixed > results.csv

# Genera grafici
python3 plot_results.py results.csv
```

**Parametri:**
| Parametro | Descrizione | Default |
|-----------|-------------|---------|
| `folder_path` | Cartella con grafi (.txt o .gr) | - |
| `num_updates` | Numero update per grafo | 1000 |
| `magnitude` | `small` (±10%), `large` (×2), `mixed` | large |

**Output CSV:**
- `Graph_N, Graph_M, Update_ID, Type, Magnitude`
- `Time_Static_ns, HeapOps_Static, ...`
- `Time_Dyn_ns, HeapOps_Dyn, ...`
- `Speedup`

---

## Riepilogo Target

| Target | Descrizione |
|--------|-------------|
| `dijkstra_main` | Doubling experiment Dijkstra |
| `test_ramalingam` | Test RR con tutte le operazioni |
| `scientific_benchmark` | Confronto Dijkstra vs RR |
