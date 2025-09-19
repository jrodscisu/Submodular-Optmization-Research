import subprocess
import sys

# Read input tuples from a file
input_file = "Inputs/inputs_no_predictions.txt"

input_sets = []
with open(input_file, "r") as f:
    for line in f:
        # Skip empty lines and comments
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) == 5:
            l, B, which, best, n_pred = parts
            input_sets.append((l, B, which, best, n_pred))

for l, B, which, best, n_pred in input_sets:
    
    result = subprocess.run(
        ["./Greedy/greedy", str(l), str(B), str(which), str(best), str(n_pred)],
        capture_output=True,
        text=True
    )
    output = result.stdout

    output_lines = [line for line in output.splitlines() if line.strip() != "-" * 40]
    print("\n".join(output_lines))

    if result.stderr:
        print("Errors:")
        print(result.stderr)
    print("-" * 40)