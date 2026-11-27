"""Check that a wild individual's rejoined language describes the remembered field, including after another lesson."""

import csv
import os
import struct
import tempfile
from pathlib import Path

from press import call, rows
from worksites import ROOT, snapshot

TEACHER = ROOT / "examples/parallax-reader.mind"
PEER = ROOT / "examples/parallax-witness.mind"
MARK = struct.pack("<2I", 0x4C4C4554, 576)
NAMES = ("amber", "jade", "plum", "teal")
PLACES = ("north", "east", "south", "west")


def narration(path):
    raw = path.read_bytes()
    at = raw.rfind(MARK)
    assert at >= 568
    return raw[at - 568 : at]


def evidence(path):
    with path.open() as f:
        return list(csv.DictReader(f, delimiter="\t"))


def check_reports(data):
    for r in data:
        image, known, mask, replies = (
            int(r[k]) for k in ("images", "known", "answer_mask", "answer_codes")
        )
        assert not (mask & ~(known & 255))
        offered = (known & 255).bit_count()
        correct = changed = both = 0
        for j in range(8):
            if mask & (1 << j):
                correct += 4 + ((image >> (2 * j)) & 3) == ((replies >> (3 * j)) & 7)
        for j in range(4):
            flags = 17 << j
            if (known & flags) == flags and ((image >> (2 * j)) & 3) != (
                (image >> (8 + 2 * j)) & 3
            ):
                changed += 1
                both += (
                    (mask & flags) == flags
                    and ((replies >> (3 * j)) & 7) == 4 + ((image >> (2 * j)) & 3)
                    and ((replies >> (12 + 3 * j)) & 7)
                    == 4 + ((image >> (8 + 2 * j)) & 3)
                )
        assert (offered, correct, changed, both, 0) == tuple(
            int(r[k])
            for k in ("offered", "correct", "changed", "both", "empty_answers")
        )


