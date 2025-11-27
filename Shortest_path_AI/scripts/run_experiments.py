#!/usr/bin/env python3
"""Collect performance metrics and generate figures for Impassable Gate solver."""

from __future__ import annotations

import csv
import math
import re
import subprocess
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "gate"
PUZZLE_DIR = ROOT / "test_puzzles"
OUTPUT_DIR = ROOT / "experiments"
OUTPUT_DIR.mkdir(exist_ok=True, parents=True)

EXCLUDED_PUZZLES = {"impassable3"}

ALGORITHMS = {
    1: "Algorithm 1",
    2: "Algorithm 2",
    3: "Algorithm 3",
}

METRIC_REGEX = {
    "execution_time": re.compile(r"Execution time:\s*([0-9.]+)"),
    "expanded_nodes": re.compile(r"Expanded nodes:\s*([0-9]+)"),
    "generated_nodes": re.compile(r"Generated nodes:\s*([0-9]+)"),
    "duplicated_nodes": re.compile(r"Duplicated nodes:\s*([0-9]+)"),
    "aux_memory": re.compile(r"Auxiliary memory usage \(bytes\):\s*([0-9]+)"),
    "num_pieces": re.compile(r"Number of pieces in the puzzle:\s*([0-9]+)"),
    "steps": re.compile(r"Number of steps in solution:\s*([0-9]+)"),
    "empty_spaces": re.compile(r"Number of empty spaces:\s*([0-9]+)"),
}

SOLVED_REGEX = re.compile(r"Solved by\s+(.*)")
WIDTH_REGEX = re.compile(r"IW\((\d+)\)")


@dataclass
class RunResult:
    puzzle: str
    algorithm_id: int
    algorithm_name: str
    execution_time: float
    expanded_nodes: int
    generated_nodes: int
    duplicated_nodes: int
    aux_memory: int
    num_pieces: int
    steps: int
    empty_spaces: int
    solving_width: int
    solved_label: str

    def to_dict(self) -> Dict[str, object]:
        data = asdict(self)
        data["algorithm"] = data.pop("algorithm_name")
        return data


def run_solver(puzzle: Path, algorithm_id: int) -> RunResult | None:
    cmd = [str(BIN), "-s", str(puzzle), str(algorithm_id)]
    completed = subprocess.run(cmd, capture_output=True, text=True)
    stdout = completed.stdout
    metrics: Dict[str, int | float] = {
        "execution_time": math.nan,
        "expanded_nodes": 0,
        "generated_nodes": 0,
        "duplicated_nodes": 0,
        "aux_memory": 0,
        "num_pieces": 0,
        "steps": 0,
        "empty_spaces": 0,
    }

    for key, pattern in METRIC_REGEX.items():
        match = pattern.search(stdout)
        if match:
            value = match.group(1)
            metrics[key] = float(value) if key == "execution_time" else int(value)

    solved_label = ""
    solved_match = SOLVED_REGEX.search(stdout)
    if solved_match:
        solved_label = solved_match.group(1).strip()

    if "no solution" in solved_label.lower() or completed.returncode != 0:
        return None

    width_match = WIDTH_REGEX.search(solved_label)
    solving_width = int(width_match.group(1)) if width_match else 0

    return RunResult(
        puzzle=puzzle.name,
        algorithm_id=algorithm_id,
        algorithm_name=ALGORITHMS[algorithm_id],
        execution_time=float(metrics["execution_time"]),
        expanded_nodes=int(metrics["expanded_nodes"]),
        generated_nodes=int(metrics["generated_nodes"]),
        duplicated_nodes=int(metrics["duplicated_nodes"]),
        aux_memory=int(metrics["aux_memory"]),
        num_pieces=int(metrics["num_pieces"]),
        steps=int(metrics["steps"]),
        empty_spaces=int(metrics["empty_spaces"]),
        solving_width=solving_width,
        solved_label=solved_label,
    )


