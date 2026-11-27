"""Verify real carbon transfer, learned decisions, physical inheritance and retained prediction errors."""

import csv
import importlib.util
import os
import tempfile
from pathlib import Path
from press import call, rows
from worksites import ROOT, snapshot

spec = importlib.util.spec_from_file_location(
    "drift_audit", ROOT / "tools/drift-audit.py"
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
student = ROOT / "examples/estuary-born-student.mind"
peer = ROOT / "examples/parallax-witness.mind"


def main():
    with tempfile.TemporaryDirectory(
        prefix="axion-drift-test-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as tmp:
        d = Path(tmp)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")

        def field(*args, **kw):
            return call("floodplain", *args, **kw)

        source = d / "base"
        field(
            "--side",
            32,
            "--density",
            100,
            "--neurons",
            256,
            "--neurogenesis",
            "inherited",
            "--worksites",
            "active",
            "--seed",
            3913,
            "--drift",
            "tidal",
            "--contact-remodel",
            "gated",
            "--drift-stride",
            1,
            "--drift-load",
            32,
            "--drift-stock",
            student,
            "--drift-contest",
            "sham",
            "--drift-label",
            7019,
            "--steps",
            0,
            "--save",
            source,
        )
        tags = [r[16] >> 14 for r in snapshot(source)["riggers"]]
        assert tags.count(1) == tags.count(2) == 512
        trace = d / "flow.tsv"
        old = d / "inheritance.tsv"
        options = (
            "--load",
            source,
            "--steps",
            192,
            "--report-every",
            192,
            "--drift-contest",
            "reciprocal",
            "--drift-cargo",
            "rafts",
            "--drift-sight",
            "audience",
            "--contact-remodel",
            "gated",
            "--brain-file",
            "compact",
        )
        proof = field(
            *options,
            "--drift-trace",
            trace,
            "--drift-window",
            64,
            "--drift-tiles",
            256,
            "--drift-surfaces",
            2048,
            "--drift-legacy-trace",
            old,
            "--drift-profile",
            24,
            "--drift-account",
            "warp",
            "--save",
            d / "watched",
        )
        physical = audit.check_trace(trace)
        assert (
            physical["pulses"] > 100
            and physical["reaches"] > 100
            and physical["released"] == physical["eaten"] + physical["settled"]
        )
        audit.check_legacy(old)
        stats = rows(proof, "drift_tick")[-1]
        assert int(stats["released"]) == int(stats["eaten"]) + int(stats["settled"])
        assert int(stats["eaten"]) - int(stats["blank_eaten"]) == int(
            stats["gained"]
        ) - int(stats["lost"])
        assert (
            int(stats["contact_edits"]) > 0
            and int(stats["tied"]) + int(stats["untied"]) > 0
        )
        carriers = rows(proof, "drift_cargo")[-1]
        assert int(carriers["exchanged"]) > 0 and int(carriers["moving_paths"]) == int(
            carriers["exchanged"]
        ) + int(carriers["too_heavy"])
        labels = rows(proof, "drift_lineage")[-3:]
        assert all(
            int(q["initial"]) + int(q["born"]) == int(q["living"]) + int(q["died"])
            for q in labels
        )
        assert labels[1]["recall"] == "1" and labels[2]["recall"] == "0"
        costs = rows(proof, "drift_harvest")
        assert any(int(q["changed"]) for q in costs if q["drift_harvest"] == "1")
        assert all(
            q["eaten"] == q["without"] for q in costs if q["drift_harvest"] == "2"
        )
        field(*options, "--drift-account", "atomic", "--save", d / "plain")
        assert (d / "plain").read_bytes() == (d / "watched").read_bytes()
        remade = field(
            *options,
            "--contact-remodel",
            "inherited",
            "--drift-trace",
            d / "remade",
            "--drift-tiles",
            128,
            "--drift-window",
            32,
        )
        remodelled = rows(remade, "drift_tick")[-1]
        audit.check_trace(d / "remade")
        assert (
            int(remodelled["mode"]) & 4096
            and remodelled["contact_edits"] != stats["contact_edits"]
        )
        field(
            "--load",
            source,
            "--drift-contest",
            "reciprocal",
            "--drift-sight",
            "audience",
            "--contact-remodel",
            "gated",
            "--drift-cargo",
            "rafts",
            "--steps",
            79,
            "--brain-file",
            "compact",
            "--save",
            d / "partial",
        )
        field("--load", d / "partial", "--steps", 113, "--save", d / "split")
        assert (d / "plain").read_bytes() == (d / "split").read_bytes()
        spacious = field("--load", d / "plain", "--neurons", 1024, "--steps", 0)
        widths = rows(spacious, "neural_widths")[0]
        histogram = list(map(int, widths["by_32_cells"].split(",")))
        tissue = rows(spacious, "spiking_growth")[-1]
        assert sum(histogram) == int(widths["living"]) and sum(histogram[8:]) == 0
        assert sum(32 * (i + 1) * count for i, count in enumerate(histogram)) == int(
            tissue["tissue_cells"]
        )
        assert (
            int(rows(spacious, "neural_ceiling")[0]["prior_cells"]) == 256
            and len(histogram) == 32
        )
        stable = field(
            "--load",
            source,
            "--steps",
            192,
            "--drift-memory",
            "unread",
            "--drift-fibres",
            "bypass",
            "--drift-surfaces",
            2048,
            "--drift-legacy-trace",
            d / "no-seam",
            "--drift-repertoire",
            d / "alleles",
        )
        unchanged = rows(stable, "drift_tick")[-1]
        assert (
            unchanged["eaten"] == unchanged["blank_eaten"] == unchanged["bare_eaten"]
            and unchanged["choices_changed"] == "0"
        )
        assert rows(stable, "drift_legacy")[-1]["redirected"] == "0"
        heritage = field(
            "--side",
            32,
            "--density",
            100,
            "--seed",
            7919,
            "--worksites",
            "active",
            "--neurons",
            256,
            "--neurogenesis",
            "inherited",
            "--drift",
            "tidal",
            "--drift-stride",
            1,
            "--drift-load",
            32,
            "--drift-surfaces",
            2048,
            "--drift-legacy-trace",
            d / "field-edges",
            "--steps",
            512,
        )
        receipts = audit.check_legacy(d / "field-edges")
        example = rows(heritage, "drift_bequest")[0]
        assert receipts["later_receivers"] > 0 and int(example["eaten"]) > 0
        assert example["maker_gone"] == "1" and int(example["receiver_born"]) > int(
            example["placed"]
        )

        new = field(
            "--load",
            d / "plain",
            "--steps",
            0,
            "--drift-germ",
            d / "common",
            "--drift-probe",
            4,
            "--drift-food-germ",
            d / "selected",
            "--drift-probe-trace",
            d / "screen",
        )
        assays = rows(new, "drift_food_test")
        assert len(assays) == 2 and assays[0]["champion"] == assays[1]["champion"]
        assert int(assays[1]["individual_offered"]) == 384
        with (d / "screen").open() as f:
            data = list(csv.DictReader(f, delimiter="\t"))
        selection = [q for q in data if q["panel"] == "0"]
        expected = max(
            selection,
            key=lambda q: (
                min(int(q[k]) for k in ("first_tide", "after_shift", "after_return")),
                int(q["food"]),
            ),
        )
        assert expected["index"] == assays[0]["champion"]
        field(
            "--load",
            source,
            "--steps",
            0,
            "--slot",
            0,
            "--receive-donor",
            d / "selected",
        )
        pools = field(
            "--side",
            16,
            "--density",
            100,
            "--drift",
            "tidal",
            "--drift-stock",
            student,
            "--drift-pool",
            d / "plain",
            "--drift-rival",
            source,
            "--drift-pool-order",
            "ba",
            "--steps",
            32,
        )
        assert rows(pools, "drift_pools")[0]["swapped"] == "1"

        sight = field(
            "--load",
            source,
            "--drift-sight",
            "own",
            "--contact-remodel",
            "gated",
            "--watch-drift",
            "surprise",
            "--steps",
            256,
            "--drift-witness-sample",
            "calibrated",
            "--save",
            d / "witnessed",
            "--export-mind",
            d / "pupil.mind",
        )
        chosen = rows(sight, "drift_witness")[0]
        assert (
            chosen["drift_witness"] == "surprise"
            and chosen["expected"] != chosen["outlet"]
        )
        call(
            "talk",
            "--speaker",
            d / "pupil.mind",
            "--listener",
            peer,
            "--save-speaker",
            d / "words.mind",
            stdin=(ROOT / "examples/drift-lessons.txt").read_text(),
        )
        visit = field(
            "--load",
            d / "witnessed",
            "--return-lesson",
            d / "pupil.mind",
            d / "words.mind",
            "--follow-witness",
            d / "pupil.mind",
            "--steps",
            64,
            "--drift-diary",
            d / "diary",
            "--drift-questions",
            ROOT / "examples/drift-questions.txt",
            "--save",
            d / "visited",
        )
        counted = audit.check_diary(d / "diary")
        answers = rows(visit, "drift_conversation")[0]
        assert int(answers["events"]) > 0
        for k in (
            "events",
            "expected_right",
            "actual_right",
            "before_right",
            "three_turns",
            "contrasting_pairs",
            "discrepancies",
            "both_sides",
            "erased_topic_answers",
            "missing_scene_answers",
            "unique_replies",
            "locations_right",
        ):
            assert int(answers[k]) == counted[k], (k, answers, counted)
        field(
            "--load",
            d / "witnessed",
            "--return-lesson",
            d / "pupil.mind",
            d / "words.mind",
            "--follow-witness",
            d / "pupil.mind",
            "--drift-diary",
            d / "unasked",
            "--steps",
            64,
            "--save",
            d / "unheard",
        )
        assert (d / "visited").read_bytes() == (d / "unheard").read_bytes()
        unheard = audit.check_diary(d / "unasked")
        assert unheard["events"] == counted["events"] and unheard["anchors"] == 0
        assert "power of two" in field(
            "--drift", "tidal", "--drift-stride", 3, "--steps", 0, ok=False
        )
    print(
        "Drift carbon, neuronal writes, pedigree, currents, material legacies, sampled conversations and exact replay passed."
    )


if __name__ == "__main__":
    main()
