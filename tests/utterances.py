"""Trace a real watershed birth and test its learned speech on CUDA."""

import argparse
import importlib.util
import os
import subprocess
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT

EX = ROOT / "examples"
spec = importlib.util.spec_from_file_location(
    "utterance_audit", ROOT / "tools/utterance-audit.py"
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def decompress(source, target):
    with target.open("wb") as output:
        subprocess.run(["zstd", "-qdc", source], stdout=output, check=True)


def main(work):
    os.environ["AXION_JOURNAL_DIR"] = str(work / "journal")
    base, habitat, young = (
        work / "origin.flood",
        work / "live.flood",
        work / "young.mind",
    )
    continuous = work / "life.tsv"
    decompress(EX / "audience-colony.flood.zst", base)
    field = ("floodplain", "--load", base, "--daylight-ink", "chorus")
    extract = ("--slot", 8749, "--export-mind", young)
    log = call(
        *field,
        "--steps",
        4096,
        "--report-every",
        1024,
        "--save",
        habitat,
        *extract,
        "--ink-follow",
        8749,
        "--ink-events",
        continuous,
    )
    observations, lives = audit.field(continuous, follow=True)
    assert (
        len(lives) > 1 and observations["complete"] and lives[8749, 11577][0] == 12271
    )
    assert young.read_bytes() == (EX / "brook-young.mind").read_bytes()
    ledger = rows(log, "field_ink")[-1]
    assert ledger["imitating"] == "0" and ledger["learned_calls"] == "0"
    assert int(ledger["vocal_heat"]) == int(ledger["syllables"]) > 0
    assert 0 < int(ledger["blind_food"]) <= int(ledger["food"])
    split, match = work / "split.flood", work / "match.flood"
    call(*field, "--steps", 1987, "--save", split)
    call("floodplain", "--load", split, "--steps", 2109, "--save", match)
    assert match.read_bytes() == habitat.read_bytes()
    enroll = ("floodplain", "--load", habitat, "--steps", 0)
    course = (
        "--story-pupil",
        young,
        "--story-trials",
        64,
        "--story-length",
        7,
        "--class-seed",
        49747,
    )
    frozen_cohort = call(
        *enroll,
        "--story-residents",
        32,
        "--story-exposure",
        "fixed",
        "--story-trials",
        8,
    )
    assert int(rows(frozen_cohort, "narrative_cohort")[-1]["total_correct"]) == 0
    taught = work / "schooled.mind"
    traces = {}
    for arm, args in (
        ("paired", ()),
        ("heard-other", ("--story-reference-input", "other")),
        ("mispaired", ("--story-reference-pair", "crossed")),
        ("contrary", ("--story-reference-language", "swapped")),
        ("shutdown", ("--story-gate", "shut")),
    ):
        words, questions = work / (arm + "-words.tsv"), work / (arm + "-questions.tsv")
        extras = ("--story-save", taught) if arm == "paired" else ()
        text = call(
            *enroll,
            *course,
            "--story-references",
            64,
            "--story-reference-trials",
            64,
            "--story-reference-trace",
            questions,
            "--story-trace",
            words,
            *args,
            *extras,
        )
        q, e = rows(text, "narrative_references")[-1], rows(text, "narrative_exam")[-1]
        data = audit.references(questions, switched=arm == "contrary")
        assert data["exact"] == int(q["exact"]) and data["located"] == int(q["located"])
        assert audit.reports(words)["exact"] == int(e["exact"])
        traces[arm] = q
        if arm == "paired":
            assert (
                int(e["exact"]) == 512
                and int(e["recovered_histories"]) == 128
                and int(e["settled"]) == 0
            )
            assert (
                0 < int(q["absent"])
                and q["invented"] == "0"
                and int(q["located"]) == int(q["answerable"])
            )
            assert int(q["contrasts"]) > 10 and q["both"] == q["contrasts"]
        if arm == "contrary":
            assert 0 < int(q["located"]) == int(q["answerable"])
    assert all(
        traces[k]["located"] == "0" for k in ("heard-other", "mispaired", "shutdown")
    )
    assert (
        traces["paired"]["heat"]
        == traces["mispaired"]["heat"]
        == traces["heard-other"]["heat"]
    )
    assert traces["shutdown"]["edits"] == "0"
    returned, port = work / "returned.mind", work / "school-home.flood"
    call(
        *enroll,
        "--return-lesson",
        young,
        taught,
        "--follow-witness",
        young,
        "--export-mind",
        returned,
        "--save",
        port,
    )
    reload = work / "reopened.mind"
    call(
        "floodplain",
        "--load",
        port,
        "--steps",
        0,
        "--slot",
        8749,
        "--export-mind",
        reload,
    )
    assert (
        reload.read_bytes()
        == returned.read_bytes()
        == (EX / "brook-witness.mind").read_bytes()
    )
    dialogue = call(
        "talk",
        "--speaker",
        returned,
        "--listener",
        EX / "brook-pupil.mind",
        stdin=(EX / "brook-conversation.txt").read_text(),
    )
    (work / "conversation.txt").write_text(dialogue)
    assert (
        audit.field_conversation(continuous, work / "conversation.txt", 8749, 11577)[
            "account_words"
        ]
        == 68
    )
    hearing = call(
        "talk",
        "--speaker",
        returned,
        "--listener",
        EX / "brook-pupil.mind",
        stdin=(EX / "brook-hearing.txt").read_text(),
    )
    turns = rows(hearing, "narrative_turn")
    assert (
        "pupil> ?" in hearing
        and int(turns[-1]["bite"]) == 12
        and turns[-1]["eaten"] == "12"
    )
    assert all(
        sum(int(r[k]) for k in ("food_stock", "held", "settled", "eaten")) == 64
        and int(r["grain_stock"]) == 64
        for r in turns
    )
    peer = work / "peer.mind"
    peer_log = call(
        *enroll,
        "--story-pupil",
        EX / "brook-pupil-young.mind",
        "--story-mentor",
        returned,
        "--class-seed",
        616451,
        "--story-trials",
        32,
        "--story-length",
        7,
        "--story-references",
        80,
        "--story-reference-trials",
        64,
        "--story-save",
        peer,
    )
    q = rows(peer_log, "narrative_references")[-1]
    assert q["located"] == q["answerable"] and int(q["located"]) > 128
    assert peer.read_bytes() == (EX / "brook-pupil.mind").read_bytes()
    steady = (
        "--story-pupil",
        returned,
        "--story-trials",
        32,
        "--story-length",
        7,
        "--story-singles",
        0,
        "--story-pairs",
        0,
    )
    paths = []
    for model in ("plastic", "packed", "table"):
        data = work / (model + ".tsv")
        extra = ("--story-benchmark", 8192) if model == "packed" else ()
        runs = call(
            *enroll, *steady, "--story-trace", data, "--story-motor", model, *extra
        )
        paths.append(data.read_bytes())
        if extra:
            speed = rows(runs, "narrative_speed")
            assert len(speed) == 3 and all(
                q["matched"] == "8192" and float(q["median_ms"]) > 0 for q in speed
            )
            assert int(speed[1]["image_bytes"]) < int(speed[0]["image_bytes"])
    assert paths[0] == paths[1] == paths[2]
    coast = ROOT / "docs/observational-learning/coast-beds.txt"
    natural = work / "captured.tsv"
    call(*enroll, *steady, "--retina-works", coast, "--story-trace", natural)
    assert audit.reports(natural, audit.eye.load_surfaces(coast))["exact"] == 256
    # Vocal imitation must leave the separately controlled somatic auditory route alone.
    snapshots = []
    for setting in ("chorus", "dialects"):
        image = work / (setting + "-shared-cortex.mind")
        call(
            *enroll,
            "--daylight-ink",
            setting,
            "--voice-route",
            "shared",
            "--slot",
            8749,
            "--export-mind",
            image,
        )
        snapshots.append(image.read_bytes())
    assert snapshots[0] == snapshots[1]
    acoustic = work / "native-voices.tsv"
    spoken = work / "dialects.flood"
    emerge = call(
        "floodplain",
        "--load",
        base,
        "--steps",
        512,
        "--daylight-ink",
        "dialects",
        "--ink-trace",
        acoustic,
        "--save",
        spoken,
    )
    vocal = rows(emerge, "field_ink")[-1]
    assert vocal["imitating"] == "1"
    assert 0 < int(vocal["learned_calls"]) <= int(vocal["calls"])
    natural, _ = audit.field(acoustic)
    assert natural["borrowed"] > 0
    direct = work / "dialects-direct.flood"
    split = work / "dialects-resumed.flood"
    call(
        "floodplain",
        "--load",
        base,
        "--steps",
        768,
        "--daylight-ink",
        "dialects",
        "--save",
        direct,
    )
    call("floodplain", "--load", spoken, "--steps", 256, "--save", split)
    assert direct.read_bytes() == split.read_bytes()
    arms = {}
    for arm in ("paired", "crossed", "cut"):
        spoken = call(
            *enroll,
            "--story-pupil",
            young,
            "--story-trials",
            32,
            "--story-language",
            "native",
            "--story-singles",
            0,
            "--story-pairs",
            0,
            "--call-transmissions",
            4,
            "--call-practice",
            64,
            "--call-exposure",
            arm,
            "--class-seed",
            104729,
            "--call-trace",
            work / (arm + "-dialects.tsv"),
        )
        arms[arm] = rows(spoken, "acquired_voice")
    last = arms["paired"][-1]
    assert int(last["pupils_differ"]) >= int(last["images"]) * 3 // 4
    assert int(last["own_catch"]) > int(last["other_catch"]) * 3
    assert (
        int(last["matched_scene_food"]) > 3 * int(last["swapped_scene_food"])
        and int(last["changed_with_scene"]) > 0
    )
    assert last["matched_portions"] == last["portions"]
    assert all(
        r["pupils_differ"] == "0" and r["altered_syllables"] == "0" for r in arms["cut"]
    )
    assert (
        arms["paired"][0]["sounds"]
        == arms["crossed"][0]["sounds"]
        == arms["cut"][0]["sounds"]
    )
    call(
        *enroll,
        *steady,
        "--story-ration",
        0,
        "--story-reference-trials",
        2,
        "--story-reference-trace",
        work / "collision",
        "--story-trace",
        work / "collision",
        ok=False,
    )
    call(*enroll, "--ink-follow", 8749, ok=False)
    # A failed checksum must never leak a partially restored pupil to the caller.
    damaged = work / "damaged.mind"
    raw = bytearray(taught.read_bytes())
    raw[-23] ^= 1
    damaged.write_bytes(raw)
    call(*enroll, "--story-pupil", damaged, "--story-trials", 1, ok=False)
    print(
        "Passed watershed replay, birth identity, paid hearing, temporal questions, live teaching, classroom controls and packed CUDA inference."
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--keep", type=Path)
    options = parser.parse_args()
    if options.keep:
        options.keep.mkdir(parents=True, exist_ok=True)
        main(options.keep)
    else:
        with tempfile.TemporaryDirectory(
            prefix="axion-utterances-", dir=os.environ.get("TMPDIR", "/var/tmp")
        ) as work:
            main(Path(work))
