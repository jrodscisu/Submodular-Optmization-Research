import subprocess

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
        if len(parts) == 3:
            l, B, which = parts
            input_sets.append((l, B, which))

for l, B, which in input_sets:
    
    result = subprocess.run(
        ["./Greedy/greedy", str(l), str(B), str(which)],
        capture_output=True,
        text=True
    )
    print(f"{l} {B} {which} {result.stdout}")

    if result.stderr:
        print("Errors:")
        print(result.stderr)
    print("-" * 40)