# SSSP Algorithms: Dijkstra & Ramalingam-Reps

Questo progetto implementa e confronta due algoritmi per il problema **Single-Source Shortest Path (SSSP)** su grafi dinamici:

- **Dijkstra (statico)**: complessità *O(m log n)*
- **Ramalingam-Reps (dinamico)**: complessità *O(||δ|| log ||δ||)*, dove ||δ|| rappresenta il numero di nodi e archi "affetti" dall'aggiornamento

Il benchmark include:
1. Verifica empirica della complessità di Dijkstra su grafi sparsi, scale-free (Barabási-Albert) e densi
2. Test della complessità di Ramalingam-Reps con operazioni di aumento/diminuzione peso e inserimento/cancellazione archi
3. Confronto diretto Dijkstra vs Ramalingam-Reps per misurare lo speedup dell'algoritmo dinamico

---

## Requisiti

- **C++17** con CMake ≥ 3.10
- **zlib** (`sudo apt install zlib1g-dev` su Ubuntu/Debian)
- **Python 3** con pacchetti: `numpy`, `pandas`, `matplotlib`, `networkx`

```bash
# Installazione dipendenze Python
pip install numpy pandas matplotlib networkx
```

---

## Build

Tutti i target vengono compilati in modalità **Release** (`-O3 -DNDEBUG`) per massima efficienza.

```bash
cd /home/pierluca/Desktop/Algorithm-Engineering/Dijkstra

# Build di tutti i target
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake --build build -j4
```

### Target disponibili

| Target                 | Descrizione                              |
|------------------------|------------------------------------------|
| `graph_generator`      | Generatore grafi (sparsi, Barabási, densi) |
| `dijkstra_main`        | Benchmark Dijkstra con doubling experiment |
| `test_ramalingam`      | Test Ramalingam-Reps (tutte le operazioni) |
| `scientific_benchmark` | Confronto Dijkstra vs Ramalingam-Reps     |

---

## Workflow Completo

Seguire i passaggi nell'ordine indicato per eseguire l'intera pipeline di benchmark.

---

### 1. Generazione Grafi

Genera tre tipologie di grafi nelle rispettive cartelle:
- `grafi_sparsi/` - Grafi sparsi uniformi (*m = 4n*)
- `grafi_barabasi/` - Grafi scale-free Barabási-Albert
- `grafi_densi/` - Grafi densi (*m ≈ n²/4*)

```bash
./build/graph_generator
```

---

### 2. Benchmark Dijkstra

Esegue il benchmark su ciascuna tipologia di grafo. I risultati CSV vengono salvati nelle cartelle `risultati_*`.

```bash
# Crea le cartelle risultati (se non esistono)
mkdir -p risultati_sparsi risultati_barabasi risultati_densi

# Esegui benchmark
./build/dijkstra_main grafi_sparsi > risultati_sparsi/dijkstra_sparsi.csv
./build/dijkstra_main grafi_barabasi > risultati_barabasi/dijkstra_barabasi.csv
./build/dijkstra_main grafi_densi > risultati_densi/dijkstra_densi.csv
```

> **Nota**: I file delle distanze (`distances_n*.csv`) vengono salvati automaticamente nelle cartelle dei grafi.

---

### 3. Verifica Correttezza Dijkstra

Confronta i risultati C++ con l'implementazione NetworkX di Python.

```bash
./verify_dijkstra.py grafi_sparsi
./verify_dijkstra.py grafi_barabasi
./verify_dijkstra.py grafi_densi
```

> **Nota**: Lo script salta automaticamente grafi > 200MB per evitare crash dovuti a eccessivo consumo di RAM.

---

### 4. Plot Risultati Dijkstra

Genera i grafici di complessità per ciascuna tipologia. I plot vengono salvati nella stessa cartella del CSV.

```bash
python3 plot_dijkstra.py risultati_sparsi/dijkstra_sparsi.csv
python3 plot_dijkstra.py risultati_barabasi/dijkstra_barabasi.csv
python3 plot_dijkstra.py risultati_densi/dijkstra_densi.csv
```

---

### 5. Test Ramalingam-Reps

Esegue il test di complessità su operazioni W+ (aumento peso), W- (diminuzione peso), INS (inserimento) e DEL (cancellazione).

#### Download del grafo

