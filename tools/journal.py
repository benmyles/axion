#!/usr/bin/env python3
"""Render Axion Genesis observations and publish immutable WebP observatory plates."""
import base64
import ctypes as C
import datetime as dt
import fcntl
import gzip
import hashlib
import html
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import textwrap
import zlib

W, H = 2200, 1320
PAPER = "#081013"
INK = "#eef5f2"
MUTED = "#658380"
LINE = "#1f3941"
SANS = "Adwaita Sans,DejaVu Sans,sans-serif"
MONO = "JetBrains Mono,DejaVu Sans Mono,monospace"
SERIF = "Noto Serif,DejaVu Serif,serif"
ROOT = None
BINARY = None


def esc(s):
    return html.escape(str(s), quote=True)


def short(s, n=42):
    return s if len(s) <= n else s[: n - 1].rstrip() + "…"


def human(n):
    if isinstance(n, str):
        return n
    if isinstance(n, float) and abs(n) < 1000 and not n.is_integer():
        return f"{n:,.2f}".rstrip("0").rstrip(".")
    return f"{n:,.0f}" if abs(n) < 1e7 else f"{n/1e9:.2f}B" if abs(n) >= 1e9 else f"{n/1e6:.1f}M"


def blend(a, b, f):
    return "#" + "".join(f"{round(int(a[i:i+2],16)*(1-f)+int(b[i:i+2],16)*f):02x}" for i in (1, 3, 5))


def utc(timestamp=None):
    instant = dt.datetime.now(dt.timezone.utc) if timestamp is None else dt.datetime.fromtimestamp(timestamp, dt.timezone.utc)
    return instant.isoformat(timespec="seconds").replace("+00:00", "Z")


def pairs(line):
    return dict(re.findall(r'([^\s=]+)=("[^"]*"|\S+)', line))


