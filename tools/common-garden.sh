#!/bin/sh
# Reproduce the reciprocal transplants in docs/ecology.md. Output is CSV.
set -eu
repo=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
binary=${AXION_BIN:-"$repo/axion"}
scratch=$(mktemp -d "${TMPDIR:-/var/tmp}/axion-gardens.XXXXXX")
trap 'rm -rf -- "$scratch"' EXIT
trap 'exit 1' INT TERM HUP
printf 'donor,habitat,swapped,contest,initial_treatment,initial_control,final_treatment,final_control\n'
for donor in 379 2191 8461; do
    "$binary" life --side 64 --neurons 256 --seed "$donor" --steps 4096 --save "$scratch/donor.eco" > "$scratch/training"
    for habitat in 601 1423 7919 9127; do
        for swapped in 0 1; do
            for contest in ancestry brain body agency plasticity; do
                set -- life --founders "$scratch/donor.eco" --side 64 --neurons 256 --seed "$habitat" \
                    --steps 512 --mutation off --contest "$contest"
                if [ "$swapped" -eq 1 ]; then set -- "$@" --swap-cohorts; fi
                "$binary" "$@" > "$scratch/report"
                awk -v d="$donor" -v h="$habitat" -v s="$swapped" -v c="$contest" '
                    /^tick=/ {
                        for (i=1; i<=NF; ++i) { split($i, pair, "="); row[pair[1]]=pair[2] }
                        if (row["tick"] == 0) { initial=row["treatment_alive"]; control=row["control_alive"] }
                    }
                    END { printf "%s,%s,%s,%s,%s,%s,%s,%s\n", d,h,s,c,initial,control,row["treatment_alive"],row["control_alive"] }
                ' "$scratch/report"
            done
        done
    done
done
