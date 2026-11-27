#!/usr/bin/env python3
"""Measure whether informative glints confer a reproductive advantage in reciprocal wild habitats."""

import argparse
import csv
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FOUNDERS = (73453, 106619, 146381, 193939, 242927, 299171)
ARMS = ("vision", "reverse-vision", "sham")
TRACER_FOUNDERS = (190949, 221581, 324113, 471869, 582317, 715657)


def contest(directory, seeds, sources, intervention="light"):
    tracing = intervention == "tracers"
    field = (
        ["--daylight-gaze", "inherited", "--daylight-probes", "stirred"]
        if tracing
        else []
    )
    directory.mkdir(parents=True, exist_ok=True)
    binary = Path(os.environ.get("AXION_BIN", ROOT / "axion")).resolve()

    def run(name, *args):
        p = subprocess.run(
            list(map(str, [binary, "floodplain", *args])),
            capture_output=True,
            text=True,
            cwd=ROOT,
        )
        (directory / (name + ".log")).write_text(p.stdout + p.stderr)
        if p.returncode:
            raise RuntimeError(f"{name} failed. See its log in {directory}")
        return p.stdout

    with (directory / "replicates.csv").open("w", newline="") as f:
        writer = csv.writer(f, lineterminator="\n")
        writer.writerow(["seed", "arm", "tick", "label_1", "label_2", "fraction_1"])
        for seed in seeds:
            with tempfile.TemporaryDirectory(prefix=".contest-", dir=directory) as temp:
                d = Path(temp)
                run(
                    f"fresh-{seed}",
                    "--side",
                    128,
                    "--neurons",
                    1024,
                    "--density",
                    100,
                    "--neurogenesis",
                    "inherited",
                    "--seed",
                    seed,
                    "--drift",
                    "tidal",
                    "--drift-lateral",
                    "contextual",
                    "--drift-crossing",
                    "seep",
                    "--drift-stores",
                    "sills",
                    "--drift-load",
                    64,
                    "--drift-stride",
                    1,
                    "--daylight",
                    "live",
                    "--daylight-period",
                    4,
                    "--daylight-banks",
                    "tidal",
                    "--shore-depth",
                    "shoals",
                    *field,
                    "--steps",
                    16384,
                    "--report-every",
                    4096,
                    "--drift-first",
                    8192,
                    "--brain-file",
                    "compact",
                    "--save",
                    d / "wild",
                )
                run(
                    f"mark-{seed}",
                    "--load",
                    d / "wild",
                    "--steps",
                    0,
                    "--daylight",
                    "live",
                    "--daylight-period",
                    4,
                    "--daylight-banks",
                    "tidal",
                    "--drift-windings",
                    "woven",
                    "--shore-depth",
                    "shoals",
                    "--drift-contest",
                    "sham",
                    "--glint-sources",
                    sources,
                    "--drift-label",
                    seed,
                    "--brain-file",
                    "compact",
                    "--save",
                    d / "marked",
                )
                for arm in (
                    ("lineages", "reverse-lineages", "stirred") if tracing else ARMS
                ):
                    manipulation = (
                        [
                            "--daylight-probes",
                            arm,
                            "--probe-trace",
                            directory / f"grains-{seed}-{arm}.tsv",
                        ]
                        if tracing
                        else ["--drift-contest", arm]
                    )
                    log = run(
                        f"contest-{seed}-{arm}",
                        "--load",
                        d / "marked",
                        "--steps",
                        49152,
                        "--report-every",
                        8192,
                        *manipulation,
                        "--drift-first",
                        8192,
                        "--retina-trials",
                        64,
                        "--retina-beds",
                        2,
                        "--retina-order",
                        "woven",
                        "--retina-works",
                        ROOT / "docs/observational-learning/estuary-beds.txt",
                        "--inlet-practice",
                        32,
                        "--inlet-prior",
                        16,
                        "--inlet-state",
                        "carried",
                        "--inlet-residents",
                        8192,
                        "--retina-born-after",
                        24576,
                        "--class-seed",
                        84913,
                    )
                    counts = {}
                    for l in log.splitlines():
                        if l.startswith("drift_lineage="):
                            row = dict(field.split("=", 1) for field in l.split())
                            label = int(row["drift_lineage"])
                            if label in (1, 2):
                                counts.setdefault(int(row["tick"]), {})[label] = int(
                                    row["living"]
                                )
                    for tick, v in sorted(counts.items()):
                        a, b = v[1], v[2]
                        writer.writerow(
                            [seed, arm, tick, a, b, a / (a + b) if a + b else ""]
                        )
                        f.flush()


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--seeds", type=int, nargs="+", default=None)
    p.add_argument(
        "--sources",
        choices=("other", "independent"),
        default="other",
        help="sample an alternate inlet or an independent inlet from all four",
    )
    p.add_argument("--intervention", choices=("light", "tracers"), default="light")
    q = p.parse_args()
    output = q.output.resolve()
    try:
        output.mkdir(parents=True)
    except FileExistsError:
        output = Path(tempfile.mkdtemp(prefix="contest-run-", dir=output))
    print(f"Contest directory {output}", flush=True)
    contest(
        output,
        q.seeds or (TRACER_FOUNDERS if q.intervention == "tracers" else FOUNDERS),
        q.sources,
        q.intervention,
    )
