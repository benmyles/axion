#!/usr/bin/env python3
"""Verify real food flux, clock alignment, saturated receivers and checkpoint continuity on CUDA."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path
from press import ROOT, call, rows

sys.path.insert(0, str(ROOT / "tools"))
from detritus_audit import audit, bounds


def run():
    with tempfile.TemporaryDirectory(
        dir=os.environ.get("TMPDIR", "/var/tmp"), prefix="axion-detritus-"
    ) as folder:
        d = Path(folder)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")
        base = d / "origin.flood"
        with base.open("xb") as f:
            subprocess.run(
                ["zstd", "-qdc", ROOT / "examples/capillary-nursery.flood.zst"],
                stdout=f,
                check=True,
            )

        def flood(*args, ok=True):
            return call("floodplain", *args, ok=ok)

        table = d / "bounds.tsv"
        l = flood("--load", base, "--steps", 0, "--capillary-cargo-proof", table)
        assert bounds(table)["blocked"] == int(
            rows(l, "detritus_boundaries")[0]["denied"]
        )
        for pulse in (1, 8, 64, 256):
            # Enter the CUDA graph at an odd time and force further short remainders.
            start = d / f"offset-{pulse}.flood"
            flood(
                "--load",
                base,
                "--steps",
                7,
                "--capillary-cargo",
                "pressure",
                "--capillary-cargo-dye",
                "exchange",
                "--capillary-period",
                pulse,
                "--save",
                start,
            )
            results = []
            for mode in ("full", "scheduled"):
                stem = f"{pulse}-{mode}"
                wake = d / (stem + ".tsv")
                save = d / (stem + ".flood")
                steps = 281
                log = flood(
                    "--load",
                    start,
                    "--steps",
                    steps,
                    "--report-every",
                    37,
                    "--capillary-dispatch",
                    mode,
                    "--capillary-cargo-events",
                    wake,
                    "--save",
                    save,
                )
                info = rows(log, "detritus_field")[0]
                launch = rows(log, "capillary_dispatch")[0]
                assert int(info["stock_summed_before"]) == int(
                    info["stock_summed_after"]
                ) and int(info["arrived"]) == int(info["departed"])
                assert (
                    int(info["routing_site_bits"])
                    == int(info["permanent_site_bits"])
                    == 0
                    and int(info["transient_site_bits"]) == 1
                )
                assert audit(wake)["uniform"] > 0
                num = int(launch["cargo_launches"])
                assert (
                    num == steps
                    if mode == "full" or pulse == 1
                    else int(launch["physical_beats"]) <= num < steps
                )
                results.append((save.read_bytes(), wake.read_bytes()))
            assert results[0] == results[1], (
                "clock dispatch changed the physical world",
                pulse,
            )
        for mode in ("pressure", "displaced"):
            a = d / (mode + "-a.flood")
            b = d / (mode + "-b.flood")
            z = d / (mode + "-z.flood")
            full = flood(
                "--load", base, "--steps", 211, "--capillary-cargo", mode, "--save", a
            )
            flood("--load", base, "--steps", 19, "--capillary-cargo", mode, "--save", b)
            flood("--load", b, "--steps", 192, "--report-every", 13, "--save", z)
            assert a.read_bytes() == z.read_bytes()
            q = rows(full, "detritus_field")[0]
            assert (
                q["arrived"] == q["departed"]
                and int(q["arrived"]) > 0
                and q["observed"] == "0"
            )
            paused = d / (mode + "-off.flood")
            flood(
                "--load",
                a,
                "--steps",
                17,
                "--capillary-cargo",
                "still",
                "--save",
                paused,
            )
            report = flood("--load", paused, "--steps", 9)
            assert not rows(report, "detritus_field")
        tiny = d / "tiny.flood"
        expanded = d / "expanded.flood"
        flood(
            "--resident",
            ROOT / "examples/capillary-guide.mind",
            "--side",
            4,
            "--density",
            0,
            "--capillary-field",
            "live",
            "--capillary-cargo",
            "pressure",
            "--capillary-cargo-dye",
            "exchange",
            "--steps",
            3,
            "--save",
            tiny,
        )
        result = flood(
            "--load", tiny, "--extend", 32, "--steps", 41, "--save", expanded
        )
        assert rows(result, "detritus_field")[0]["dye_exchange"] == "1"
        flood("--load", expanded, "--steps", 0)
        clash = d / "shared.tsv"
        msg = flood(
            "--load",
            base,
            "--steps",
            10,
            "--capillary-cargo",
            "pressure",
            "--capillary-cargo-events",
            clash,
            "--capillary-events",
            clash,
            ok=False,
        )
        assert "distinct output" in msg and not clash.exists()
    print(
        "detritus passed: 65,536 boundary cases, measured mass, displaced pressure, exact graph replay and restart"
    )


if __name__ == "__main__":
    run()
