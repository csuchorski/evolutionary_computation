# Laboratory 1 — Hamiltonian cycle over a selected half of the nodes

Select exactly `k = ceil(n/2)` of `n` nodes and build a closed tour through them,
minimising **tour length + sum of the costs of the selected nodes**.

The project is split in two parts:

- **`src/main.cpp`** — the optimisation engine in C++17. It reads the instances,
  builds the integer distance matrix once, runs all four construction methods and
  writes machine-readable CSV files. It contains no reporting or plotting code.
- **`report/lab1_report.ipynb`** — the report as a Jupyter notebook. It executes
  the engine, then produces the tables, objective distributions, 2-D
  visualisations, best-tour listings and the discussion in Python. The notebook
  is exported to `report/report.pdf`.

## Requirements

- `g++` with C++17 (tested with GCC 15.3)
- Python 3 with `nbconvert`, `ipykernel`, `matplotlib`, `pandas`, `numpy`
  (listed in `requirements.txt`, installed automatically into `.venv` by `run.sh`)
- `google-chrome` (only for printing the notebook export to PDF)

## Quick start

```bash
./run.sh
```

The script creates `.venv` on first use, builds the engine, executes the
notebook, exports HTML with the report stylesheet and prints
`report/report.pdf` with headless Chrome. The export hides code-cell inputs and
keeps only their outputs; the notebook itself still contains all code.

Manual equivalent:

```bash
make
.venv/bin/jupyter nbconvert --to notebook --execute --inplace report/lab1_report.ipynb
.venv/bin/python tools/render_report.py          # HTML + PDF
.venv/bin/jupyter lab                            # edit / browse the notebook
```

## Engine

```bash
./tsp_solver --seed 12345 --runs 200 --out results
```

The two instances are fixed: `TSPA.csv` and `TSPB.csv`, one `x;y;cost` row per node.

| Option | Default | Meaning |
|---|---|---|
| `--out DIR` | `results` | output directory |
| `--seed N` | `12345` | RNG seed for the random method |
| `--runs N` | `200` | number of solutions per method |

### Outputs (`results/`)

| File | Content |
|---|---|
| `summary.csv` | per instance and method: min, max, average, best, first node, time |
| `runs.csv` | every constructed solution: run, start node, objective, distance, costs |
| `best_tours.csv` | best tour per instance and method (0-based node list) |
| `best_tours_columns.csv` | best tours as columns: one header row and 100 node rows, ready for Excel |

## Methods

1. **Random** — random `k`-subset in random order, closed cycle.
2. **NN append at end** — grow an open path by appending the node minimising
   `cost[j] + d[last][j]`; close once at the end.
3. **NN any position** — insert the best node at the front, at the end, or
   between any two consecutive path nodes, using the full objective delta.
4. **Greedy cycle** — keep a closed cycle from the first step; insert the
   `(node, edge)` pair minimising
   `cost[j] + d[a][j] + d[j][b] - d[a][b]`. The wrap edge `last -> first` is an
   ordinary insertion position, so both new arcs are always accounted for.

Each greedy method is run 200 times, one run per starting node; the random
method generates 200 independent solutions. Every solution is re-evaluated by a
single `evaluate()` routine and validated (size, uniqueness, objective
consistency).

## Report contents

`report/lab1_report.ipynb` contains: problem statement, distance-matrix
construction, pseudocode of all four algorithms, experiment setup, the
min/max/average table, box plots of all 200 solutions per method, 2-D
visualisations of the eight best solutions (colour and radius encode node cost),
the best tours as 0-based node lists, observations and reproduction notes.

Generated artefacts:

```
results/*.csv
report/lab1_report.ipynb      executed notebook (outputs stored)
report/report.html            exported notebook with report CSS
report/report.pdf             A4 PDF report
report/figures/*.png          figures written by the notebook
```

## Instances

- `TSPA.csv`, `TSPB.csv` — 200 nodes each, `x;y;cost`.
- Distances: `d(i,j) = llround(hypot(xi-xj, yi-yj))`, computed once after reading.
- Selected nodes: `k = 100`.