class Drawing:
    def __init__(self, width=W, height=H):
        self.width = width
        self.height = height
        self.parts = [
            f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
            '<defs><linearGradient id="wash" x1="0" y1="0" x2="0" y2="1"><stop stop-color="#3ee6a8" stop-opacity=".35"/><stop offset="1" stop-color="#3ee6a8" stop-opacity=".02"/></linearGradient></defs>',
        ]
        self.rect(0, 0, width, height, PAPER)

    def add(self, s):
        self.parts.append(s)

    def rect(self, x, y, w, h, fill, r=0, extra=""):
        self.add(f'<rect x="{x:.2f}" y="{y:.2f}" width="{max(0,w):.2f}" height="{max(0,h):.2f}" rx="{r}" fill="{fill}" {extra}/>')

    def text(self, x, y, s, size=18, color=INK, font=SANS, weight=400, anchor="start", space=0):
        self.add(
            f'<text x="{x:.2f}" y="{y:.2f}" font-family="{font}" font-size="{size}" font-weight="{weight}" '
            f'text-anchor="{anchor}" letter-spacing="{space}" fill="{color}">{esc(s)}</text>'
        )

    def label(self, x, y, s, color=MUTED):
        self.text(x, y, s.upper(), 13, color, MONO, 500, space=1.1)

    def path(self, d, stroke, width=1, fill="none", extra=""):
        self.add(f'<path d="{d}" stroke="{stroke}" stroke-width="{width}" fill="{fill}" stroke-linecap="round" stroke-linejoin="round" {extra}/>')

    def dot(self, x, y, r, fill, extra=""):
        self.add(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="{r:.2f}" fill="{fill}" {extra}/>')

    def rule(self, x, y, x2, y2, color=LINE, width=1):
        self.path(f"M{x} {y} L{x2} {y2}", color, width)

    def finish(self):
        return "".join(self.parts) + "</svg>"


class Rect(C.Structure):
    _fields_ = [("x", C.c_double), ("y", C.c_double), ("width", C.c_double), ("height", C.c_double)]


class GError(C.Structure):
    _fields_ = [("domain", C.c_uint), ("code", C.c_int), ("message", C.c_char_p)]


class Codec:
    """Use system librsvg, Cairo, GLib and libwebp via their stable C ABIs."""

    def __init__(self):
        try:
            r = C.CDLL("librsvg-2.so.2")
            c = C.CDLL("libcairo.so.2")
            g = C.CDLL("libgobject-2.0.so.0")
            w = C.CDLL("libwebp.so.7")
            glib = C.CDLL("libglib-2.0.so.0")
        except OSError as e:
            raise RuntimeError(f"WebP journal needs librsvg, Cairo, GLib and libwebp: {e}") from e
        ptr = C.c_void_p
        uint = C.c_size_t
        integer = C.c_int
        err = C.POINTER(C.POINTER(GError))

        def fn(lib, name, ret, *args):
            f = getattr(lib, name)
            f.restype = ret
            f.argtypes = list(args)
            return f

        self.handle = fn(r, "rsvg_handle_new_from_data", ptr, C.c_char_p, uint, err)
        self.render = fn(r, "rsvg_handle_render_document", integer, ptr, ptr, C.POINTER(Rect), err)
        self.unref = fn(g, "g_object_unref", None, ptr)
        self.error_free = fn(glib, "g_error_free", None, C.POINTER(GError))
        self.surface = fn(c, "cairo_image_surface_create", ptr, integer, integer, integer)
        self.context = fn(c, "cairo_create", ptr, ptr)
        self.destroy = fn(c, "cairo_destroy", None, ptr)
        self.flush = fn(c, "cairo_surface_flush", None, ptr)
        self.drop = fn(c, "cairo_surface_destroy", None, ptr)
        self.data = fn(c, "cairo_image_surface_get_data", ptr, ptr)
        self.stride = fn(c, "cairo_image_surface_get_stride", integer, ptr)
        self.encode = fn(w, "WebPEncodeBGRA", uint, ptr, integer, integer, integer, C.c_float, C.POINTER(ptr))
        self.free = fn(w, "WebPFree", None, ptr)

    def write(self, drawing, path):
        data = drawing.finish().encode()
        error = C.POINTER(GError)()
        handle = self.handle(data, len(data), C.byref(error))

        def reason():
            if not error:
                return "unknown vector rendering failure"
            msg = error.contents.message.decode(errors="replace")
            self.error_free(error)
            return msg

        if not handle:
            raise RuntimeError(reason())
        width, height = drawing.width, drawing.height
        image = self.surface(0, width, height)
        ctx = self.context(image)
        output = C.c_void_p()
        try:
            if not self.render(handle, ctx, C.byref(Rect(0, 0, width, height)), C.byref(error)):
                raise RuntimeError(reason())
            self.flush(image)
            pixels = self.data(image)
            stride = self.stride(image)
            if sys.byteorder != "little":
                original = C.string_at(pixels, stride * height)
                pixels = C.create_string_buffer(b"".join(original[i : i + 4][::-1] for i in range(0, len(original), 4)))
            n = self.encode(pixels, width, height, stride, 88, C.byref(output))
            if not n:
                raise RuntimeError("WebP encoding failed")
            with open(path, "wb") as f:
                f.write(C.string_at(output, n))
        finally:
            if output:
                self.free(output)
            self.destroy(ctx)
            self.drop(image)
            self.unref(handle)


def png(size, pixels):
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    stride = 3 * size
    data = b"".join(b"\0" + pixels[i : i + stride] for i in range(0, len(pixels), stride))
    data = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(data, 5))
        + chunk(b"IEND", b"")
    )
    return base64.b64encode(data).decode()


