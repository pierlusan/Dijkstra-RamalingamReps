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

### Generazione Grafi
Lo script genera tre tipologie di grafi (Sparsi uniformi, Scale-free/Barabasi, Densi) nelle rispettive cartelle.
```bash
# Compila ed esegui il generatore
g++ -O3 -std=c++17 -o graph_generator graph_generator.cpp Graph.cpp -lz
./graph_generator
```

### Esecuzione Benchmark
Il driver accetta come argomento la cartella contenente i grafi da testare.
```bash
# Compila il driver
g++ -O3 -std=c++17 -o main main.cpp Graph.cpp DijkstraSolver.cpp -lz

# Esegui benchmark (redirigi output su file CSV se desiderato per i plot)
./main grafi_sparsi > dijkstra_sparsi.csv
./main grafi_barabasi > dijkstra_barabasi.csv
./main grafi_densi > dijkstra_densi.csv
```
*Nota: I file contenenti le distanze calcolate (`distances_n*.csv`) vengono salvati automaticamente nelle cartelle dei grafi.*

### Verifica Correttezza
Confronta i risultati di Dijkstra C++ con `networkx`. Richiede come argomento la cartella dei grafi.
```bash
# Assicurati che lo script sia eseguibile
chmod +x verify_dijkstra.py

./verify_dijkstra.py grafi_sparsi
./verify_dijkstra.py grafi_barabasi
./verify_dijkstra.py grafi_densi
```

### Plot Risultati
```bash
python3 plot_dijkstra.py
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