def gather_results() -> List[RunResult]:
    puzzles = [
        p
        for p in sorted(PUZZLE_DIR.iterdir())
        if p.is_file() and p.stem not in EXCLUDED_PUZZLES and p.name not in EXCLUDED_PUZZLES
    ]
    runs: List[RunResult] = []
    for puzzle in puzzles:
        for algorithm_id in ALGORITHMS:
            result = run_solver(puzzle, algorithm_id)
            if result is not None:
                runs.append(result)
    if not runs:
        raise RuntimeError("No successful runs were collected.")
    return runs


def comb(n: int, k: int) -> int:
    if k < 0 or k > n:
        return 0
    return math.comb(n, k)


def log10(value: float) -> float | None:
    if value and value > 0:
        return math.log10(value)
    return None


def compute_theoretical_metrics(runs: List[RunResult]) -> List[Dict[str, object]]:
    enriched: List[Dict[str, object]] = []
    for run in runs:
        data = run.to_dict()
        steps = max(run.steps, 1)
        num_pieces = max(run.num_pieces, 1)
        empty_spaces = max(run.empty_spaces, 1)
        width = run.solving_width if run.solving_width > 0 else num_pieces
        width = min(width, num_pieces)

        if run.algorithm_id == 1:
            choose_val = comb(num_pieces, width)
            theoretical_time = choose_val * steps
            theoretical_space = max(choose_val, 1)
        elif run.algorithm_id == 2:
            branching = max(num_pieces + empty_spaces, 1)
            theoretical_time = branching ** min(steps, 12)
            theoretical_space = branching * steps
        else:
            cumulative = sum(comb(num_pieces, k) for k in range(1, width + 1))
            theoretical_time = cumulative * steps
            theoretical_space = max(cumulative, 1)

        data["theoretical_time"] = float(theoretical_time)
        data["theoretical_space"] = float(theoretical_space)
        data["generated_nodes_log10"] = log10(run.generated_nodes)
        data["expanded_nodes_log10"] = log10(run.expanded_nodes)
        data["aux_memory_log10"] = log10(run.aux_memory)
        data["theoretical_time_log10"] = log10(theoretical_time)
        data["theoretical_space_log10"] = log10(theoretical_space)
        enriched.append(data)
    return enriched


def write_csv(rows: List[Dict[str, object]], path: Path) -> None:
    if not rows:
        return
    fieldnames = list(rows[0].keys())
    with path.open("w", newline="") as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def write_markdown_table(rows: List[Dict[str, object]], path: Path, columns: Iterable[str]) -> None:
    if not rows:
        return
    columns = list(columns)
    lines = [
        "| " + " | ".join(columns) + " |",
        "| " + " | ".join(["---"] * len(columns)) + " |",
    ]
    for row in rows:
        line = "| " + " | ".join(str(row.get(col, "")) for col in columns) + " |"
        lines.append(line)
    path.write_text("\n".join(lines))


def build_tables(rows: List[Dict[str, object]]) -> None:
    grouped: Dict[int, List[Dict[str, object]]] = {algo: [] for algo in ALGORITHMS}
    for row in rows:
        grouped[int(row["algorithm_id"])].append(row)

    for algo_id, data in grouped.items():
        if not data:
            continue
        data.sort(key=lambda r: r["puzzle"])
        columns = [
            "puzzle",
            "execution_time",
            "generated_nodes",
            "aux_memory",
            "num_pieces",
            "steps",
            "empty_spaces",
            "solving_width",
            "expanded_nodes",
            "duplicated_nodes",
        ]
        write_markdown_table(data, OUTPUT_DIR / f"table_algorithm{algo_id}.md", columns)

    write_csv(rows, OUTPUT_DIR / "experiment_metrics.csv")