def trend(d, x, y, w, h, values, color="#3ee6a8", fill=False, times=None, limits=None):
    if not values:
        return
    lo, hi = limits or (min(values), max(values))
    if lo == hi:
        lo -= max(1, abs(lo) * 0.05)
        hi += max(1, abs(hi) * 0.05)
    times = times or list(range(len(values)))
    pts = [(x + w * (t - times[0]) / max(1, times[-1] - times[0]), y + h - (v - lo) / (hi - lo) * h) for t, v in zip(times, values)]
    path = "M" + " L".join(f"{a:.1f} {b:.1f}" for a, b in pts)
    if fill and len(pts) > 1:
        d.path(path + f" L{x+w} {y+h} L{x} {y+h}Z", "none", 0, "url(#wash)")
    d.path(path, color, 2.9)
    d.dot(*pts[-1], 4, color)
    if len(pts) == 1:
        d.dot(*pts[0], 6, "none", f'stroke="{color}" stroke-width="2"')


def brief(data):
    title, subtitle, family, measure, unit, stats, narrative, bullets = genesis_summary(data)
    return {
        "command": data.get("command", "genesis"),
        "kind": "genesis",
        "archive": data.get("archive", ""),
        "source": data.get("source", ""),
        "device": data.get("device", ""),
        "title": title,
        "subtitle": subtitle,
        "family": family,
        "measure": measure,
        "unit": unit,
        "stats": stats,
        "narrative": narrative,
        "bullets": bullets,
    }


def atomic(path, write):
    name = None
    try:
        with tempfile.NamedTemporaryFile(prefix=".write-", dir=path.parent, delete=False) as fd:
            name = Path(fd.name)
        write(name)
        os.replace(name, path)
        name = None
    finally:
        if name and name.exists():
            name.unlink()


def read_history(root):
    runs = root / "runs"
    runs.mkdir(parents=True, exist_ok=True)
    return [json.loads(p.read_text()) for p in sorted(runs.glob("[0-9]*-*/summary.json"))]


def md(s):
    return re.sub(r"([\\\[\]*_\x60<>|])", r"\\\1", " ".join(str(s).splitlines()))


def markdown(history):
    lines = [
        "# Axion Genesis Evolution Journal",
        "",
        "[Latest run](latest.webp) · [Bench](genesis-bench.webp) · [Evolve](genesis-evolve.webp) · [Probe](genesis-probe.webp) · [Converse](genesis-converse.webp)",
        "",
        f"{len(history):,} recorded Genesis runs.",
        "",
    ]
    for b in reversed(history):
        lines.extend(
            [
                f"## {b['id']:05d} · {md(b['title'])}",
                "",
                f"Completed {b['date']} · [WebP plate]({b['path']}/plate.webp) · [Summary]({b['path']}/summary.json) · [Observations]({b['path']}/observations.json.gz)",
                "",
            ]
        )
        for bullet in b.get("bullets", []):
            lines.append("- " + md(bullet))
        lines.extend(["", " ".join(md(x) for x in b.get("narrative", [])), ""])
    return "\n".join(lines).rstrip("\n") + "\n"


def refresh(root, history, codec, repair=False):
    if repair:
        for b in history:
            obs_p = root / b["path"] / "observations.json.gz"
            if obs_p.is_file():
                with gzip.open(obs_p, "rt", encoding="utf-8") as f:
                    raw = json.load(f)
                nb = brief(raw)
                b.update(nb)
                atomic(root / b["path"] / "plate.webp", lambda p: codec.write(genesis_plate_drawing(raw, b), p))
                atomic(root / b["path"] / "summary.json", lambda p: p.write_text(json.dumps(b, ensure_ascii=False, indent=2) + "\n"))
    atomic(root / "JOURNAL.md", lambda p: p.write_text(markdown(history)))
    if history:
        newest = history[-1]
        latest = root / newest["path"] / "plate.webp"
        if latest.is_file():
            atomic(root / "latest.webp", lambda p: shutil.copyfile(latest, p))
            gmode = newest.get("genesis_mode")
            if gmode:
                atomic(root / f"genesis-{gmode}.webp", lambda p: shutil.copyfile(latest, p))


