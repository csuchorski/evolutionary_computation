# Laboratory 2 — Hamiltonian cycle over a selected half of the nodes

Select exactly `k = ceil(n/2)` of `n` nodes and build a closed tour through them,
minimising **tour length + sum of the costs of the selected nodes**.

The project is split in two parts:

- **`src/`** — the optimisation engine in C++17, split by responsibility and
  assignment:

  ```
  src/
    main.cpp            CLI, method registry, CSV output
    core/common.{hpp,cpp}   instance reading, distance matrix, objective, validation
    lab1/lab1.{hpp,cpp}     Laboratory-1 methods
    lab2/lab2.{hpp,cpp}     Laboratory-2 regret methods
  ```

  Each lab registers its methods so `--lab` selects which assignment runs; the
  default runs all of them, so the report keeps the previous algorithms. It
  contains no reporting or plotting code.
- **`report/lab2_report.ipynb`** — the report as a Jupyter notebook. It executes
  the engine, then produces the tables, objective distributions, 2-D
  visualisations, best-tour listings, the weight sweep and the discussion in
  Python. The notebook is exported to `report/report.pdf`. The Laboratory-1
  notebook (`report/lab1_report.ipynb`) is kept as the previous report.

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
.venv/bin/jupyter nbconvert --to notebook --execute --inplace report/lab2_report.ipynb
.venv/bin/python tools/render_report.py          # HTML + PDF
.venv/bin/jupyter lab                            # edit / browse the notebook
```

## Engine

```bash
./tsp_solver --seed 12345 --runs 200 --out results --lab all
```

`--lab all` (the default) runs the methods of both assignments; `--lab 1` or
`--lab 2` restrict the run to a single assignment.

The two instances are fixed: `TSPA.csv` and `TSPB.csv`, one `x;y;cost` row per node.

| Option | Default | Meaning |
|---|---|---|
| `--out DIR` | `results` | output directory |
| `--seed N` | `12345` | RNG seed for the random method |
| `--runs N` | `200` | number of solutions per method |
| `--lab L` | `all` | which assignment's methods to run: `1`, `2`, `all` or `1,2` |
| `--wr W` | `0.5` | regret weight of the weighted-sum criterion |
| `--wb W` | `0.5` | best-change weight of the weighted-sum criterion |

### Outputs (`results/`)

| File | Content |
|---|---|
| `summary.csv` | per instance and method: min, max, average, best, first node, time |
| `runs.csv` | every constructed solution: run, start node, objective, distance, costs |
| `best_tours.csv` | best tour per instance and method (0-based node list) |
| `best_tours_columns.csv` | best tours as columns: one header row and 100 node rows, ready for Excel |

## Methods

Laboratory 1:

1. **Random** — random `k`-subset in random order, closed cycle.
2. **NN append at end** — grow an open path by appending the node minimising
   `cost[j] + d[last][j]`; close once at the end.
3. **NN any position** — insert the best node at the front, at the end, or
   between any two consecutive path nodes, using the full objective delta.
4. **Greedy cycle** — keep a closed cycle from the first step; insert the
   `(node, edge)` pair minimising
   `cost[j] + d[a][j] + d[j][b] - d[a][b]`. The wrap edge `last -> first` is an
   ordinary insertion position, so both new arcs are always accounted for.

Laboratory 2 (each runs on both the open-path and the closed-cycle base):

5. **2-regret** — for every unused node compute the two cheapest insertion
   deltas; insert the node with the largest gap `second - best` at its best
   position.
6. **Weighted sum** — maximise `wr * regret_norm + wb * (1 - best_norm)`, where
   both terms are min-max normalised across the candidate nodes each step. Equal
   weights by default (`--wr`/`--wb`); `wb = 1` reduces exactly to method 3
   (open path) or method 4 (closed cycle).

Each deterministic method is run 200 times, one run per starting node; the random
method generates 200 independent solutions. Every solution is re-evaluated by a
single `evaluate()` routine and validated (size, uniqueness, objective
consistency).

## Report contents

`report/lab2_report.ipynb` contains: problem statement, distance-matrix
construction, pseudocode of all eight methods, experiment setup, the
min/max/average table, box plots of all 200 solutions per method, 2-D
visualisations of the sixteen best solutions (colour and radius encode node
cost), the best tours as 0-based node lists, the weight sweep, observations and
reproduction notes.

Generated artefacts:

```
results/*.csv
results/sweep/*/summary.csv   weight-sweep runs (wr/wb grid)
report/lab2_report.ipynb      executed notebook (outputs stored)
report/lab1_report.ipynb      Laboratory-1 report
report/report.html            exported notebook with report CSS
report/report.pdf             A4 PDF report
report/figures/*.png          figures written by the notebook
```

## Instances

- `TSPA.csv`, `TSPB.csv` — 200 nodes each, `x;y;cost`.
- Distances: `d(i,j) = llround(hypot(xi-xj, yi-yj))`, computed once after reading.
- Selected nodes: `k = 100`.
