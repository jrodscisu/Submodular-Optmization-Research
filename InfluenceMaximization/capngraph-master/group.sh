#!/bin/bash
cat <(head -n1 $1) <(tail -n+2 $1 | sort -n)
