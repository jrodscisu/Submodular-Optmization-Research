#!/usr/bin/env bash
set -euo pipefail

SCRIPTS=(
  "../LibMVC/data/run_all_mtx2mis.sh"
  "../LibMVC/standalone/run_all_tests.sh"
  "./run_all_tests.sh"
)

for s in "${SCRIPTS[@]}"; do
  printf "==> Running: %s\n" "$s"
  if [ ! -e "$s" ]; then
    printf "Warning: %s not found, skipping\n" "$s" >&2
    continue
  fi

  if [ -x "$s" ]; then
    "$s"
  else
    # try to infer interpreter: python if shebang mentions python, else bash
    if head -n 1 "$s" 2>/dev/null | grep -qE 'python'; then
      python3 "$s"
    else
      bash "$s"
    fi
  fi
done

printf "All