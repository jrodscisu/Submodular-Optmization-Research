#!/bin/bash

# Bash script to run tiptop first, wait for it to finish, then run degreeDiscount k times.
# Assumes:
# - tiptop is built in tiptop-master/target/release/tiptop
# - degreeDiscount is compiled as ./degreeDiscount in the InfluenceMaximization directory
# - degreeDiscount takes arguments: <graph> <k> (adjust if its interface differs)
# - k is the third argument (number of seeds/runs for degreeDiscount)
# Usage: ./run_scripts.sh <graph> <model> <k> <epsilon> [<delta>] [other tiptop options]

if [ $# -lt 2 ]; then
    echo "Usage: $0 minK maxK [timeout_seconds] [other tiptop options]"
    exit 1
fi

minK=$1
maxK=$2
TIMEOUT=${3:-300}  # Default 5 minutes if not provided
shift 3  # Shift to pass remaining args to tiptop


for dir in ../Coverage/data/ML/ml-1m/bygenre_50/*/; do
    if [ -d "$dir" ]; then
        for file in "$dir"*; do
            if [ -f "$file" ]; then
                # Remove the base path to get relative path
                relative_file=${file#../Coverage/data/ML/ml-1m/bygenre_50/}
                
                for i in $(seq "$minK" "$maxK"); do
                    cd tiptop-master || { echo "Error: tiptop-master directory not found"; exit 1; }
                    echo "Running tiptop with graph=$relative_file, k=$i, epsilon=0.02"
                    timeout $TIMEOUT cargo run --bin tiptop "../data/bin/bygenre_50/${relative_file%.txt}.bin" "IC" "$i" "0.02" --threads 12
                    TIKTOP_EXIT_CODE=$?
                    if [ $TIKTOP_EXIT_CODE -eq 124 ]; then
                        echo "tiptop timed out after $TIMEOUT seconds"
                        cd ..
                        continue 
                    elif [ $TIKTOP_EXIT_CODE -ne 0 ]; then
                        echo "Error: tiptop failed with exit code $TIKTOP_EXIT_CODE"
                        exit $TIKTOP_EXIT_CODE
                    else
                        echo "tiptop completed successfully"
                    fi

                    # Change back to InfluenceMaximization directory
                    cd .. || { echo "Error: Could not return to parent directory"; exit 1; }

                    # Run degreeDiscount k times (assuming it takes <graph> <k> as args; adjust as needed)
                    echo "Running Greedy Coverage graph=$relative_file, k=$i"
                    echo "Run $i of Greedy"
                    ./../Coverage/src/greedy_bygenre_50 "$i" "$file" "data/temp_opt.txt" "bygenre_50_output/${relative_file%.txt}"
                    # ./degreeDiscount "$i" "$GRAPH" "data/temp_opt.txt"  || { echo "Error: degreeDiscount run $i failed"; exit 1; }
                done

                echo "All runs completed"
            fi
        done
    fi
done
