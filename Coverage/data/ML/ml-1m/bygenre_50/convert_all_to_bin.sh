#!/bin/bash

# Script to retrieve filenames in every folder of the current directory

for dir in */; do
    if [ -d "$dir" ]; then
        for file in "$dir"*; do
            if [ -f "$file" ]; then
                cargo run --manifest-path "../../../../../InfluenceMaximization/capngraph-master/Cargo.toml" --bin "convert" --features="bins" ${file} "../../../../../InfluenceMaximization/data/bin/bygenre_50/${file%.txt}.bin" 
            fi
        done
    fi
done
