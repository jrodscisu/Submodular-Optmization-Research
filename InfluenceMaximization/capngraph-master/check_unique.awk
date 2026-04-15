#!/usr/bin/gawk -f
BEGIN {
    duplicates = 0;
}

$1 in edges && $2 in edges[$1] {
    printf "Edge %d → %d duplicated.\n", $1, $2
    duplicates += 1;
}

{
    edges[$1][$2] += 1;
}

END {
    if (duplicates > 0) {
        printf "%d duplicates found.\n", duplicates
    }
}
