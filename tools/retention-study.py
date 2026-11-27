#!/usr/bin/env python3
"""Compare inherited learning rules, follow freely born cohorts and retest identifiable animals on CUDA."""

import argparse
import csv
import os
import re
import subprocess
from pathlib import Path

from capillary_audit import read
from context_audit import stream

ROOT = Path(__file__).resolve().parents[1]
SOURCES = {
    "brook": ("capillary-nursery", 20480),
    "older": ("capillary-older-nursery", 86016),
    "slow": ("capillary-slow-nursery", 20480),
    "coast": ("detritus-estuary", 428952),
    "continued-brook": ("context-watershed", 36864),
}


def summary(log, key):
    return next(
        dict(re.findall(r"([^\s=]+)=([^\s]+)", line))
        for line in log.splitlines()
        if line.startswith(key + "=")
    )


def study(out, binary, sources, residents, lives, episodes, advance):
    out.mkdir(parents=True, exist_ok=False)
    env = {**os.environ, "AXION_JOURNAL_DIR": str(out / "journal")}
    exam = []
    common = out / "public-quarry.flood"
    with common.open("xb") as f:
        subprocess.run(
            ["zstd", "-qdc", ROOT / "examples/capillary-nursery.flood.zst"],
            stdout=f,
            check=True,
        )

    def run(tag, home, *args):
        path = out / (tag + ".log")
        with path.open("x") as f:
            subprocess.run(
                list(map(str, [binary, "floodplain", "--load", home, *args])),
                cwd=ROOT,
                env=env,
                stdout=f,
                stderr=subprocess.STDOUT,
                check=True,
            )
        return path.read_text()

    def enroll(tag, home, when, setting, family):
        receipt = out / (tag + ".tsv")
        result = run(
            tag,
            home,
            "--steps",
            0,
            "--capillary-memory",
            setting,
            "--capillary-residents",
            residents,
            "--retina-born-after",
            max(0, when - 2048),
            "--capillary-contacts",
            0,
            "--capillary-history",
            "empty",
            "--context-streams",
            lives,
            "--context-episodes",
            episodes,
            "--context-transfer-at",
            episodes // 2,
            "--context-transfer-rock",
            family,
            "--context-change",
            37,
            "--class-seed",
            4609327,
            "--capillary-rock",
            0,
            "--capillary-depth",
            9,
            "--capillary-moves",
            5,
            "--capillary-supply",
            "scarce",
            "--capillary-screen",
            receipt,
        )
        exam.append({"run": tag, **summary(result, "context_population")})
        return read(receipt) if receipt.exists() else []

    def teach_and_retest(tag, home, cohort):
        # All qualifying juveniles in census order up to a prespecified maximum.
        both = [
            r
            for r in cohort
            if r["basal_lut"] == 14
            and r["local_lut"] == 10
            and r["native_whole"] == lives
        ]
        for i, r in enumerate(both[:16]):
            subject = tag + "-" + str(r["slot"])
            young = out / (subject + "-birth.mind")
            educated = out / (subject + ".mind")
            run(
                subject + "-birth",
                home,
                "--steps",
                0,
                "--slot",
                r["slot"],
                "--export-mind",
                young,
            )
            run(
                subject + "-school",
                common,
                "--steps",
                0,
                "--capillary-pupil",
                young,
                "--capillary-memory",
                "regional",
                "--capillary-trials",
                32,
                "--class-seed",
                2027909,
                "--capillary-depth",
                9,
                "--capillary-vocabulary",
                384,
                "--capillary-recipes",
                1152,
                "--capillary-interviews",
                96,
                "--capillary-save",
                educated,
            )
            geometries = ("assembled", "living") if i == 0 else ("assembled",)
            for geometry in geometries:
                for family in ((9, 11, 6) if geometry == "living" else (9,)):
                    motions = {}
                    for arm in (
                        ("adaptive", "retention", "clamped")
                        if geometry == "living"
                        else ("adaptive",)
                    ):
                        name = f"{subject}-{geometry}-{family}-{arm}"
                        trace = out / (name + ".tsv")
                        run(
                            name,
                            common,
                            "--steps",
                            0,
                            "--capillary-pupil",
                            educated,
                            "--context-arm",
                            arm,
                            "--context-streams",
                            256 if geometry == "living" else 128,
                            "--context-episodes",
                            16,
                            "--context-transfer-at",
                            8,
                            "--context-transfer-rock",
                            family,
                            "--context-change",
                            43,
                            "--context-questions",
                            "varied",
                            "--class-seed",
                            6298421,
                            "--capillary-depth",
                            10,
                            "--capillary-moves",
                            5,
                            "--capillary-supply",
                            "scarce",
                            "--capillary-works",
                            geometry,
                            "--context-trace",
                            trace,
                        )
                        events = read(trace)
                        stream(events, None, None, physical=False)
                        motions[arm] = [
                            (v["fingerprint"], v["heat"], v["key"]) for v in events
                        ]
                    assert all(v == motions["adaptive"] for v in motions.values())

    for name in sources:
        archive, tick = SOURCES[name]
        home = out / (name + ".flood")
        with home.open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ROOT / "examples" / (archive + ".flood.zst")],
                stdout=f,
                check=True,
            )
        for memory in ("adaptive", "regional"):
            for family in ((0, 9, 7) if memory == "adaptive" else (0, 9)):
                key = f"{name}-{memory}-{family}"
                cohort = enroll(key, home, tick, memory, family)
                if memory == "regional" and family == 9:
                    teach_and_retest(name, home, cohort)
            if advance:
                key = f"{name}-descendants-{memory}"
                end = out / (key + ".flood")
                age = 1024 if name == "coast" else 32768
                run(
                    key,
                    home,
                    "--steps",
                    age,
                    "--report-every",
                    age // 4,
                    "--capillary-field",
                    "live",
                    "--capillary-memory",
                    memory,
                    "--capillary-food",
                    "pigments",
                    "--capillary-cargo",
                    "pressure",
                    "--capillary-cargo-dye",
                    "exchange",
                    "--capillary-period",
                    4 if "brook" in name else 8,
                    "--context-events",
                    out / (key + "-choices.tsv"),
                    "--save",
                    end,
                )
                # At 1,024 coastal ticks use only residents born since the intervention.
                when = max(tick + age, tick + 2048)
                cohort = enroll(key + "-cohort", end, when, memory, 9)
                if memory == "regional":
                    teach_and_retest(key, end, cohort)
    if exam:
        with (out / "summary.csv").open("x") as f:
            writer = csv.DictWriter(
                f,
                fieldnames=list(dict.fromkeys(k for r in exam for k in r)),
                lineterminator="\n",
            )
            writer.writeheader()
            writer.writerows(exam)


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--binary", type=Path, default=ROOT / "axion")
    p.add_argument("--habitats", nargs="+", choices=SOURCES, default=["brook"])
    p.add_argument("--residents", type=int, default=2048)
    p.add_argument("--lives", type=int, default=8)
    p.add_argument("--episodes", type=int, default=10)
    p.add_argument("--advance", action="store_true")
    a = p.parse_args()
    study(
        a.output.resolve(),
        a.binary.resolve(),
        a.habitats,
        a.residents,
        a.lives,
        a.episodes,
        a.advance,
    )