def main():
    with tempfile.TemporaryDirectory(
        prefix="axion-field-voices-", dir=os.environ.get("TMPDIR", "/var/tmp")
    ) as name:
        d = Path(name)
        os.environ["AXION_JOURNAL_DIR"] = str(d / "journal")

        def field(*args, **kw):
            return call("floodplain", *args, **kw)

        def talk(who, script, *args):
            return call(
                "talk", "--speaker", who, "--listener", PEER, *args, stdin=script
            )

        field0 = d / "field.flood"
        start = d / "resident.mind"
        one = d / "one.mind"
        other = d / "other.mind"
        back = d / "back.mind"
        baseline = field(
            "--resident",
            TEACHER,
            "--side",
            16,
            "--density",
            0,
            "--steps",
            0,
            "--export-mind",
            start,
            "--save",
            field0,
        )
        slot = int(rows(baseline, "rehoused_resident")[0]["rehoused_resident"])
        before = narration(start)
        src_images, src_known, src_stamp = struct.unpack_from("<3I", before, 548)
        assert src_stamp and src_known
        for path, pattern in ((one, "what occupies *"), (other, "what rested in *")):
            script = (
                "/braid 137\n/braid-gloss speaker 8 5003 north east south west amber jade plum teal\n"
                + f"/braid-pattern speaker actual explicit port 64 29373 {pattern}\n/quit\n"
            )
            talk(start, script, "--save-speaker", path)
        after = narration(one)
        assert after[:548] != before[:548] and after[548:560] != before[548:560]
        taught = d / "taught.flood"
        again = d / "again.flood"
        home = field(
            "--load",
            field0,
            "--return-lesson",
            start,
            one,
            "--follow-witness",
            start,
            "--steps",
            0,
            "--save",
            taught,
            "--export-mind",
            back,
        )
        r = rows(home, "field_lesson")[0]
        assert int(r["changed_bits"]) > 0 and int(r["field_stamp"]) == src_stamp
        live = narration(back)
        assert live[:548] == after[:548] and live[548:560] == before[548:560]
        a, b = snapshot(field0), snapshot(taught)
        old = a["narrators"][slot]
        new = b["narrators"][slot]
        assert old[:548] != new[:548] and old[548:] == new[548:]
        ar = field0.read_bytes()
        br = taught.read_bytes()
        assert ar[:-8].count(old) == 1 and ar[:-8].replace(old, new) == br[:-8]
        same = field(
            "--load",
            taught,
            "--steps",
            0,
            "--return-lesson",
            start,
            one,
            "--save",
            again,
        )
        assert (
            rows(same, "field_lesson")[0]["changed_bits"] == "0"
            and again.read_bytes() == br
        )
        assert "since the supplied baseline" in field(
            "--load", taught, "--steps", 0, "--return-lesson", start, other, ok=False
        )
        assert "witness" in field(
            "--load", taught, "--steps", 0, "--return-lesson", start, PEER, ok=False
        )
        spoken = talk(
            back, "".join(f"what occupies {place}?\n" for place in PLACES) + "/quit\n"
        )
        utterances = [
            line.removeprefix("speaker> ").strip()
            for line in spoken.splitlines()
            if line.startswith("speaker> ")
        ]
        expected = [
            NAMES[(src_images >> (8 + 2 * j)) & 3]
            for j in range(4)
            if src_known & (16 << j)
        ]
        assert expected and all(w in utterances for w in expected), (
            expected,
            utterances,
        )
        questions = "".join(f"what occupies {place}?\n" for place in PLACES)
        direct = field(
            "--load",
            taught,
            "--steps",
            0,
            "--follow-witness",
            start,
            "--field-questions",
            "-",
            stdin=questions + "/quit\n",
        )
        public = [
            line.removeprefix("witness> ").strip()
            for line in direct.splitlines()
            if line.startswith("witness> ")
        ]
        isolated = talk(back, "/braid-replies mute\n" + questions + "/quit\n")
        single = [
            line.removeprefix("speaker> ").strip()
            for line in isolated.splitlines()
            if line.startswith("speaker> ")
        ]
        speaker_flags = [int(r["answered"]) for r in rows(isolated, "braid_recall")]
        field_flags = [int(r["answered"]) for r in rows(direct, "field_utterance")]
        assert speaker_flags == field_flags and len(field_flags) == 4
        assert [w for w in public if w != "?"] == [w for w in single if w != "?"]
        assert public.count("?") > utterances.count("?")
        assert int(rows(direct, "field_interview")[0]["stamp"]) == src_stamp
        untrained = field(
            "--load",
            field0,
            "--steps",
            0,
            "--follow-witness",
            start,
            "--field-questions",
            "-",
            stdin="what occupies north?\n/quit\n",
        )
        assert rows(untrained, "field_utterance")[0]["answered"] == "0"
        trace = d / "class.tsv"
        f = field(
            "--load",
            field0,
            "--steps",
            32,
            "--watch-readers",
            8,
            "--field-class",
            256,
            "--class-seed",
            82751,
            "--class-trace",
            trace,
        )
        result = rows(f, "field_watch")[-1]
        assert result["field_watch"] == "qualified" and int(result["slot"]) == slot
        assert int(result["samples"]) == 1 and int(result["tick"]) == int(
            rows(baseline, "flood_tick")[-1]["flood_tick"]
        )
        table = evidence(trace)
        check_reports(table)
        assert len(table) == 1 and table[0]["diagnostic"] == "8"
        fresh = d / "new-dialects.tsv"
        rd = field(
            "--load",
            field0,
            "--steps",
            0,
            "--slot",
            slot,
            "--field-retest",
            32,
            "--class-seed",
            53,
            "--class-history",
            82751,
            "--class-practice",
            12,
            "--class-trace",
            fresh,
        )
        trials = evidence(fresh)
        check_reports(trials)
        assert len(trials) == len({v["dialect_key"] for v in trials}) == 32
        assert {v["course_key"] for v in trials} == {"82751"} and rows(
            rd, "field_class"
        )[0]["individual"] == "1"
        departed = d / "departed.flood"
        missing = d / "no-witness.mind"
        end = field(
            "--load",
            taught,
            "--steps",
            256,
            "--follow-witness",
            start,
            "--export-mind",
            missing,
            "--save",
            departed,
        )
        assert (
            rows(end, "field_follow")[-1]["field_follow"] == "lost"
            and not missing.exists()
            and departed.exists()
        )
        print(
            "Verified field identity, conflict and retry handling, exact preservation of wild and classroom episodes, visible English reports and independent CUDA reader qualification."
        )


if __name__ == "__main__":
    main()
