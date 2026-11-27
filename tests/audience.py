"""Exercise recipient-specific teaching, acoustic realignment and public tracers on CUDA."""

import importlib.util
import os
import subprocess
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT

EX = ROOT / "examples"
COAST = ROOT / "docs/observational-learning/coast-beds.txt"
spec = importlib.util.spec_from_file_location(
    "audience", ROOT / "tools/audience-audit.py"
)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def main(work):
    os.environ["AXION_JOURNAL_DIR"] = str(work / "journal")
    home = work / "home"
    with home.open("wb") as f:
        subprocess.run(
            ["zstd", "-qdc", EX / "audience-colony.flood.zst"], stdout=f, check=True
        )
    raw = EX / "audience-guide-english.mind"
    child = EX / "cascade-speaker.mind"
    field = ("floodplain", "--load", home)
    base = (
        *field,
        "--steps",
        0,
        "--audience-guide",
        raw,
        "--audience-pupil",
        child,
        "--audience-trials",
        32,
        "--retina-works",
        COAST,
        "--class-seed",
        317087,
    )
    surface = audit.eye.load_surfaces(COAST)
    examples = {}
    for pairing, arm in (
        ("live", "intact"),
        ("shifted", "intact"),
        ("fixed", "intact"),
        ("live", "exchanged"),
        ("live", "muted"),
        ("live", "private"),
    ):
        key = pairing, arm
        trace = work / f"{pairing}-{arm}.tsv"
        lesson = work / f"{pairing}-{arm}-lesson.tsv"
        log = call(
            *base,
            "--audience-lessons",
            48,
            "--audience-lesson-seed",
            319267,
            "--audience-lesson-trace",
            lesson,
            "--audience-arm",
            arm,
            "--audience-pairing",
            pairing,
            "--audience-trace",
            trace,
        )
        events = audit.lesson(lesson)
        replay = audit.audit(trace, surface)
        score = rows(log, "audience_exchange")[-1]
        teach = rows(log, "audience_practice")[-1]
        assert (
            int(score["caught_after"]) == replay["caught_after"]
            and int(teach["edits"]) == events["edits"]
        )
        examples[key] = (score, teach)
    live, learned = examples["live", "intact"]
    assert (
        int(live["words_after"]) == int(live["portions"])
        and int(live["both_unseen"]) == 32
    )
    for key in (("shifted", "intact"), ("fixed", "intact")):
        score, practice = examples[key]
        assert all(
            learned[k] == practice[k]
            for k in (
                "pictures",
                "sounds",
                "guide_heat",
                "pupil_heat",
                "public_tape",
                "guide_contacts",
                "pupil_contacts",
                "guide_spikes",
                "pupil_spikes",
            )
        )
        assert int(score["words_after"]) < int(live["words_after"])
    for key in (("live", "exchanged"), ("live", "muted"), ("live", "private")):
        score, _ = examples[key]
        assert (
            score["offer_heat"] == live["offer_heat"]
            and score["words_before"] == score["words_after"]
        )
    mixed = work / "mixed.tsv"
    log = call(*base, "--audience-access", "mixed", "--audience-trace", mixed)
    q = rows(log, "audience_exchange")[-1]
    assert [int(q["seen_" + k]) for k in ("neither", "one", "both")] == [16, 32, 16]
    assert audit.audit(mixed, surface)["encounters"] == 64
    packed = rows(call(*base, "--audience-access", "mixed"), "audience_exchange")[-1]
    varying = {"commitment_bytes", "assessment_bytes", "detail_rows", "cuda_ms"}
    assert all(q[k] == v for k, v in packed.items() if k not in varying)
    assert int(packed["commitment_bytes"]) == 8 and int(packed["detail_rows"]) == 2
    young = work / "b"
    split = work / "c"
    full = work / "d"
    grains = work / "grains.tsv"
    args = (
        "--drift-contest",
        "sham",
        "--drift-label",
        773,
        "--daylight-audience",
        "reverse-lineages",
    )
    call(*field, *args, "--steps", 61, "--save", full)
    call(*field, *args, "--steps", 29, "--save", young)
    log = call(
        "floodplain",
        "--load",
        young,
        "--steps",
        32,
        "--save",
        split,
        "--probe-trace",
        grains,
    )
    assert (
        full.read_bytes() == split.read_bytes()
        and int(rows(log, "field_audience")[-1]["visible_neighbours"]) > 0
    )
    assert audit.eye.audit_grains(grains)["public_choices"] > 0
    calibrated = EX / "audience-pupil.mind"
    guide = EX / "audience-guide.mind"
    for script in ("audience-conversation.txt", "audience-other-pupil.txt"):
        log = call(
            "talk",
            "--speaker",
            guide,
            "--listener",
            calibrated,
            stdin=(EX / script).read_text(),
        )
        assert int(rows(log, "public_offer")[-1]["calibrated"]) == 1
        assert int(rows(log, "audience_delivery")[-1]["pupil_ate"]) == 8
        assert len([s for s in log.splitlines() if s.startswith("pupil> ")]) >= 3
    pupil_home = work / "pupil-home"
    with pupil_home.open("wb") as f:
        subprocess.run(
            ["zstd", "-qdc", EX / "cascade-colony.flood.zst"], stdout=f, check=True
        )
    lesson = (
        "--steps",
        0,
        "--return-lesson",
        child,
        calibrated,
        "--follow-witness",
        child,
    )
    returned = work / "returned"
    log = call(
        "floodplain",
        "--load",
        pupil_home,
        *lesson,
        "--save",
        returned,
        "--export-mind",
        work / "returned.mind",
    )
    assert int(rows(log, "field_acoustic_alignment")[-1]["edits"]) == 56
    retry = work / "retry"
    log = call(
        "floodplain",
        "--load",
        returned,
        *lesson,
        "--save",
        retry,
        "--export-mind",
        work / "again.mind",
    )
    assert (
        int(rows(log, "field_acoustic_alignment")[-1]["edits"]) == 0
        and returned.read_bytes() == retry.read_bytes()
    )
    partial = work / "partial.mind"
    transcript = (
        (EX / "audience-lessons.txt")
        .read_text()
        .replace("319267 48 live", "319267 2 live")
    )
    call(
        "talk",
        "--speaker",
        raw,
        "--listener",
        child,
        "--save-listener",
        partial,
        stdin=transcript,
    )
    conflict = call(
        "floodplain",
        "--load",
        returned,
        "--steps",
        0,
        "--return-lesson",
        child,
        partial,
        ok=False,
    )
    assert "acoustic contacts changed" in conflict
    voice_rows = []
    for substitute in (None, EX / "audience-second-guide.mind"):
        f = work / ("own.tsv" if substitute is None else "substitute.tsv")
        log = call(
            *field,
            "--steps",
            0,
            "--audience-guide",
            guide,
            "--audience-pupil",
            EX / "audience-two-voices.mind",
            "--audience-trials",
            32,
            "--class-seed",
            642791,
            "--audience-trace",
            f,
            *(("--audience-timbre", substitute) if substitute else ()),
        )
        assert rows(log, "audience_exchange")[-1]["calibrated"] == "1"
        voice_rows.append(list(audit.records(f)))
    assert all(
        all(
            a[k] == b[k]
            for k in (
                "first_wave",
                "second_wave",
                "work",
                "picked",
                "before",
                "starting_spikes",
            )
        )
        for a, b in zip(*voice_rows)
    )
    assert sum(r["after_words"] for r in voice_rows[0]) > sum(
        r["after_words"] for r in voice_rows[1]
    )
    call(*base, "--audience-lessons", 3, ok=False)
    call(*field, "--steps", 0, "--audience-trials", 8, ok=False)


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="axion-audience-test-") as temp:
        main(Path(temp))
    print(
        "Recipient-specific contact, fixed-work controls, lexical persistence and watershed replay passed."
    )
