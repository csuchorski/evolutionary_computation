# Laboratory 2 — Hamiltonian cycle over a selected half of the nodes

Select exactly `k = ceil(n/2)` of `n` nodes and build a closed tour through them,
minimising **tour length + sum of the costs of the selected nodes**.

This laboratory uses the shared engine described in the root
[`README.md`](../README.md). The Lab-2 methods are implemented in
`engine/methods/regret_lab2.{hpp,cpp}` and registered under `--lab 2`; the
Laboratory-1 methods remain available under `--lab 1`. The report is
`report/lab2_report.ipynb` and the generated artefacts live in `results/` and
`report/`.

## Quick start

```bash
./run.sh lab2
```

The shared script creates `.venv` at the repository root on first use, builds
`tsp_solver`, executes the notebook, exports HTML and prints
`lab2/report/report.pdf`. Manual equivalent:

```bash
make
.venv/bin/python tools/render_report.py lab2
```

## Engine

```bash
./tsp_solver --seed 12345 --runs 200 --out lab2/results --data data --lab 1,2
```

The two instances are `data/TSPA.csv` and `data/TSPB.csv`, one `x;y;cost` row
per node. See the root README for all CLI options and output columns.

## Methods

Laboratory 1 (`--lab 1`):

1. **Random** — random `k`-subset in random order, closed cycle.
2. **NN append at end** — grow an open path by appending the node minimising
   `cost[j] + d[last][j]`; close once at the end.
3. **NN any position** — insert the best node at the front, at the end, or
   between any two consecutive path nodes, using the full objective delta.
4. **Greedy cycle** — keep a closed cycle from the first step; insert the
   `(node, edge)` pair minimising
   `cost[j] + d[a][j] + d[j][b] - d[a][b]`. The wrap edge `last -> first` is an
   ordinary insertion position, so both new arcs are always accounted for.

Laboratory 2 (`--lab 2`, each on the open-path and closed-cycle base):

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
heuristic methods, pseudocode of all eight methods, experiment setup, the
min/max/average table, box plots of all 200 solutions per method, 2-D
visualisations of the sixteen best solutions (colour and radius encode node
cost), the best tours as 0-based node lists, the weight sweep, observations and
reproduction notes.

Generated artefacts:

```text
results/*.csv
results/sweep/*/summary.csv   weight-sweep runs (wr/wb grid)
report/lab2_report.ipynb      executed notebook (outputs stored)
report/report.html            exported notebook with report CSS
report/report.pdf             A4 PDF report
report/figures/*.png          figures written by the notebook
```
