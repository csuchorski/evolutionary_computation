# Laboratory 5 — move evaluations from previous iterations

Select exactly `k = ceil(n/2)` of `n` nodes and build a closed tour through them,
minimising **tour length + sum of the costs of the selected nodes**.

This laboratory speeds up the steepest local search by keeping a **list of
improving moves (LM)** between iterations. Instead of evaluating the whole
neighbourhood in every iteration, the search browses saved move deltas and
applies the best applicable one. The neighbourhood contains **inter-route node
exchanges** and **intra-route 2-opt edge exchanges**.

The implementation lives in `engine/methods/local_search_lab5.{hpp,cpp}` and is
registered under `--lab 5`. The report is `report/lab5_report.ipynb`; generated
artefacts live in `results/` and `report/`.

## Quick start

```bash
./run.sh lab5
```

The shared script creates `.venv` at the repository root on first use, builds
`tsp_solver`, executes the notebook, exports HTML and prints
`lab5/report/report.pdf`. Manual equivalent:

```bash
make
.venv/bin/python tools/render_report.py lab5
```

## Engine

```bash
./tsp_solver --seed 12345 --runs 200 --out lab5/results --data data --lab all
```

The two instances are `data/TSPA.csv` and `data/TSPB.csv`, one `x;y;cost` row
per node. See the root README for all CLI options and output columns. The
candidate-list size of the combined method is configurable with
`--candidates` (default `10`). The candidate lists are rebuilt for every run, so
the reported running time includes their construction.

## List of improving moves

A saved move is either:

- **inter-route** — replace selected node `u` by unselected node `v`; the saved
  descriptor is `(p,u,n,v)`, where `(p,u)` and `(u,n)` are the removed edges;
- **intra-route 2-opt** — remove directed edges `(a,b)` and `(c,d)` and add
  `(a,c)`, `(b,d)`, reversing the segment between the removed edges.

Each entry stores the move descriptor and its delta. The LM is kept ordered from
the best (most negative) delta to the worst. A saved delta is exact for as long
as the move is applicable, so it is applied directly and never re-scored.

### Applicability

For an inter-route move `(p,u,n,v)`:

| Situation | Action |
|---|---|
| `u` is no longer selected, or `v` is already selected | remove the move from LM |
| both `(p,u)` and `(u,n)` still exist, in either direction | apply the saved delta and remove the move |
| at least one removed edge has disappeared | remove the move from LM |

For an edge move `(a,b),(c,d)`, the three applicability cases are used:

| Situation | Action |
|---|---|
| at least one removed edge no longer exists | remove the move from LM |
| both edges exist in a different relative direction | leave it in LM, skip it now |
| both edges exist in the same relative direction, including both reversed | apply it and remove it from LM |

The “different direction” case is kept because a later 2-opt reversal may make
the move applicable again.

### Relative-direction variants

Every 2-opt edge pair is evaluated in **both relative directions**:

```text
variant A: removed (a,b), (c,d)   -> added (a,c), (b,d)
variant B: removed (a,b), (d,c)   -> added (a,d), (b,c)
```

Variant B is stored for future use even if it is not applicable to the current
tour. This is done both when building the initial LM and whenever the LM is
rebuilt.

### Search loop

```text
x = randomTour()
LM = all improving moves of x, sorted best-to-worst
repeat
    browse LM from the best entry
        remove entries whose removed edges vanished
        skip entries whose removed edges have the other relative direction
        apply the first applicable improving entry using its saved delta
    if no entry was applied
        if LM was just rebuilt: stop
        discard LM and rebuild it from the current solution
until no applicable improving move exists and rebuilding adds none
```

When no move in the current LM is applicable, the LM is rebuilt from scratch;
skipped entries are not carried over because the rebuild regenerates every
improving move of the new solution, including both relative directions.

## Methods

| Method | Description | Key |
|---|---|---|
| full neighbourhood | steepest, LM over all moves, random start | `ls_lm_steepest_random` |
| candidate moves | steepest, LM over candidate moves, random start | `ls_lm_cand_random` |
| **baseline** | full steepest local search, random start | `ls_steepest_edges_random` |

All random-start methods share the baseline's RNG stream, so the comparison uses
the same 200 random tours per instance. The combined method uses the candidate
lists in both directions for inter-route moves: a candidate edge stored only in the unselected endpoint's list is also used. This
keeps the combined method close to the candidate-search quality.

## Diagnostics in `runs.csv`

| Column | Meaning |
|---|---|
| `initial_objective` | objective of the starting solution |
| `moves` | accepted improving moves |
| `evaluated` | deltas computed |
| `lm_checks` | LM entries inspected |
| `lm_skipped` | entries skipped because of the other relative direction |
| `lm_removed` | entries discarded because a removed edge vanished or the saved context was no longer valid |
| `regenerations` | full neighbourhood rebuilds |
| `max_lm_size` | largest LM length |

`evaluated` is the direct measure of the speed-up: LM evaluates far fewer deltas
than the full steepest baseline. `regenerations` and `max_lm_size` show how much
work the LM saves between rebuilds.

## Report contents

`report/lab5_report.ipynb` contains: problem statement, data and distance
matrix, the LM representation, the three applicability cases, both relative
directions, delta formulas, pseudocode, the full-neighbourhood and candidate-based LM methods, the
experimental setup, objective and running-time tables for all methods, a
dedicated LM vs full-steepest comparison, search-effort diagnostics, the
LM + candidate comparison, distributions, 2-D visualisations and node lists of
the best solutions, and conclusions.

Generated artefacts:

```text
results/*.csv
report/lab5_report.ipynb     executed notebook (outputs stored)
report/report.html           exported notebook with report CSS
report/report.pdf            A4 PDF report
report/figures/*.png         figures written by the notebook
```