def store(root, data, history, codec):
    b = brief(data)
    identifier = max((x["id"] for x in history), default=0) + 1
    rel = f"runs/{identifier:08d}-genesis"
    b.update(id=identifier, path=rel, date=data.get("recorded_at") or utc(), version=1)
    glines = data.get("lines", [])
    if any(l.startswith("genesis_summary=") for l in glines):
        b["genesis_mode"] = "evolve"
    elif any(l.startswith("genesis_probe=") for l in glines):
        b["genesis_mode"] = "probe"
    elif any(l.startswith("genesis_converse=") for l in glines):
        b["genesis_mode"] = "converse"
    elif any(l.startswith("genesis_inspect=") for l in glines):
        b["genesis_mode"] = "inspect"
    elif any(l.startswith("genesis_bench=") for l in glines):
        b["genesis_mode"] = "bench"
    staged = Path(tempfile.mkdtemp(prefix=".pending-", dir=root))
    try:
        codec.write(genesis_plate_drawing(data, b), staged / "plate.webp")
        (staged / "summary.json").write_text(json.dumps(b, ensure_ascii=False, indent=2) + "\n")
        with gzip.open(staged / "observations.json.gz", "wt", encoding="utf-8", compresslevel=5) as f:
            json.dump(data, f, ensure_ascii=False, separators=(",", ":"))
        os.replace(staged, root / rel)
    finally:
        if staged.exists():
            shutil.rmtree(staged)
    history.append(b)
    return b


def receipt_id(name, stat, raw):
    identity = f"{name}/{stat.st_mtime_ns}/{stat.st_ino}".encode()
    return hashlib.sha256(identity + b"\0" + raw).hexdigest()


def queued(root, own=None):
    root.mkdir(parents=True, exist_ok=True)
    with (root / ".lock").open("a+") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        history = read_history(root)
        codec = Codec()
        known = {r.get("receipt") for r in history}
        processed = []
        published = []
        receipts = sorted(root.glob(".receipt-*"), key=lambda p: (p.stat().st_mtime_ns, p.name))
        for path in receipts:
            raw = path.read_bytes()
            stat = path.stat()
            receipt = receipt_id(path.name, stat, raw)
            if receipt not in known:
                data = json.loads(raw)
                data["receipt"] = receipt
                if not re.fullmatch(r"\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ", str(data.get("recorded_at", ""))):
                    data["recorded_at"] = utc(stat.st_mtime)
                entry = store(root, data, history, codec)
                known.add(receipt)
                if entry:
                    published.append(entry)
            processed.append(path)
        if processed or own is None:
            refresh(root, history, codec, repair=(own is None))
        for path in processed:
            path.unlink()
        if own is not None:
            published = [r for r in history if r.get("receipt") == own]
        for entry in published:
            print(f"Run {entry['id']:05d} · {root/entry['path']/'plate.webp'}", file=sys.stderr)
    return len(history)


def main():
    global ROOT, BINARY
    if len(sys.argv) < 3:
        raise ValueError("usage: journal.py JOURNAL_DIR AXION_BINARY [record|rebuild]")
    ROOT = Path(sys.argv[1]).resolve()
    BINARY = str(Path(sys.argv[2]).resolve())
    args = sys.argv[3:]
    mode = args[0] if args else "rebuild"
    if mode in ("--help", "help"):
        print("axion journal rebuild [--journal-dir DIR]")
        return
    if mode == "record":
        own = None
        if len(args) == 3:
            with os.fdopen(int(args[2]), "rb") as src:
                own = receipt_id(args[1], os.fstat(src.fileno()), src.read())
        elif len(args) != 1:
            raise ValueError("invalid journal receipt arguments")
        queued(ROOT, own)
        return
    if mode != "rebuild":
        raise ValueError("journal accepts rebuild")
    print(f"Rebuilt {queued(ROOT)} journal records in {ROOT}.")


if "genesis_summary" not in globals():
    exec(compile(Path(__file__).with_name("genesis_journal.py").read_bytes(), "genesis_journal.py", "exec"))

if __name__ == "__main__":
    try:
        main()
    except (ValueError, RuntimeError, OSError, subprocess.CalledProcessError) as exc:
        print(f"axion journal: {exc}", file=sys.stderr)
        sys.exit(1)