def scale_points(points: List[Tuple[float, float]], width: int, height: int, padding: int = 50) -> List[Tuple[float, float]]:
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)
    span_x = max(max_x - min_x, 1e-6)
    span_y = max(max_y - min_y, 1e-6)

    scaled = []
    for x, y in points:
        sx = padding + (x - min_x) / span_x * (width - 2 * padding)
        sy = height - padding - (y - min_y) / span_y * (height - 2 * padding)
        scaled.append((sx, sy))
    return scaled


def generate_scatter_svg(rows: List[Dict[str, object]], x_key: str, y_key: str, filename: str,
                          title: str, x_label: str, y_label: str) -> None:
    filtered = [row for row in rows if row.get(x_key) is not None and row.get(y_key) is not None]
    if not filtered:
        return
    points = [(row[x_key], row[y_key]) for row in filtered]
    scaled = scale_points(points, width=800, height=600)
    svg_lines = [
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"600\">",
        f"<text x=\"400\" y=\"30\" text-anchor=\"middle\" font-size=\"18\">{title}</text>",
        f"<text x=\"400\" y=\"585\" text-anchor=\"middle\" font-size=\"14\">{x_label}</text>",
        f"<text x=\"20\" y=\"300\" transform=\"rotate(-90,20,300)\" text-anchor=\"middle\" font-size=\"14\">{y_label}</text>",
    ]

    for (sx, sy), row in zip(scaled, filtered):
        svg_lines.append(f"<circle cx=\"{sx:.2f}\" cy=\"{sy:.2f}\" r=\"5\" fill=\"#1f77b4\" />")
        svg_lines.append(
            f"<text x=\"{sx + 6:.2f}\" y=\"{sy - 6:.2f}\" font-size=\"10\">{row['puzzle']}</text>"
        )

    svg_lines.append("</svg>")
    (OUTPUT_DIR / filename).write_text("\n".join(svg_lines))


def generate_figures(rows: List[Dict[str, object]]) -> None:
    for algo_id, algo_name in ALGORITHMS.items():
        subset = [row for row in rows if int(row["algorithm_id"]) == algo_id]
        if not subset:
            continue
        generate_scatter_svg(
            subset,
            x_key="theoretical_time_log10",
            y_key="generated_nodes_log10",
            filename=f"time_scatter_algorithm{algo_id}.svg",
            title=f"{algo_name}: Theoretical vs Generated Nodes",
            x_label="log10(Theoretical worst-case nodes)",
            y_label="log10(Generated nodes)",
        )
        generate_scatter_svg(
            subset,
            x_key="theoretical_space_log10",
            y_key="expanded_nodes_log10",
            filename=f"space_scatter_algorithm{algo_id}.svg",
            title=f"{algo_name}: Space usage",
            x_label="log10(Theoretical space)",
            y_label="log10(Expanded nodes)",
        )

    algo1_map = {row["puzzle"]: row for row in rows if int(row["algorithm_id"]) == 1}
    comparative_points: List[Dict[str, object]] = []
    for row in rows:
        base = algo1_map.get(row["puzzle"])
        if base and row.get("expanded_nodes_log10") is not None and base.get("theoretical_space_log10") is not None:
            comparative_points.append({
                "puzzle": row["puzzle"],
                "algorithm_id": row["algorithm_id"],
                "theoretical_space_log10": base["theoretical_space_log10"],
                "expanded_nodes_log10": row["expanded_nodes_log10"],
            })
    if comparative_points:
        generate_scatter_svg(
            comparative_points,
            x_key="theoretical_space_log10",
            y_key="expanded_nodes_log10",
            filename="space_comparative.svg",
            title="Comparative space usage vs Algorithm 1 theoretical bound",
            x_label="log10(Algorithm 1 theoretical space)",
            y_label="log10(Expanded nodes)",
        )


def main() -> None:
    runs = gather_results()
    rows = compute_theoretical_metrics(runs)
    build_tables(rows)
    generate_figures(rows)
    print(f"Generated experiment outputs under {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
