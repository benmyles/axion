#!/usr/bin/env python3
"""Evolve a delayed learner and test transfer of one physical change to seven untouched fruits."""
import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--axion", default="./axion")
parser.add_argument("--output", required=True, type=Path)
parser.add_argument("--seeds", nargs="+", type=int, default=[1877, 5569, 8123, 31139])
parser.add_argument("--capacity", type=int, default=16384)
parser.add_argument("--first-seasons", type=int, default=1024)
parser.add_argument("--seasons", type=int, default=2048)
parser.add_argument("--subjects", type=int, default=1024)
parser.add_argument("--ripening", choices=["steady", "flush"], default="steady")
parser.add_argument("--exposures", nargs="+", type=int, default=[0, 8, 32, 64])
args = parser.parse_args()


def field(*options):
    cmd = [args.axion, "floodplain", "--materials", "off", "--neurogenesis", "fixed", "--pod-pulse", "enteric", "--steps", "0", *map(str, options)]
    output = subprocess.run(cmd, text=True, check=True, stdout=subprocess.PIPE).stdout
    return [dict(part.split("=", 1) for part in line.split())
            for line in output.splitlines() if line.startswith("pod_remap=")]


with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-frames-") as stage:
    ancestor = Path(stage) / "parents.flood"
    brood = Path(stage) / "juveniles.flood"
    with args.output.open("w", newline="") as out:
        writer = None
        for seed in args.seeds:
            field("--seed", seed, "--pods", "sealed", "--pod-frames", "single", "--pod-regimes", "independent",
                  "--raft-capacity", args.capacity, "--raft-seasons", args.first_seasons, "--raft-days", 512, "--save", ancestor)
            for regime in ("provisioned", "fostered"):
                field("--colonists", ancestor, "--seed", seed ^ 0x70b1, "--pods", "sealed", "--pod-period", 48,
                      "--pod-frames", "inherited", "--pod-regimes", "shared", "--raft-frames", "diverse",
                      "--recruit", regime, "--raft-seasons", args.seasons, "--raft-capacity", args.capacity,
                      "--raft-days", 768, "--raft-ripening", args.ripening, "--save", brood)
                for exposure in args.exposures:
                    for row in field("--load", brood, "--pod-remap", 768, "--pod-observe", exposure,
                                     "--pod-test-seed", seed ^ 0x235719, "--pod-subjects", args.subjects):
                        result = dict(rearing_seed=seed, recruitment=regime, **row)
                        if writer is None:
                            writer = csv.DictWriter(out, fieldnames=result.keys(), lineterminator="\n")
                            writer.writeheader()
                        writer.writerow(result)
                out.flush()
                print(seed, regime, "complete", flush=True)
