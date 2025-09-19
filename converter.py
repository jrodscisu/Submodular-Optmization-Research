import csv

input_file = "Outputs/outputs.txt"
output_file = "Outputs/outputs.csv"

with open(input_file, "r") as fin, open(output_file, "w", newline="") as fout:
    writer = csv.writer(fout)
    # Optional: write header
    writer.writerow(["K", "R", "RANDOMS", "l", "B", "disjoint", "worse", "n_pred", "approximation"])
    for line in fin:
        line = line.strip()
        if not line or set(line) == {"-"}:
            continue
        writer.writerow(line.split())