#!/usr/bin/env python3
import argparse
import os
import re

COVERAGE_PATTERNS = {
    "Greedy": re.compile(r"^Greedy coverage(?: is)?:\s*(\d+)"),
    "TipTop": re.compile(r"^TipTop coverage(?: is)?:\s*(\d+)"),
    "OH": re.compile(r"^OH coverage(?: is)?:\s*(\d+)")
}
K_PATTERN = re.compile(r"^-+\s*K\s*=\s*(\d+)-+")


def parse_coverage_file(path):
    results = {}
    current_k = None

    with open(path, "r", encoding="utf-8", errors="replace") as infile:
        for line in infile:
            line = line.strip()
            if not line:
                continue

            k_match = K_PATTERN.match(line)
            if k_match:
                current_k = int(k_match.group(1))
                results[current_k] = {}
                continue

            if current_k is None:
                continue

            for label, pattern in COVERAGE_PATTERNS.items():
                match = pattern.match(line)
                if match:
                    results[current_k][label] = int(match.group(1))
                    break

    return results


def compute_largest_gap(coverages):
    best_gap = None
    best_meta = None

    for k, values in coverages.items():
        if "Greedy" not in values or "TipTop" not in values:
            continue

        greedy_value = values["Greedy"]
        tiptop_value = values["TipTop"]
        if greedy_value == 0:
            continue

        pct = 100.0 * (tiptop_value - greedy_value) / greedy_value
        if best_gap is None or abs(pct) > abs(best_gap):
            best_gap = pct
            best_meta = {
                "k": k,
                "greedy_value": greedy_value,
                "tiptop_value": tiptop_value,
            }

    return best_gap, best_meta


def format_label_from_filename(path):
    name = os.path.basename(path)
    if name.endswith("_output.txt"):
        return name[: -len("_output.txt")]
    return name


def main():
    parser = argparse.ArgumentParser(description="Compute the largest percentage gap in a bygenre output file.")
    parser.add_argument("input_file", help="Path to one bygenre output file")
    parser.add_argument("aggregate_output", nargs="?", help="Optional aggregate file to append the result")
    args = parser.parse_args()

    coverages = parse_coverage_file(args.input_file)
    if not coverages:
        raise SystemExit(f"No coverage data found in {args.input_file}")

    best_gap, best_meta = compute_largest_gap(coverages)
    label = format_label_from_filename(args.input_file)

    if best_meta is None:
        line = f"{label} has no valid percentage gap between TipTop and Greedy"
    else:
        gap_str = f"{abs(best_gap):.4f}%"
        if best_gap > 0:
            line = f"{label}: TipTop is {gap_str} better than Greedy at K={best_meta['k']}"
        elif best_gap < 0:
            line = f"{label}: TipTop is {gap_str} worse than Greedy at K={best_meta['k']}"
        else:
            line = f"{label}: TipTop and Greedy are equal at K={best_meta['k']}"

    if args.aggregate_output:
        with open(args.aggregate_output, "a", encoding="utf-8") as outfile:
            outfile.write(line + "\n")
    else:
        print(line)


if __name__ == "__main__":
    main()
