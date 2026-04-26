#!/bin/bash

# Bash script to run tiptop first, wait for it to finish, then run degreeDiscount k times.
# Assumes:
# - tiptop is built in tiptop-master/target/release/tiptop
# - degreeDiscount is compiled as ./degreeDiscount in the InfluenceMaximization directory
# - degreeDiscount takes arguments: <graph> <k> (adjust if its interface differs)
# - k is the third argument (number of seeds/runs for degreeDiscount)
# Usage: ./run_scripts.sh <graph> <model> <k> <epsilon> [<delta>] [other tiptop options]

if [ $# -lt 2 ]; then
    echo "Usage: $0 <graph> <k> [other tiptop options]"
    exit 1
fi

GRAPH=$1
K=$2
shift 2  # Shift to pass remaining args to tiptop

# Change to tiptop-master directory and run tiptop
for i in $(seq 12 "$K"); do
    cd tiptop-master || { echo "Error: tiptop-master directory not found"; exit 1; }
    echo "Running tiptop with graph=$GRAPH, k=$i, epsilon=$0.02"
    cargo run --bin tiptop "../data/${GRAPH}.bin" "IC" "$i" "0.02" --threads 12 "$@"
    TIKTOP_EXIT_CODE=$?
    if [ $TIKTOP_EXIT_CODE -ne 0 ]; then
        echo "Error: tiptop failed with exit code $TIKTOP_EXIT_CODE"
        exit $TIKTOP_EXIT_CODE
    fi
    echo "tiptop completed successfully"

    # # Change back to InfluenceMaximization directory
    cd .. || { echo "Error: Could not return to parent directory"; exit 1; }

    # # Run degreeDiscount k times (assuming it takes <graph> <k> as args; adjust as needed)
    # echo "Running degreeDiscount graph=$GRAPH, k=$i"
    # echo "Run $i of degreeDiscount"
    # ./degreeDiscount "$i" "$GRAPH" "data/temp_opt.txt"  || { echo "Error: degreeDiscount run $i failed"; exit 1; }
done

echo "All runs completed"
