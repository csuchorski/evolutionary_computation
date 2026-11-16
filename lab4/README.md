# Laboratory 4 — candidate moves in local search

Select exactly `k = ceil(n/2)` of `n` nodes and build a closed tour through them,
minimising **tour length + sum of the costs of the selected nodes**.

This laboratory uses the shared engine described in the root
[`README.md`](../README.md). The candidate-move local search lives in
`engine/methods/local_search_lab4.{hpp,cpp}` and is registered under `--lab 4`.
The methods of Laboratories 1–3 remain available and are included in the report
tables. The report is `report/lab4_report.ipynb`; the generated artefacts live in
`results/` and `report/`.

## Quick start

```bash
./run.sh lab4
```

The shared script creates `.venv` at the repository root on first use, builds
`tsp_solver`, executes the notebook, exports HTML and prints
`lab4/report/report.pdf`. Manual equivalent:

```bash
make
.venv/bin/python tools/render_report.py lab4
```

## Engine

```bash
./tsp_solver --seed 12345 --runs 200 --out lab4/results --data data --lab all
```

The two instances are `data/TSPA.csv` and `data/TSPB.csv`, one `x;y;cost` row
per node. See the root README for all CLI options and output columns. The
candidate-list size is configurable with `--candidates` (default `10`).

## Candidate moves

The task is to speed up the **steepest** local search from Laboratory 3 without
changing its neighbourhood, which was the **2-opt edge exchange** (plus the
always-present inter-route node exchange). Instead of scanning the whole
neighbourhood, only **candidate moves** are examined — moves that introduce at
least one candidate edge.

Candidate edges are built once per instance, directly from the distance matrix,
and **separately for the two move families**:

- **inter-route list** — the `count` nearest vertices under
  `d(i, j) + cost[j]`, because an inter-route exchange changes the selected set;
- **intra-route list** — the `count` nearest vertices under `d(i, j)` only,
  because an intra-route move keeps the selected set and the node costs cancel.

Both lists are directed: `(i, j)` is a candidate edge when `j` is in `i`'s list.

### Loops over candidate moves only

The scan is organised so that it walks candidate edges directly — no full
neighbourhood is ever built:

```text
for every selected node a in the tour:
    for every j in a's list:
        if j is selected:        # intra-route
            two 2-opt moves adding the edge (a, j)
        else:                    # inter-route
            two node exchanges adding the edge (a, j):
            swap next(a) with j, or swap prev(a) with j
```

For a selected anchor `a` the candidate edge `(a, j)` is always the edge that the
move introduces: the intra 2-opt removes the edges after `a`/`j` or the edges
before `a`/`j`, and the inter exchange removes one of `a`'s two tour neighbours
(so `a` stays and `j` becomes adjacent to it). This yields the two moves that
introduce a given candidate edge for each family. Exchanging the anchor `a`
itself with `j` would **not** introduce `(a, j)` and is therefore not used.

Reciprocal candidate edges and inter exchanges reachable from two anchors would
otherwise produce the same move twice; each move is evaluated only once (the
loop keeps the first occurrence in iteration order), so the evaluated set is
exactly the set of candidate moves.

The cost of a full iteration drops from `O(k·(n−k) + k²)` to
`O(k · count)`, so the running time is much shorter while the solution quality
degrades only slightly. The default `count = 10` can be tuned with
`--candidates`; the report includes a small sweep.

### Method and baseline

| Method | Key |
|---|---|
| steepest local search restricted to candidate moves, random start | `ls_cand_steepest_random` |
| **baseline**: full steepest local search (no candidate mechanism), random start | `ls_steepest_edges_random` |

Both use the same neighbourhood and 200 random starting solutions. The candidate
method shares the **baseline's RNG stream**, so the two start from the same 200
random tours and the comparison isolates the effect of the candidate
restriction. All objective deltas are `O(1)` and the final tour is re-evaluated
and validated once; the search never recomputes the objective from scratch.

## Diagnostics in `runs.csv`

For the candidate method the shared output reports `initial_objective`,
`moves` (accepted improving moves) and `evaluated` (candidate deltas computed).
`evaluated` is the direct measure of the candidate-list speed-up: the
candidate search evaluates roughly an order of magnitude fewer deltas than the
full baseline.

## Report contents

`report/lab4_report.ipynb` contains: problem statement, data and distance
matrix, the candidate-edge definitions and loop organisation, the steepest
candidate pseudocode, experimental setup, the objective and running-time tables
in `average (min – max)` form for all methods, a dedicated candidate-move
baseline comparison, the candidate-size sweep, distributions, 2-D
visualisations and node lists of the best solutions, and conclusions.

Generated artefacts:

```text
results/*.csv
results/sweep/c<count>/summary.csv   candidate-size sweep runs
report/lab4_report.ipynb             executed notebook (outputs stored)
report/report.html                   exported notebook with report CSS
report/report.pdf                    A4 PDF report
report/figures/*.png                 figures written by the notebook
```
