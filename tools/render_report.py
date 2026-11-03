#!/usr/bin/env python3
"""Execute a report notebook, export styled HTML and print it to PDF.

Usage:  .venv/bin/python tools/render_report.py [lab1|lab2|lab3]

The notebook itself builds the C++ engine, runs the solver and writes the
figures; this script only drives nbconvert and headless Chrome.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CSS = ROOT / "report" / "report.css"


def run(cmd: list[object], **kwargs) -> None:
    print("+ " + " ".join(str(c) for c in cmd), flush=True)
    subprocess.run([str(c) for c in cmd], check=True, **kwargs)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("lab", nargs="?", default="lab3",
                        choices=["lab1", "lab2", "lab3"])
    args = parser.parse_args()

    lab_dir = ROOT / args.lab
    notebook = lab_dir / "report" / f"{args.lab}_report.ipynb"
    html = lab_dir / "report" / "report.html"
    pdf = lab_dir / "report" / "report.pdf"

    if not notebook.exists():
        print(f"notebook not found: {notebook}", file=sys.stderr)
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
         notebook, "--ExecutePreprocessor.timeout=1800"], cwd=ROOT)

    # 2. export HTML into the lab's report directory
    run([jupyter, "nbconvert", "--to", "html", notebook,
         "--output", "report.html", "--output-dir", lab_dir / "report"], cwd=ROOT)

    # 3. inject the shared print stylesheet
    html_text = html.read_text(encoding="utf-8")
    if "</head>" not in html_text:
        print("unexpected HTML export: no </head>", file=sys.stderr)
        return 1
    css = CSS.read_text(encoding="utf-8")
    html_text = html_text.replace("</head>", f"<style>\n{css}\n</style>\n</head>", 1)
    html.write_text(html_text, encoding="utf-8")

    # 4. print to PDF with headless Chrome (vector figures, A4)
    chrome = (shutil.which("google-chrome") or shutil.which("google-chrome-stable")
              or shutil.which("chromium") or shutil.which("chromium-browser"))
    if chrome is None:
        print(
            "HTML was generated, but no Chrome/Chromium was found for the PDF.\n"
            "Install one of:\n"
            "  sudo snap install chromium\n"
            "  sudo apt install ./google-chrome-stable_current_amd64.deb\n"
            f"or open {html} in a browser and print it to PDF manually.",
            file=sys.stderr)
        return 1
    run([chrome, "--headless=new", "--no-sandbox", "--disable-gpu",
         "--disable-dev-shm-usage", "--no-pdf-header-footer",
         "--virtual-time-budget=20000",
         f"--print-to-pdf={pdf}", html.as_uri()])

    size = pdf.stat().st_size / 1e6 if pdf.exists() else 0.0
    print(f"\nHTML: {html}\nPDF:  {pdf} ({size:.1f} MB)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
