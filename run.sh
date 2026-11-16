#!/usr/bin/env bash
# Build the shared engine, execute a report notebook, and print it to PDF.
# Usage: ./run.sh [lab1|lab2|lab3|lab4]     (default: lab3)
set -euo pipefail
cd "$(dirname "$0")"

LAB="${1:-lab3}"
case "$LAB" in
    1|lab1) LAB=lab1 ;;
    2|lab2) LAB=lab2 ;;
    3|lab3) LAB=lab3 ;;
    4|lab4) LAB=lab4 ;;
    *) echo "usage: $0 [lab1|lab2|lab3|lab4]" >&2; exit 2 ;;
esac

if [ ! -x .venv/bin/jupyter ]; then
    echo "Creating .venv and installing notebook dependencies (once) ..."
    python3 -m venv .venv
    .venv/bin/pip install --upgrade pip -q
    .venv/bin/pip install -q -r requirements.txt
fi

make

.venv/bin/python tools/render_report.py "$LAB"