Il test utilizza il grafo stradale **Western USA** dalla [9th DIMACS Implementation Challenge](https://www.diag.uniroma1.it/challenge9/download.shtml).

| Grafo | Nodi | Archi |
|-------|------|-------|
| USA-road-d.W | 6,262,104 | 15,248,146 |

```bash
# Crea la cartella e scarica il grafo
mkdir -p grafi_rr
cd grafi_rr
wget https://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.W.gr.gz
gunzip USA-road-d.W.gr.gz
cd ..
```

#### Esecuzione test

```bash
./build/test_ramalingam
```

**Output generato:**
- `rr_complexity.csv` - Metriche (||δ||, tempo, correttezza)

---

### 6. Plot Risultati Ramalingam-Reps

```bash
python3 plot_ramalingam.py
```

**Output:**
- `plots/rr_complexity.png`

---

### 7. Benchmark Dijkstra vs Ramalingam-Reps

Confronto diretto tra l'algoritmo statico e quello dinamico.

```bash
# Crea la cartella risultati
mkdir -p risultati_vs

# Esegui benchmark
./build/scientific_benchmark ./grafi_vs 0.1 large > risultati_vs/results.csv
```

#### Parametri del benchmark

| Parametro       | Descrizione                                    | Default |
|-----------------|------------------------------------------------|---------|
| `folder_path`   | Cartella con grafi (.txt o .gr)                | -       |
| `update_factor` | Moltiplicatore Update (*K = N × factor*)       | 1.0     |
| `magnitude`     | `small` (±10%), `large` (×2), `mixed`          | large   |
| `spt_mode`      | `spt` (archi SPT) o `random` (archi casuali)   | spt     |
| `depth`         | Profondità SPT: `root`, `middle`, `leaf`, `mixed` | mixed |
| `update_type`   | Tipo update: `increase`, `decrease`, `mixed`   | mixed   |

#### Esempi avanzati

```bash
# Solo archi vicino alla radice SPT (worst-case)
./build/scientific_benchmark ./grafi_vs 0.5 large spt root increase > spt_root_inc.csv

# Solo archi vicino alle foglie SPT
./build/scientific_benchmark ./grafi_vs 0.5 large spt leaf decrease > spt_leaf_dec.csv

# Modalità random (comportamento casuale)
./build/scientific_benchmark ./grafi_vs 0.5 large random > results_random.csv
```

---

### 8. Plot Risultati Confronto

```bash
python3 plot_results.py risultati_vs/results.csv
```

---

## Riepilogo Rapido

```bash
# ===== BUILD =====
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake --build build -j4

# ===== DIJKSTRA =====
./build/graph_generator
mkdir -p risultati_sparsi risultati_barabasi risultati_densi

./build/dijkstra_main grafi_sparsi > risultati_sparsi/dijkstra_sparsi.csv
./build/dijkstra_main grafi_barabasi > risultati_barabasi/dijkstra_barabasi.csv
./build/dijkstra_main grafi_densi > risultati_densi/dijkstra_densi.csv

./verify_dijkstra.py grafi_sparsi
./verify_dijkstra.py grafi_barabasi
./verify_dijkstra.py grafi_densi

python3 plot_dijkstra.py risultati_sparsi/dijkstra_sparsi.csv
python3 plot_dijkstra.py risultati_barabasi/dijkstra_barabasi.csv
python3 plot_dijkstra.py risultati_densi/dijkstra_densi.csv

# ===== RAMALINGAM-REPS =====
./build/test_ramalingam
python3 plot_ramalingam.py

# ===== DIJKSTRA VS RAMALINGAM-REPS =====
mkdir -p risultati_vs
./build/scientific_benchmark ./grafi_vs 0.1 large > risultati_vs/results.csv
python3 plot_results.py risultati_vs/results.csv
```

---

## Struttura del Progetto

```
Dijkstra/
├── CMakeLists.txt              # Build configuration
├── README.md                   # Questa guida
│
├── Graph.cpp/.h                # Struttura dati grafo
├── DijkstraSolver.cpp/.h       # Implementazione Dijkstra
├── RamalingamReps.cpp/.h       # Implementazione Ramalingam-Reps
├── BenchmarkStats.h            # Contatori per statistiche
│
├── graph_generator.cpp         # Generatore grafi
├── main.cpp                    # Driver benchmark Dijkstra
├── test_ramalingam.cpp         # Driver test Ramalingam-Reps
├── BenchmarkDriver.cpp         # Driver confronto Dijkstra vs RR
│
├── plot_dijkstra.py            # Visualizzazione risultati Dijkstra
├── plot_ramalingam.py          # Visualizzazione risultati RR
├── plot_results.py             # Visualizzazione confronto
├── verify_dijkstra.py          # Verifica correttezza
│
├── grafi_sparsi/               # Grafi sparsi generati
├── grafi_barabasi/             # Grafi scale-free generati
├── grafi_densi/                # Grafi densi generati
├── grafi_vs/                   # Grafi per benchmark confronto
│
├── risultati_sparsi/           # Risultati benchmark sparsi
├── risultati_barabasi/         # Risultati benchmark Barabási
├── risultati_densi/            # Risultati benchmark densi
├── risultati_vs/               # Risultati confronto
│
└── plots/                      # Grafici generati
```
