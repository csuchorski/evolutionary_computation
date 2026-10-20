#!/usr/bin/env python3
"""Execute the report notebook, export styled HTML and print it to PDF.

Usage:  .venv/bin/python tools/render_report.py

The notebook itself builds the C++ engine, runs both instances and writes the
figures; this script only drives nbconvert and headless Chrome.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NB = ROOT / "report" / "lab1_report.ipynb"
HTML = ROOT / "report" / "report.html"
PDF = ROOT / "report" / "report.pdf"
CSS = ROOT / "report" / "report.css"


def run(cmd: list[object], **kwargs) -> None:
    print("+ " + " ".join(str(c) for c in cmd), flush=True)
    subprocess.run([str(c) for c in cmd], check=True, **kwargs)


def main() -> int:
    if not NB.exists():
        print(f"notebook not found: {NB}", file=sys.stderr)
        return 1

    jupyter = ROOT / ".venv" / "bin" / "jupyter"
    if not jupyter.exists():
        found = shutil.which("jupyter")
        if found is None:
            print("jupyter not found; create .venv and install nbconvert", file=sys.stderr)
            return 1
        jupyter = Path(found)

    # 1. execute the notebook and store its outputs in the .ipynb itself
    run([jupyter, "nbconvert", "--to", "notebook", "--execute", "--inplace",
         NB, "--ExecutePreprocessor.timeout=900"], cwd=ROOT)

    # 2. export HTML
    run([jupyter, "nbconvert", "--to", "html", NB,
         "--output", "report.html", "--output-dir", ROOT / "report"], cwd=ROOT)

    # 3. inject the print stylesheet (kept outside the notebook)
    html = HTML.read_text(encoding="utf-8")
    if "</head>" not in html:
        print("unexpected HTML export: no </head>", file=sys.stderr)
        return 1
    css = CSS.read_text(encoding="utf-8")
    html = html.replace("</head>", f"<style>\n{css}\n</style>\n</head>", 1)
    HTML.write_text(html, encoding="utf-8")

    # 4. print to PDF with headless Chrome (vector figures, A4)
    chrome = (shutil.which("google-chrome") or shutil.which("chromium")
              or shutil.which("chromium-browser"))
    if chrome is None:
        print("Chrome/Chromium not found; open report/report.html and print it",
              file=sys.stderr)
        return 1
    run([chrome, "--headless=new", "--no-sandbox", "--disable-gpu",
         "--disable-dev-shm-usage", "--no-pdf-header-footer",
         "--virtual-time-budget=20000",
         f"--print-to-pdf={PDF}", HTML.as_uri()])

    size = PDF.stat().st_size / 1e6 if PDF.exists() else 0.0
    print(f"\nHTML: {HTML}\nPDF:  {PDF} ({size:.1f} MB)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
