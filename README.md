# Evolutionary Computation — laboratories 1–3

This repository contains the three assignments of the Evolutionary Computation
course. All laboratories solve the same problem: select exactly `k = ceil(n/2)`
of `n` nodes and build a closed tour through them, minimising

```text
sum of the costs of the selected nodes + length of the closed tour.
```

## Shared layout

The optimisation engine, the instances and the report tooling are shared. A lab
directory contains only its report, its results and a short README.

```text
EC/
  data/
    TSPA.csv, TSPB.csv         200 nodes each, x;y;cost

  engine/
    main.cpp                   CLI, method registry, experiment driver
    core/
      instance.{hpp,cpp}       instance reading and distance matrix
      objective.{hpp,cpp}      objective evaluation and validation
      method.hpp               Method / BuildContext / per-run statistics
      driver.{hpp,cpp}         run loop, timing, min/max/average
      output.{hpp,cpp}         CSV serialisation
      cli.{hpp,cpp}            command-line options
    methods/
      heuristics_lab1.{hpp,cpp}     random, NN append, NN any position, greedy cycle
      regret_lab2.{hpp,cpp}         2-regret and weighted-sum heuristics
      local_search_lab3.{hpp,cpp}   steepest/greedy local search

  tools/
    render_report.py           execute a lab notebook, export HTML and PDF
    report_common.py           shared notebook helpers and method labels

  report/report.css            shared print stylesheet
  requirements.txt
  Makefile
  run.sh                       ./run.sh lab1 | lab2 | lab3

  lab1/  report/, results/, README.md, run.sh
  lab2/  report/, results/, README.md, run.sh
  lab3/  report/, results/, README.md, run.sh
```

## Requirements

- `g++` with C++17 (tested with GCC 15.3)
- Python 3 with `nbconvert`, `ipykernel`, `matplotlib`, `pandas`, `numpy`,
  `jupyterlab` (see `requirements.txt`)
- `google-chrome` or `chromium` for printing a report to PDF

## Quick start

```bash
./run.sh lab3
```

The script creates `.venv` on first use, builds the shared engine, executes the
selected notebook, injects `report/report.css` into the HTML export and prints
the PDF with headless Chrome. The export hides code-cell inputs and keeps their
outputs; the notebook itself still contains all code.

Manual equivalent:

```bash
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
make
.venv/bin/python tools/render_report.py lab3
```

## Engine

```bash
./tsp_solver --seed 12345 --runs 200 --out lab3/results --lab all
```

| Option | Default | Meaning |
|---|---|---|
| `--out DIR` | `results` | output directory |
| `--data DIR` | `data` | directory with `TSPA.csv` and `TSPB.csv` |
| `--seed N` | `12345` | base RNG seed |
| `--runs N` | `200` | number of solutions per method |
| `--lab L` | `all` | `1`, `2`, `3`, `all` or a comma-separated list |
| `--wr W` | `0.5` | regret weight of the Lab-2 weighted-sum criterion |
| `--wb W` | `0.5` | best-change weight of the Lab-2 weighted-sum criterion |

Every method gets its own RNG stream derived from `--seed` and its key, so
`--lab 3` and `--lab all` produce the same Lab-3 results.

### Outputs

| File | Content |
|---|---|
| `summary.csv` | per instance and method: min, max, average, best, first node, total time |
| `runs.csv` | every solution: run, start node, objective, distance, costs, initial objective, accepted moves, evaluated deltas |
| `best_tours.csv` | best tour per instance and method (0-based node list) |
| `best_tours_columns.csv` | best tours as columns, one header row and 100 node rows |

Heuristic methods report `initial_objective = objective`, `moves = 0` and
`evaluated = 0`; local-search methods fill those diagnostics with the real
search effort.

## Laboratories

- **Laboratory 1** — four heuristic methods (`random`, `nn_end`, `nn_all`,
  `greedy_cycle`). See `lab1/README.md`.
- **Laboratory 2** — 2-regret and weighted-sum heuristics, each on the
  open-path and closed-cycle base. See `lab2/README.md`.
- **Laboratory 3** — steepest and greedy local search with inter-route node
  exchanges and intra-route node/edge exchanges. See `lab3/README.md`.

## Instances

- `data/TSPA.csv`, `data/TSPB.csv` — 200 nodes each, one `x;y;cost` row per node.
- Distances: `d(i,j) = llround(hypot(xi-xj, yi-yj))`, computed once after reading.
- Selected nodes: `k = 100`.
