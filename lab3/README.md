# Laboratory 3 — local search for the Hamiltonian cycle

Select exactly `k = ceil(n/2)` of `n` nodes and build a closed tour through them,
minimising **tour length + sum of the costs of the selected nodes**.

This laboratory uses the shared engine described in the root
[`README.md`](../README.md). The local-search implementation lives in
`engine/methods/local_search_lab3.{hpp,cpp}` and is registered under `--lab 3`.
The heuristic methods of Laboratories 1 and 2 remain available and are
included in the report tables. The report is `report/lab3_report.ipynb`; the
generated artefacts live in `results/` and `report/`.

## Quick start

```bash
./run.sh lab3
```

The shared script creates `.venv` at the repository root on first use, builds
`tsp_solver`, executes the notebook, exports HTML and prints
`lab3/report/report.pdf`. Manual equivalent:

```bash
make
.venv/bin/python tools/render_report.py lab3
```

## Engine

```bash
./tsp_solver --seed 12345 --runs 200 --out lab3/results --data data --lab 3
```

`--lab all` runs all 16 methods and is what the report uses. The two instances
are `data/TSPA.csv` and `data/TSPB.csv`, one `x;y;cost` row per node. See the
root README for all CLI options and output columns.

## Local search

The search keeps the current tour and its objective value and evaluates every
candidate move with an **O(1) objective delta**. No neighbour tour is constructed
explicitly and the objective is never recomputed from scratch during the search.
The final tour is re-evaluated and validated once. No deltas from previous
iterations and no candidate-list restriction of the neighbourhood are used; the
greedy variant's temporary descriptor list always contains the whole mixed
neighbourhood and is rebuilt after every accepted move.

### Neighbourhood

- **Inter-route move** — exchange one selected node with one unselected node.
  Always present in the neighbourhood.
- **Intra-route move A** — exchange two selected nodes (positions).
- **Intra-route move B** — exchange two cycle edges (2-opt), reversing the
  segment between them.

The intra-route type is a binary option; the total neighbourhood is the mixture
of inter- and intra-route moves.

### Deltas

For an inter-route exchange of selected `u = t[i]` with unselected `v`, with
`prev = t[i-1]` and `next = t[i+1]`:

```text
Δ = cost[v] - cost[u]
    + d[prev][v] + d[v][next]
    - d[prev][u] - d[u][next].
```

A two-node exchange leaves the selected set unchanged, so the node costs cancel.
Only the at most four arcs starting at `{i-1, i, j-1, j}` change; the set is
deduplicated, so adjacent swaps are handled exactly.

A two-edge exchange of edges `(a,b)` and `(c,e)` uses the symmetric 2-opt delta

```text
Δ = d[a][c] + d[b][e] - d[a][b] - d[c][e],
```

and reverses the segment between the two edges.

### Steepest and greedy

- **Steepest** scans the whole mixed neighbourhood every iteration and applies
  the single best improving move. It stops when no negative delta exists.
- **Greedy** builds a descriptor list of the whole mixed neighbourhood, shuffles
  it with `std::shuffle` and a `std::mt19937` stream, then evaluates deltas one
  by one and applies the first improving move. The list contains no deltas, so
  the method does not "evaluate everything and then choose". After an accepted
  move the neighbourhood is rebuilt and reshuffled.

The randomization is a fresh uniform permutation of the combined inter- and
intra-route neighbourhood, so the two move kinds are interleaved and neither is
always tested first.

### The eight methods

| Search | Intra move | Starting solution | Key |
|---|---|---|---|
| steepest | node exchange | random | `ls_steepest_nodes_random` |
| steepest | edge exchange | random | `ls_steepest_edges_random` |
| steepest | node exchange | greedy cycle | `ls_steepest_nodes_greedy` |
| steepest | edge exchange | greedy cycle | `ls_steepest_edges_greedy` |
| greedy | node exchange | random | `ls_greedy_nodes_random` |
| greedy | edge exchange | random | `ls_greedy_edges_random` |
| greedy | node exchange | greedy cycle | `ls_greedy_nodes_greedy` |
| greedy | edge exchange | greedy cycle | `ls_greedy_edges_greedy` |

Random starts are 200 independent random tours. Greedy starts use the
Laboratory-1 greedy cycle with run *r* starting from node *r*, covering every
node once. The engine gives each method its own RNG stream derived from the seed
and the method key.

## Diagnostics in `runs.csv`

For local-search methods the shared output adds:

| Column | Meaning |
|---|---|
| `initial_objective` | objective of the starting solution |
| `moves` | accepted improving moves |
| `evaluated` | candidate deltas computed |

Heuristic methods report `initial_objective = objective`, `moves = 0` and
`evaluated = 0`.

## Report contents

`report/lab3_report.ipynb` contains: problem statement, data and distance
matrix, reference heuristics, neighbourhood and move-evaluation formulas,
steepest and greedy pseudocode, the randomization description, starting
solutions and the eight variants, experimental setup, the objective table in
`average (min – max)` form for all 16 methods, the running-time table,
distributions, an equal-time random-restart baseline, 2-D visualisations and
node lists of the best solutions, and conclusions.

Generated artefacts:

```text
results/*.csv
report/lab3_report.ipynb      executed notebook (outputs stored)
report/report.html            exported notebook with report CSS
report/report.pdf             A4 PDF report
report/figures/*.png          figures written by the notebook
```
