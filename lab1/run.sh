#!/usr/bin/env bash
# Build the C++ engine, execute the report notebook, and print it to PDF.
set -euo pipefail
cd "$(dirname "$0")"

if [ ! -x .venv/bin/jupyter ]; then
    echo "Creating .venv and installing notebook dependencies (once) ..."
    python3 -m venv .venv
    .venv/bin/pip install --upgrade pip -q
    .venv/bin/pip install -q -r requirements.txt
fi

make

.venv/bin/python tools/render_report.py
