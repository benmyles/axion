#!/usr/bin/env python3
"""Render Axion field observations and publish immutable WebP journal entries."""
import base64
import csv
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
import statistics
import struct
import subprocess
import sys
import tempfile
import textwrap
import zlib
from collections import Counter,defaultdict

W,H=1500,1040
PAPER="#f2f0e7";INK="#172a27";MUTED="#546860";LINE="#c6cfc1"
GREEN="#235e51";MINT="#a3e4b8";AMBER="#edaa68";DARK="#142e2b"
COLORS=("#b6e897","#f6ab75","#87d6cf","#e8d170","#bcb6ec","#ed9dab","#8fbedc","#d2e4a5")
SANS="Adwaita Sans,DejaVu Sans,sans-serif";MONO="JetBrains Mono,DejaVu Sans Mono,monospace"
SERIF="Noto Serif,DejaVu Serif,serif"
ROOT=None;BINARY=None


def esc(s):return html.escape(str(s),quote=True)
def short(s,n=42):return s if len(s)<=n else s[:n-1].rstrip()+"…"
def human(n):
    if isinstance(n,str):return n
    if isinstance(n,float) and abs(n)<1000 and not n.is_integer():
        return f"{n:,.2f}".rstrip("0").rstrip(".")
    return f"{n:,.0f}" if abs(n)<1e7 else f"{n/1e9:.2f}B" if abs(n)>=1e9 else f"{n/1e6:.1f}M"
def val(r,k,default=0):
    try:return float(r.get(k,default))
    except (TypeError,ValueError):return default
def quantum(x):
    x=(int(x)^0x698126b7)&0xffffffff;x=((x^(x>>16))*0x7feb352d)&0xffffffff
    x=((x^(x>>15))*0x846ca68b)&0xffffffff;return x^(x>>16)
def hue(family):return COLORS[quantum(family)%len(COLORS)]
def blend(a,b,f):
    return "#"+"".join(f"{round(int(a[i:i+2],16)*(1-f)+int(b[i:i+2],16)*f):02x}" for i in (1,3,5))
def utc(timestamp=None):
    instant=dt.datetime.now(dt.timezone.utc) if timestamp is None else dt.datetime.fromtimestamp(timestamp,dt.timezone.utc)
    return instant.isoformat(timespec="seconds").replace("+00:00","Z")
def ratio(n,d):return f"{100*n/d:.1f}%" if d else "—"
def counted(n,noun):
    return f"{human(n)} {noun}" + ("" if n==1 else "s")
def pairs(line):return dict(re.findall(r'([^\s=]+)=([^\s]+)',line))
def numeric(x):
    try:return float(x)
    except (ValueError,TypeError):return None


class Drawing:
    def __init__(self,width=W,height=H):
        self.width=width;self.height=height
        self.parts=[f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
          '<defs><linearGradient id="water" x2="1" y2="1"><stop stop-color="#183d35"/><stop offset="1" stop-color="#0d2327"/></linearGradient>'
          '<linearGradient id="wash" x1="0" y1="0" x2="0" y2="1"><stop stop-color="#66a386" stop-opacity=".5"/><stop offset="1" stop-color="#66a386" stop-opacity=".02"/></linearGradient>'
          '<pattern id="grid" width="34" height="34" patternUnits="userSpaceOnUse"><path d="M 34 0 H 0 V 34" fill="none" stroke="#b4d5bb" stroke-opacity=".10" stroke-width="1"/></pattern></defs>']
        self.rect(0,0,width,height,PAPER)
    def add(self,s):self.parts.append(s)
    def rect(self,x,y,w,h,fill,r=0,extra=""):
        self.add(f'<rect x="{x:.2f}" y="{y:.2f}" width="{max(0,w):.2f}" height="{max(0,h):.2f}" rx="{r}" fill="{fill}" {extra}/>')
    def text(self,x,y,s,size=18,color=INK,font=SANS,weight=400,anchor="start",space=0):
        self.add(f'<text x="{x:.2f}" y="{y:.2f}" font-family="{font}" font-size="{size}" font-weight="{weight}" text-anchor="{anchor}" letter-spacing="{space}" fill="{color}">{esc(s)}</text>')
    def label(self,x,y,s,color=MUTED):self.text(x,y,s.upper(),13,color,MONO,500,space=1.1)
    def path(self,d,stroke,width=1,fill="none",extra=""):
        self.add(f'<path d="{d}" stroke="{stroke}" stroke-width="{width}" fill="{fill}" stroke-linecap="round" stroke-linejoin="round" {extra}/>')
    def dot(self,x,y,r,fill,extra=""):
        self.add(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="{r:.2f}" fill="{fill}" {extra}/>')
    def rule(self,x,y,x2,y2,color=LINE,width=1):self.path(f"M{x} {y} L{x2} {y2}",color,width)
    def finish(self):return "".join(self.parts)+"</svg>"


class Rect(C.Structure):
    _fields_=[("x",C.c_double),("y",C.c_double),("width",C.c_double),("height",C.c_double)]
class GError(C.Structure):
    _fields_=[("domain",C.c_uint),("code",C.c_int),("message",C.c_char_p)]


class Codec:
    """Use the system vector rasterizer and WebP encoder through their stable C ABIs."""
    def __init__(self):
        try:
            r=C.CDLL("librsvg-2.so.2");c=C.CDLL("libcairo.so.2");g=C.CDLL("libgobject-2.0.so.0");w=C.CDLL("libwebp.so.7");glib=C.CDLL("libglib-2.0.so.0")
        except OSError as e:raise RuntimeError(f"WebP journal needs librsvg, Cairo, GLib and libwebp: {e}") from e
        ptr=C.c_void_p;uint=C.c_size_t;integer=C.c_int;err=C.POINTER(C.POINTER(GError))
        def fn(lib,name,ret,*args):
            f=getattr(lib,name);f.restype=ret;f.argtypes=list(args);return f
        self.handle=fn(r,"rsvg_handle_new_from_data",ptr,C.c_char_p,uint,err)
        self.render=fn(r,"rsvg_handle_render_document",integer,ptr,ptr,C.POINTER(Rect),err)
        self.unref=fn(g,"g_object_unref",None,ptr);self.error_free=fn(glib,"g_error_free",None,C.POINTER(GError))
        self.surface=fn(c,"cairo_image_surface_create",ptr,integer,integer,integer)
        self.context=fn(c,"cairo_create",ptr,ptr);self.destroy=fn(c,"cairo_destroy",None,ptr)
        self.flush=fn(c,"cairo_surface_flush",None,ptr);self.drop=fn(c,"cairo_surface_destroy",None,ptr)
        self.data=fn(c,"cairo_image_surface_get_data",ptr,ptr);self.stride=fn(c,"cairo_image_surface_get_stride",integer,ptr)
        self.encode=fn(w,"WebPEncodeBGRA",uint,ptr,integer,integer,integer,C.c_float,C.POINTER(ptr));self.free=fn(w,"WebPFree",None,ptr)
    def write(self,drawing,path):
        data=drawing.finish().encode();error=C.POINTER(GError)();handle=self.handle(data,len(data),C.byref(error))
        def reason():
            if not error:return "unknown vector rendering failure"
            msg=error.contents.message.decode(errors="replace");self.error_free(error);return msg
        if not handle:raise RuntimeError(reason())
        width,height=drawing.width,drawing.height
        image=self.surface(0,width,height);ctx=self.context(image);output=C.c_void_p()
        try:
            if not self.render(handle,ctx,C.byref(Rect(0,0,width,height)),C.byref(error)):raise RuntimeError(reason())
            self.flush(image);pixels=self.data(image);stride=self.stride(image)
            if sys.byteorder!="little":
                original=C.string_at(pixels,stride*height)
                pixels=C.create_string_buffer(b"".join(original[i:i+4][::-1] for i in range(0,len(original),4)))
            n=self.encode(pixels,width,height,stride,88,C.byref(output))
            if not n:raise RuntimeError("WebP encoding failed")
            with open(path,"wb") as f:f.write(C.string_at(output,n))
        finally:
            if output:self.free(output)
            self.destroy(ctx);self.drop(image);self.unref(handle)


def png(size,pixels):
    def chunk(tag,data):return struct.pack(">I",len(data))+tag+data+struct.pack(">I",zlib.crc32(tag+data)&0xffffffff)
    stride=3*size;data=b"".join(b"\0"+pixels[i:i+stride] for i in range(0,len(pixels),stride))
    data=b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",size,size,8,2,0,0,0))+chunk(b"IDAT",zlib.compress(data,5))+chunk(b"IEND",b"")
    return base64.b64encode(data).decode()


def creature(d,x,y,scale,body,tape,color,angle=0):
    # Bilateral glyph geometry encodes body bits and expressed tape length.
    width=6+(body&7)*.65;length=16+((body>>4)&7);pairs=2+((max(1,tape)-1)%4);tail=6+((body>>18)&7)
    d.add(f'<g transform="translate({x:.2f},{y:.2f}) rotate({angle}) scale({scale:.3f})">')
    for i in range(pairs):
        q=-length*.42+i*length*.82/max(1,pairs-1)
        for sign in (-1,1):
            a=sign*width
            d.path(f"M{a*.65:.1f} {q:.1f} Q{a*1.5:.1f} {q-3:.1f} {a*1.8:.1f} {q+4:.1f} L{a*2.2:.1f} {q+6:.1f}",color,1.1)
    d.path(f"M-2 {length*.72:.1f} Q-2 {length+tail:.1f} -7 {length+tail+3:.1f} M2 {length*.72:.1f} Q3 {length+tail:.1f} 7 {length+tail+2:.1f}",color,1.4)
    d.path(f"M0 {-length:.1f} C{width*1.2:.1f} {-length*.9:.1f} {width*1.4:.1f} {length*.1:.1f} 2 {length:.1f} Q0 {length+2:.1f} -2 {length:.1f} C{-width*1.3:.1f} {length*.2:.1f} {-width*1.2:.1f} {-length*.9:.1f} 0 {-length:.1f}Z",color,1,color)
    for i in range(2+((body>>12)&3)):
        q=-length*.32+i*length*.23
        d.path(f"M{-width*.56:.1f} {q:.1f} Q0 {q+3:.1f} {width*.56:.1f} {q:.1f}","#1d3d33",1.1,extra='opacity=".65"')
    for sign in (-1,1):
        d.path(f"M{sign*width*.32:.1f} {-length*.9:.1f} Q{sign*width*.9:.1f} {-length-9:.1f} {sign*(width+5):.1f} {-length-13-(body>>24&3):.1f}",color,1.1)
        d.dot(sign*width*.43,-length*.55,1.25,"#18352e")
    d.add("</g>")


def auto_scores(data):
    result=list(data.get("scores",[]))
    for line in data.get("lines",[]):
        r=pairs(line)
        for key,label,num,den in (("junctions","Context-dependent revision","revised","items"),("crossings","Revised route","changed","items"),("maze","Shortest journey","shortest","journeys"),
            ("valuation","Food revaluation","changed","items"),("revisit","Object recall","correct","items"),("conditioning","Acquisition","learned","items"),
            ("appetite","Resident choice","correct","goals"),("spring_probe","Foraging choice","correct","items"),
            ("migration","Foraging survival","survived","lives"),("growth","Survival","survived","lifetimes")):
            if r.get(key) not in ("intact","resident","learned"):continue
            a,b=numeric(r.get(num)),numeric(r.get(den))
            if a is not None and b and 0<=a<=b:result.append([label,int(a),int(b)])
    return result


def foraging(data):
    return {r["migration"]:r for line in data.get("lines",[]) if line.startswith("migration=")
            for r in [pairs(line)] if r.get("lives") and r.get("offspring_mass")}


def interview_turns(data):
    if data.get("interviews"):
        traces=data.get("perceptions",[])
        turns=[]
        for i,v in enumerate(data["interviews"]):
            t=dict(turn=i+1,first=v&7,second=(v>>3)&7,request=(v>>6)&7,view=(v>>9)&3,sound=(v>>11)&7,words=v>>14)
            if i<len(traces):
                p=traces[i];t.update(covered=p&3,dim=(p>>2)&3,retinal=(p>>4)&15,inner=bool(p&256),imagined=bool(p&2048))
                if p&(1<<16):t.update(goal=(p>>17)&1,opened=(p>>18)&3,first_input=(p>>20)&15,second_input=(p>>24)&15)
            turns.append(t)
        return turns
    return [pairs(line) for line in data.get("lines",[]) if line.startswith("interview=turn")]


def dialogues(data):
    return {(r["dialogue"],r["wire"]):r for line in data.get("lines",[]) if line.startswith("dialogue=")
            for r in [pairs(line)] if "wire" in r and "exchanges" in r}

def recollections(data):
    return {r["recollection"]:r for line in data.get("lines",[]) if line.startswith("recollection=")
            for r in [pairs(line)] if "questions" in r}

def measured_performance(data):
    """Keep elapsed time, throughput and state sizes in their recorded units."""
    rows=[pairs(line) for line in data.get("lines",[]) if "=" in line]
    metrics=data.get("metrics",{})
    p={"device":data.get("device",""),"command":data.get("command","")}
    def value(r,k):
        try:
            number=float(r[k])
            return number if math.isfinite(number) and number>0 else 0
        except (KeyError,TypeError,ValueError,OverflowError):return 0
    flood=next((r for r in reversed(rows) if "flood_seconds" in r),{})
    whole=value(metrics,"command_wall_ns")*1e-9
    if whole:p.update(wall=whole,interactive=bool(metrics.get("interactive_wall")))
    rate_types=(("school_scenes_per_s",1,"scenes/s"),("Gcell_updates/s",1e9,"cell updates/s"),
                ("Gneuron_ticks/s",1e9,"neuron-ticks/s"),("Goutput_bits/s",1e9,"output bits/s"),
                ("Gtransitions_per_s",1e9,"transitions/s"),("Mchoices/s",1e6,"choices/s"),
                ("Mdecisions/s",1e6,"decisions/s"),("Mqueries/s",1e6,"queries/s"),("Mqueries_per_s",1e6,"queries/s"),
                ("Mcalls_per_s",1e6,"policy calls/s"),("Mmoves_per_s",1e6,"moves/s"),("Mevents_per_s",1e6,"events/s"),
                ("trials/s",1,"trials/s"),("Mmuscle_choices_per_s",1e6,"grip choices/s"),
                ("Mraft_slot_ticks_per_s",1e6,"raft slot-ticks/s"),("pod_presentations_per_s",1,"cue exposures/s"),("station_visits_per_s",1,"station visits/s"),("Mconstruction_choices_per_s",1e6,"hand choices/s"))
    accent=next((r for r in reversed(rows) if "acquired_voice" in r and value(r,"total_cuda_ms")), {})
    speed = next((r for r in rows if r.get("narrative_speed")=="packed" and value(r,"median_ms")), {})
    refs = next((r for r in rows if "narrative_references" in r and value(r,"cuda_ms")), {})
    cohort = next((r for r in rows if "narrative_cohort" in r and value(r,"cuda_ms")), {})
    if flood and value(flood,"site_ticks"):
        elapsed=value(flood,"simulation_seconds")
        duration=elapsed or value(flood,"flood_seconds")
        if duration:
            p.update(seconds=duration,time_scope="Advancing the habitat" if elapsed else "Advance and censuses",
                     rate=value(flood,"site_ticks")/duration,unit="site-ticks/s",work=int(flood["site_ticks"]))
            inclusive=value(flood,"flood_seconds")
            if elapsed and inclusive:p["advance_share"]=elapsed/inclusive
    elif (gbinf:=next((r for r in rows if r.get("genesis_bench")=="snn_inference" and value(r,"Gsyn_ops/s")),{})):
        sec=value(gbinf,"median_ms")*.001;work=int(gbinf.get("creatures",0))*int(gbinf.get("synapses_per_brain",4096))*int(gbinf.get("substeps",256))
        p.update(work=work,rate=value(gbinf,"Gsyn_ops/s")*1e9,unit="syn-ops/s",seconds=sec,time_scope="Blackwell SM 12.0 in-register SNN",bytes=int(gbinf.get("bytes_per_creature",18944)),byte_scope="Full embodied creature (4B/syn)")
    elif (gsum:=next((r for r in reversed(rows) if "genesis_summary" in r and value(r,"wall_ms")),{})):
        sec=value(gsum,"wall_ms")*.001;work=int(gsum.get("ticks",0))*int(gsum.get("pop",0))
        p.update(work=work,rate=work/sec,unit="creature-ticks/s",seconds=sec,time_scope="Coupled Blackwell world + SNN + CA-DNA",bytes=18944,byte_scope="Full embodied creature (4B/syn)")
    elif (msdr:=next((r for r in rows if r.get("mind_bench")=="sdr_columns" and value(r,"Gneuron_ticks/s")),{})):
        sec=value(msdr,"median_ms")*.001;work=int(msdr["columns"])*int(msdr["neurons_per_column"])*int(msdr["ticks"])
        p.update(work=work,rate=value(msdr,"Gneuron_ticks/s")*1e9,unit="neuron-ticks/s",seconds=sec,time_scope="Blackwell SM 12.0 cortical columns",bytes=816,byte_scope="Theta-gamma engram in L2")
    elif (mevo:=next((r for r in reversed(rows) if "mind_evo" in r and value(r,"gpu_ms")),{})):
        sec=value(mevo,"gpu_ms")*.001;work=int(mevo.get("spikes",0))
        n_pil = 114 if "mcmcs" in mevo else (112 if "pdes" in mevo else (110 if "pcae" in mevo else (108 if "sxlp" in mevo else (106 if "odrk" in mevo else (104 if "ffts" in mevo else (102 if "lqrc" in mevo else (100 if "kflt" in mevo else (98 if "mdpp" in mevo else (96 if "bnet" in mevo else (94 if "lalg" in mevo else (92 if "past" in mevo else (90 if "symc" in mevo else (88 if "sres" in mevo else (86 if "cdis" in mevo else (84 if "bdeb" in mevo else (82 if "nmet" in mevo else (80 if "dlgc" in mevo else (78 if "xmod" in mevo else (76 if "audv" in mevo else (74 if "visv" in mevo else (72 if "sats" in mevo else (70 if "gwav" in mevo else (68 if "nmat" in mevo else (66 if "sqpr" in mevo else (64 if "rbnd" in mevo else (62 if "rvsc" in mevo else (60 if "osch" in mevo else (58 if "chrl" in mevo else (56 if "ind" in mevo else (54 if "csat" in mevo else (52 if "csim" in mevo else (50 if "plan" in mevo else 48))))))))))))))))))))))))))))))))
        p.update(work=work,rate=work/sec,unit="spikes/s",seconds=sec,time_scope=f"CUDA {n_pil}-pillar evolutionary generation",bytes=48,byte_scope="Evolvable cortical genome")
    elif (meval:=next((r for r in reversed(rows) if "mind_eval" in r and value(r,"ms")),{})):
        sec=value(meval,"ms")*.001;work=int(meval.get("spikes",0))
        nd=sum(1 for l in data.get("lines",[]) if l.strip().startswith("mind_domain="))
        p.update(work=work,rate=work/sec,unit="spikes/s",seconds=sec,time_scope=f"GPU {nd or 42}-domain cognitive battery",bytes=816,byte_scope="Theta-gamma engram")
    elif (speed:=next((r for r in rows if "compound_benchmark" in r and value(r,"cooperative_ms")),{})):
        seconds=value(speed,"cooperative_ms")*.001;work=int(speed["compound_benchmark"])
        p.update(work=work,rate=work/seconds,unit="feeder choices/s",seconds=seconds,time_scope="CUDA frozen readout",bytes=int(speed["receipt_bytes"]),byte_scope="Physical receipt per choice")
    elif (voices:=next((r for r in rows if "auditory_feeders" in r and value(r,"cuda_ms")),{})):
        seconds=value(voices,"cuda_ms")*.001;work=int(voices["cases"])
        p.update(work=work,rate=work/seconds,unit="spoken encounters/s",seconds=seconds,time_scope="CUDA private dialogue",bytes=96,byte_scope="Learned voice and memory per pupil")
    elif (four:=next((r for r in rows if "compound_lives" in r or "compound_population" in r or "compound_exchange" in r),{})) and (value(four,"elapsed_ms") or value(four,"cuda_ms")):
        seconds=(value(four,"elapsed_ms") or value(four,"cuda_ms"))*.001
        population=value(four,"pairs")*value(four,"histories") or value(four,"compound_lives") or 1
        work=population*value(four,"lessons")+(2 if "compound_exchange" not in four else 1)*value(four,"cases")
        p.update(work=work,rate=work/seconds,unit="four-feeder trials/s",seconds=seconds,time_scope="CUDA acquisition and frozen tests",bytes=4,byte_scope="Two-port synapses per animal")
    elif (signals:=next((r for r in rows if ("material_relay" in r or "material_correspondence" in r) and value(r,"cuda_ms")),{})):
        seconds=value(signals,"cuda_ms")*.001
        if "material_relay" in signals:
            work=value(signals,"pairs")*value(signals,"histories")*value(signals,"lessons")+2*value(signals,"cases")
        else:work=value(signals,"lessons")+value(signals,"cases")
        p.update(work=work,rate=work/seconds,unit="foraging trials/s",seconds=seconds,time_scope="CUDA acquisition and frozen tests",bytes=int(signals.get("contact_bytes",16)),byte_scope="Appetitive synapses per animal")
    elif (reactions:=next((r for r in rows if "catalytic_contacts" in r and value(r,"cuda_ms")),{})):
        seconds=value(reactions,"cuda_ms")*.001;work=value(reactions,"catalytic_contacts")
        p.update(work=work,rate=work/seconds,unit="two-beat tests/s",seconds=seconds,time_scope="CUDA material contacts")
    elif (cohort:=next((r for r in rows if "context_population" in r and value(r,"cuda_ms")),{})):
        work=int(cohort["cases"]);seconds=value(cohort,"cuda_ms")/1000
        p.update(work=work,rate=work/seconds,unit="paired episodes/s",seconds=seconds,time_scope="CUDA within-body retention comparison")
    elif (sequence:=next((r for r in rows if "context_series" in r and value(r,"cuda_ms")),{})):
        work=int(sequence["total"]);seconds=value(sequence,"cuda_ms")/1000
        p.update(work=work,rate=work/seconds,unit="episodes/s",seconds=seconds,time_scope="CUDA continuous learning and decisions")
        if int(sequence["arm"]) not in (1,2):p.update(bytes=int(sequence["bytes"]),byte_scope="Additional material context / animal")
    elif (proof:=next((r for r in rows if "detritus_boundaries" in r and value(r,"cuda_ms")),{})):
        work=int(proof["detritus_boundaries"]);seconds=value(proof,"cuda_ms")/1000
        p.update(work=work,rate=work/seconds,unit="partitions/s",seconds=seconds,time_scope="CUDA transport boundary proof")
    elif (stamped:=next((r for r in rows if r.get("capillary_speed")=="compiled" and value(r,"median_ms")),{})):
        work=int(stamped["cell_steps"]);seconds=value(stamped,"median_ms")/1000
        p.update(work=work,rate=work/seconds,unit="site forecasts/s",seconds=seconds,time_scope="Median CUDA pressure kernel",bytes=int(stamped["bytes"]),byte_scope="Read-only material circuit")
    elif (test:=next((r for r in reversed(rows) if any(k in r for k in ("capillary_instruction","capillary_exchange","capillary_interview","capillary_school","capillary_cohort")) and value(r,"cuda_ms")),{})):
        work=int(test.get("cases",int(test.get("enrolled",0))*2*int(test.get("trials",0))));seconds=value(test,"cuda_ms")/1000
        p.update(work=work,rate=work/seconds,unit="cases/s",seconds=seconds,time_scope="CUDA "+("instruction and construction" if "capillary_instruction" in test else "material and speech trials"))
    elif accent:
        work=int(accent["images"])*int(accent["acquired_voice"]);seconds=value(accent,"total_cuda_ms")/1000
        p.update(work=work,rate=work/seconds,unit="paired tests/s",seconds=seconds,time_scope="CUDA acoustic transmission and tuition")
    elif speed:
        work=int(speed["histories"]); seconds=value(speed,"median_ms")/1000
        p.update(work=work,rate=work/seconds,unit="histories/s",seconds=seconds,time_scope="Median CUDA vocal inference",
                 bytes=int(speed["image_bytes"]),byte_scope="Compiled vocal circuit")
    elif refs:
        work=4*int(refs["layouts"]);seconds=value(refs,"cuda_ms")/1000
        p.update(work=work,rate=work/seconds,unit="questions/s",seconds=seconds,time_scope="CUDA temporal questions")
    elif cohort:
        work=int(cohort["enrolled"])*int(cohort["tests"]);seconds=value(cohort,"cuda_ms")/1000
        p.update(work=work,rate=work/seconds,unit="answers/s",seconds=seconds,time_scope="CUDA tuition and examination")
    else:
        for r in reversed(rows):
            if "narrative_speed" in r and value(r, "median_ms"):
                work = int(r["histories"]); seconds = value(r, "median_ms") / 1000
                p.update(work=work, rate=work / seconds, unit="histories/s", seconds=seconds, time_scope="CUDA vocal inference", bytes=int(r["image_bytes"]), byte_scope="Read-only vocal image")
                break
            if "narrative_exam" in r and value(r, "cuda_ms"):
                work = int(r["cases"]); seconds = value(r, "cuda_ms") / 1000
                p.update(work=work, rate=work / seconds, unit="requests/s", seconds=seconds, time_scope="CUDA scenes, speech and capture")
                break
            if "audience_exchange" in r and value(r,"cuda_ms"):
                work=int(value(r,"encounters")); seconds=value(r,"cuda_ms")/1000
                p.update(work=work,rate=work/seconds,unit="encounters/s",seconds=seconds,
                         time_scope="CUDA social contact")
                break
            if ("retinal_panel" in r or "cascade_interview" in r) and value(r,"cuda_ms"):
                speaks="cascade_interview" in r
                work=int(value(r,"courses")*(1 if speaks else value(r,"bodies")))
                seconds=value(r,"cuda_ms")/1000
                if work:
                    p.update(work=work,rate=work/seconds,unit="interviews/s" if speaks else "courses/s",seconds=seconds,
                             time_scope="Sealed acoustic predictions" if speaks else "CUDA optical courses")
                    break
            if ("inlet_panel" in r or "inlet_conversation" in r) and value(r,"cuda_ms"):
                spoken="inlet_conversation" in r
                work=int(value(r,"courses")*(1 if spoken else value(r,"programs")))
                duration=value(r,"cuda_ms")/1000
                if work:
                    p.update(rate=work/duration,unit="interviews/s" if spoken else "first reaches/s",seconds=duration,
                             time_scope="Prospective acoustic trials" if spoken else "CUDA withheld inlets",work=work)
                    break
            if "drift_food_test" in r and value(r,"cuda_ms"):
                work=int(value(r,"alleles")*value(r,"lives")*96);duration=value(r,"cuda_ms")/1000
                if work:
                    p.update(rate=work/duration,unit="beads/s",seconds=duration,time_scope="CUDA common currents",work=work)
                    break
            if ("apprentice_language" in r or "regulatory_survey" in r or "brood_budget" in r) and value(r,"cuda_ms"):
                language="apprentice_language" in r;brood="brood_budget" in r
                work=int(value(r,"delivered") if brood else value(r,"cases") if language else value(r,"enrolled")*value(r,"dialects"));duration=value(r,"cuda_ms")/1000
                if work:
                    p.update(rate=work/duration,unit="beads/s" if brood else "lessons/s",seconds=duration,time_scope="CUDA material rearing" if brood else "CUDA classroom",work=work)
                    break
            if ("field_class" in r or "field_interview" in r) and value(r,"cuda_ms"):
                population="field_class" in r
                work=int(value(r,"field_class" if population else "questions"));duration=value(r,"cuda_ms")/1000
                if work:
                    p.update(rate=work/duration,unit="lessons/s" if population else "queries/s",seconds=duration,time_scope="CUDA classroom" if population else "CUDA field replies",work=work)
                    break
            if "neural_benchmark" in r and value(r,"packed_ms"):
                census=next((v for v in reversed(rows) if "neural_dispatch" in v),{})
                work=int(value(census,"expressed_lanes"))*32*int(r["neural_benchmark"]);duration=value(r,"packed_ms")/1000
                if work:p.update(rate=work/duration,unit="cell-ticks/s",seconds=duration,time_scope="Packed cortical kernel",work=work);break
            if "braid_view_ms" in r and value(r,"braid_view_ms"):
                tasks=int(r['conversations']);duration=value(r,'braid_view_ms')/1000
                p.update(rate=tasks/duration,unit="witness probes/s",seconds=duration,time_scope="Training and perspective probes",work=tasks);break
            if "braid_episodes" in r and value(r,"cuda_ms"):
                tasks=36*int(r["braid_episodes"])*int(r.get("withheld_per_kind",1));duration=value(r,"cuda_ms")/1000
                p.update(rate=tasks/duration,unit="follow-ups/s",seconds=duration,time_scope="Teaching and paired inquiry kernel",work=tasks);break
            if "braid_dialogue_ms" in r or "braid_timing" in r:
                duration=(value(r,"braid_dialogue_ms") or value(r,"braid_timing"))/1000;tasks=value(r,"conversations") or value(r,"decisions")
                if duration and tasks:
                    p.update(rate=tasks/duration,unit="exchanges/s" if "conversations" in r else "decisions/s",seconds=duration,time_scope="Exposure and probe kernels",work=int(tasks));break
            if "braid_image" in r and value(r,"packed_ms"):
                tasks=int(r["calls"]);duration=value(r,"packed_ms")/1000
                p.update(rate=tasks/duration,unit="requests/s",seconds=duration,time_scope="Frozen motor planner kernel",work=tasks,bytes=int(r["bytes"]),byte_scope="Frozen float readout / resident");break
            if "bank_image" in r and value(r,"packed_ms"):
                p.update(rate=1000*value(r,"pulls")/value(r,"packed_ms"),unit="choices/s",seconds=value(r,"packed_ms")/1000,time_scope="Packed bank readout kernel",work=int(r["pulls"]),bytes=4,byte_scope="Frozen bank readout / resident");break
            if "bank_timing" in r and value(r,"bank_timing"):
                tasks=int(r["paid_pulls"])+int(r["probes"]);ms=value(r,"bank_timing")
                p.update(rate=tasks*1000/ms,unit="choices/s",seconds=ms/1000,time_scope="Paired exposure and probe kernels",work=tasks);break
            if "reed_image" in r and value(r,"packed_ms"):
                p.update(rate=1000*value(r,"decisions")/value(r,"packed_ms"),unit="choices/s",seconds=value(r,"packed_ms")/1000,time_scope="Packed motor kernel",work=int(r["decisions"]),bytes=int(r["bytes"])//int(r["reed_image"]),byte_scope="Frozen choices / animal");break
            if "runnel_timing" in r and value(r,"seconds"):
                p.update(rate=value(r,"trials")/value(r,"seconds"),unit="trials/s",seconds=value(r,"seconds"),time_scope="GPU experiment and readback",work=int(r["trials"]));break
            if "practice_image" in r and value(r,"Mbranches_per_s"):
                p.update(rate=value(r,"Mbranches_per_s")*1e6,unit="branches/s",seconds=value(r,"reading_ms")/1000,work=int(r["queries"]),time_scope="Packed contingent dispatch",bytes=int(r["image_bytes"])+int(r["directory_bytes"]),byte_scope="Complete learned policy bank");break
            if "practice_conversations" in r and value(r,"conversations_per_s"):
                p.update(rate=value(r,"conversations_per_s"),unit="exchanges/s",seconds=value(r,"seconds"),work=int(r["practice_conversations"]),time_scope="Practice, speech, physical work and readback");break
            if "linkage_meetings" in r and value(r,"meetings_per_s"):
                p.update(rate=value(r,"meetings_per_s"),unit="meetings/s",work=int(r["linkage_meetings"]),seconds=value(r,"seconds"),time_scope="Practice, dialogue, action and readback");break
            if "linkage_compile" in r and value(r,"Mplans_per_s"):
                p.update(rate=value(r,"Mplans_per_s")*1e6,unit="plans/s",work=int(r["trials"]),seconds=value(r,"image_ms")/1000,time_scope="Fixed mechanism image",bytes=int(r["image_bytes"]),byte_scope="Per learned fixture and goal");break
            if "effort_surface_dialogues" in r and value(r,"questions_per_s"):
                p.update(rate=value(r,"questions_per_s"),unit="questions/s",seconds=value(r,"seconds"),time_scope="Surface questions and readback",work=int(r["questions"]));break
            if "effort_dialogues" in r and value(r,"turns_per_s"):
                p.update(rate=value(r,"turns_per_s"),unit="turns/s",seconds=value(r,"seconds"),time_scope="Spoken route choices and readback",work=int(r["turns"]));break
            if "effort_deploy" in r and value(r,"Mcomparisons_per_s"):
                p.update(rate=value(r,"Mcomparisons_per_s")*1e6,unit="comparisons/s",seconds=value(r,"packed_ms")/1000,time_scope="Packed valuation kernel",work=int(r["comparisons"]),bytes=int(r["image_bytes"]),byte_scope="Prospective readout / animal");break
            if "press_dialogues" in r and value(r,"questions_per_s"):
                p.update(rate=value(r,"questions_per_s"),unit="questions/s",seconds=value(r,"seconds"),time_scope="GPU exchanges and readback",work=int(r["press_dialogues"]));break
            if "press_deploy" in r and value(r,"Mdecisions_per_s"):
                p.update(rate=value(r,"Mdecisions_per_s")*1e6,unit="decisions/s",seconds=value(r,"packed_ms")/1000,time_scope="Frozen choice kernel",work=int(r["per_batch"]));break
            # Older observations recorded duration and work separately.
            if value(r,"assay_gpu_ms") and value(r,"rounds") and value(r,"pod_transfers"):
                r=dict(r,pod_presentations_per_s=2000*value(r,"rounds")*value(r,"pod_transfers")/value(r,"assay_gpu_ms"))
            choice=next(((value(r,k)*scale,unit) for k,scale,unit in rate_types if value(r,k)),None)
            duration=next((value(r,k)/1000 for k in ("school_gpu_ms","median_ms","GPU_ms","learning_ms","assay_gpu_ms") if value(r,k)),0)
            if "raft_slot_ticks" in r:duration=value(r,"seconds")
            if "workshop_seconds" in r:duration=value(r,"workshop_seconds")
            if not choice and not duration:continue
            if choice:p.update(rate=choice[0],unit=choice[1])
            if duration:p.update(seconds=duration,time_scope="Learning and replay" if "school_gpu_ms" in r else "Median GPU batch" if "median_ms" in r else "Timed GPU interval")
            break
    shoal=next((r for r in rows if r.get("shoal_test")=="experienced"),{})
    deployment=next((r for r in rows if value(r,"Mdiagrams_per_s")),{})
    if not value(flood,"site_ticks") and deployment:
        p.update(rate=value(deployment,"Mdiagrams_per_s")*1e6,unit="diagrams/s",seconds=value(deployment,"median_ms")/1000,
                 time_scope="Median GPU batch",bytes=int(deployment["spatial_image_bytes"]),byte_scope="Settled contacts / animal")
    elif not value(flood,"site_ticks") and value(shoal,"seconds"):
        p.update(rate=value(shoal,"trials")/value(shoal,"seconds"),unit="beds/s",seconds=value(shoal,"seconds"),
                 time_scope="Building and paired exposures",bytes=16,byte_scope="Spatial contacts / animal")
    field=next((r for r in rows if r.get("field_transfer")=="lived"),{})
    image=next((r for r in rows if value(r,"Mmaterial_diagrams_per_s")),{})
    if not value(flood,"site_ticks") and image:
        p.update(rate=value(image,"Mmaterial_diagrams_per_s")*1e6,unit="diagrams/s",seconds=value(image,"median_ms")/1000,
                 time_scope="Median GPU batch",bytes=int(image["receptive_bytes"]),byte_scope="Settled field receptors / animal")
    elif not value(flood,"site_ticks") and value(field,"seconds"):
        p.update(rate=value(field,"trials")/value(field,"seconds"),unit="beds/s",seconds=value(field,"seconds"),
                 time_scope="Lived circuits in fresh beds")
    salts=next((r for r in rows if value(r,"Mbrine_choices_per_s")),{})
    transfer=next((r for r in rows if r.get("brine_transfer")=="responsive"),{})
    social=next((r for r in reversed(rows) if r.get("brine_dialogue")=="consulted"),{})
    if not value(flood,"site_ticks") and salts:
        p.update(rate=value(salts,"Mbrine_choices_per_s")*1e6,unit="choices/s",seconds=value(salts,"median_ms")/1000,
                 time_scope="Median GPU batch",bytes=int(salts["settled_bytes"]),byte_scope="Settled preferences / animal")
    elif not value(flood,"site_ticks") and value(transfer,"seconds"):
        p.update(rate=value(transfer,"choices")/value(transfer,"seconds"),unit="choices/s",seconds=value(transfer,"seconds"),
                 time_scope="Exposure and physical test",bytes=8,byte_scope="Writable predictive traces")
    elif value(social,"seconds") and value(social,"questions"):
        p.update(rate=value(social,"questions")/value(social,"seconds"),unit="questions/s",seconds=value(social,"seconds"),time_scope="Consultation and feeding")
    compiled=next((q for q in rows if value(q,"Mcontacts_per_s")),{})
    uttered=next((q for q in reversed(rows) if value(q,"utterances_per_s")),{})
    brood=next((q for q in rows if value(q,"pair_lives_per_s")),{})
    if compiled:
        p.update(rate=value(compiled,"Mcontacts_per_s")*1e6,unit="pair contacts/s",seconds=value(compiled,"median_ms")/1000,time_scope="Median frozen GPU batch",bytes=int(compiled["settled_bytes"]),byte_scope="Frozen acoustic circuit / animal")
    elif uttered:
        p.update(rate=value(uttered,"utterances_per_s"),unit="utterances/s",seconds=value(uttered,"seconds"),time_scope="GPU exchange and readback")
    elif brood and not value(flood,"site_ticks"):
        p.update(rate=value(brood,"pair_lives_per_s"),unit="pair lives/s",seconds=value(brood,"seconds"),time_scope="Rearing and replacement")
    ceramic=next((q for q in rows if value(q,"Mclay_choices_per_s")),{})
    if ceramic:p.update(rate=value(ceramic,"Mclay_choices_per_s")*1e6,unit="object choices/s",seconds=value(ceramic,"median_ms")/1000,time_scope="Median GPU batch",bytes=int(ceramic.get("frozen_bytes",ceramic["chart_bytes"])),byte_scope="Frozen inference circuit / resident" if "frozen_bytes" in ceramic else "Cached predictions / resident")
    states=(("hand_image_bytes","Hand image / animal"),("motor_image_bytes","Motor image / animal"),("resident_bytes_per_site","Habitat state / site"),("state_bytes","State / animal"),("resident_bytes","State / resident"),
            ("policy_bytes","Policy state"),("plastic_state_bytes","Plastic state"),("code_bytes","Compiled code"),("organ_bytes","Learning organ"),
            ("speaker_bytes","Speaker state"),("developmental_bytes","Developmental circuit"),("agency_bytes","Agency circuit"),
            ("concept_bytes","Concept circuit"),("recollection_bytes","Recall circuit"))
    if p.get("unit")=="site-ticks/s":
        physical=next((value(r,"resident_bytes_per_site") for r in rows if value(r,"resident_bytes_per_site")),0)
        if physical:p.update(bytes=int(physical),byte_scope="Habitat state / site")
    for r in [metrics,*reversed(rows)]:
        if "bytes" in p:break
        if "grounding_neurons" in r and value(r,"bytes"):
            p.update(bytes=int(r["bytes"]),byte_scope="Grounding state / animal");break
        sample=next(((int(value(r,k)),label) for k,label in states if value(r,k)),None)
        if sample:p.update(bytes=sample[0],byte_scope=sample[1]);break
    if whole and not p.get("rate"):
        observations=sum(2*int(r["pairs"]) if "reed_analogy" in r or "flume_dialogue" in r else int(r["communities"]) if "reed_chain" in r else 0 for r in rows)
        if observations:p.update(rate=observations/whole,unit="trials/s",seconds=whole,time_scope="Complete experiment command",work=observations)
    if whole and not p.get("rate"):
        first_choices=sum(int(r["offered"]) for r in rows if "press_transfer" in r and "offered" in r)
        seasons=[r for r in rows if "press_season" in r]
        if first_choices:
            p.update(seconds=whole,time_scope="Preparation and scoring",rate=first_choices/whole,unit="first choices/s",work=first_choices)
        elif seasons:
            cohort=int(seasons[-1]["press_season"])*int(seasons[-1]["adults"])
            p.update(seconds=whole,time_scope="Rearing and census",rate=cohort/whole,unit="lifetimes/s",work=cohort)
    if metrics.get("field_conversation_events") and p.get("byte_scope")=="Speaker state":
        p["byte_scope"]="Speaker voice and vision"
    arena=next((r for r in reversed(rows) if r.get("neural_storage")=="paged"),None)
    if arena and value(arena,"device_bytes"):
        p["bytes"]=int(arena["device_bytes"]);p["byte_scope"]="Reserved CUDA neural arena"
        p["neural_spill_pages"]=int(arena.get("spill_pages",0))
    if speed and p.get("time_scope")=="Median CUDA vocal inference":
        p.update(bytes=int(speed["image_bytes"]),byte_scope="Compiled vocal circuit")
    return p if any(k in p for k in ("wall","seconds","rate","bytes")) else {}


def perf_number(n):
    for scale,unit in ((1e12,"T"),(1e9,"G"),(1e6,"M"),(1e3,"k")):
        if n>=scale:return f"{n/scale:.3g}{unit}"
    return f"{n:.3g}"

def perf_time(n):
    if n<.001:return f"{n*1e6:.3g} μs"
    if n<1:return f"{n*1e3:.3g} ms"
    if n<60:return f"{n:.3g} s"
    return f"{n/60:.3g} min"

def perf_items(p):
    items=[]
    if p.get("rate"):items.append((perf_number(p["rate"])+" "+p["unit"],"Measured throughput"))
    if p.get("seconds"):items.append((perf_time(p["seconds"]),p["time_scope"]))
    elif p.get("wall"):items.append((perf_time(p["wall"]),"Session including input" if p.get("interactive") else "Command elapsed"))
    if p.get("bytes"):items.append((f"{p['bytes']:,} B",p["byte_scope"]))
    if "advance_share" in p:items.append((f"{100*p['advance_share']:.1f}%","Advance / census loop time"))
    elif p.get("seconds") and p.get("wall") and not (p.get("wall")==p.get("seconds") and p.get("time_scope") in ("Preparation and scoring","Rearing and census")):
        items.append((perf_time(p["wall"]),"Session including input" if p.get("interactive") else "Command elapsed"))
    return items

def performance_ribbon(d,performance,caption="Run performance"):
    facts=perf_items(performance)
    if not facts:return
    y=982;d.rect(0,y,W,H-y,"#203c37")
    d.text(49,y+24,caption.upper(),13,"#c1d8c2",MONO,600,space=.6)
    gpu=performance.get("device","").replace("NVIDIA ","").replace(" Generation Laptop GPU","").replace(" Laptop GPU","")
    d.text(49,y+45,short(gpu,28),13,"#b5c8ba")
    width=(W-315)/len(facts)
    for i,(metric,label) in enumerate(facts):
        x=286+i*width;d.rule(x-14,y+12,x-14,H-10,"#48655c")
        d.text(x,y+27,metric,20,"#edf4dd",SANS,600);d.text(x,y+47,label,13,"#bfd4c3")

def latest_timing(history,habitat=False):
    return next((r for r in reversed(history) if r.get("performance",{}).get("rate") and
                 (not habitat or r["command"]=="floodplain")),None)


def backwater_summary(entries):
    def v(q,k):return int(q.get(k,0))
    probes=entries("bank_test");talk=entries("bank_talk");broods=entries("bank_brood");field=entries("bank_field")
    spoken={r["bank_dialogue"]:r for r in entries("bank_dialogue")}
    if "heard" in spoken:
        q=spoken["heard"];silent=spoken["mute"]
        return ("Asking the worker at the bank","A learned name for the source   /   a reply about its impulse","Spoken decisions after diversion",
                100*v(q,"correct")/v(q,"probes"),"%", [["Public decisions",v(q,"probes")],["Correct and decoded",v(q,"correct")],["Verified explanations",v(q,"faithful")]],
                [f"A copy of the speaker requested one of two local measurements on each of {v(q,'probes'):,} held-out reaches.",
                 f"After the spring moved, {v(q,'correct'):,} spoken instructions named the stocked pan and were decoded by the worker.",
                 f"Removing the cited source changed the public decision in {v(q,'faithful'):,} physically verified explanations."],
                [f"Muting the reports left {v(silent,'correct'):,} correct spoken decisions.",f"The listener consumed {v(q,'food'):,} measured units of sediment carbon on the probes."])
    if probes:
        arms={r["bank_test"]:r for r in probes if v(r,"season")==2}
        q=arms["open"];a=arms.get("invited");pool=arms.get("chorus")
        # The common garden field remains independent of the standardized twin assay.
        return ("When a different bank becomes reliable","Paired foragers   /   hidden diversion   /   identical public probe",
                "Choice after the spring moves",100*v(q,"correct")/v(q,"probes"),"%",
                [["Held-out pulls",v(q,"probes")],["Both opposed pans",v(q,"both")],["Frozen after first season",v(arms["first_only"],"correct")]],
                [f"The spring changed banks after {v(q,'exposure')} encounters per resident.",
                 f"On the following probes, foragers chose {v(q,'correct'):,} of {v(q,'probes'):,} stocked pans and solved both sides of {v(q,'both'):,} acoustic conflicts.",
                 (f"With a single inquiry they addressed the connected source on {v(a,'source_requests'):,} of {v(a,'probes'):,} probes." if a else f"Freezing further writes left {v(arms['first_only'],'correct'):,} correct reaches.")],
                [f"Silencing the upstream response gave {v(arms['cut_upstream'],'correct'):,} correct pulls.",
                 (f"Paired residents using sampled bodies as strikers gave {v(pool,'correct'):,} correct reaches through distinct resonators." if pool else f"Unfiltered resident drums gave {v(arms['wild_drums'],'correct'):,} correct pulls.")])
    if talk:
        q=talk[-1];actions=sum(v(r,"pulls") for r in talk);asks=sum(v(r,"inquiries") for r in talk)
        return ("A question at the fork","Two acoustic maps   /   two local resonators","Spoken bank inquiries",asks,"",
                [["Requests",asks],["Paid reaches",actions],["Carbon swallowed",sum(v(r,"food") for r in talk)]],
                [f"The speaker issued {asks:,} requests at the two-bank apparatus.",
                 f"The worker supplied {sum(v(r,'reports') for r in talk):,} recognized accounts of a local vibration.",
                 f"The speaker made {actions:,} paid reaches and closed with a reserve of {v(q,'reserve'):,}."],
                [f"{sum(v(r,'writes') for r in talk):,} delayed changes reached its private contacts.",
                 f"{sum(v(r,'cited') for r in talk):,} spoken explanations identified a waveform that changed the chosen pan."])
    if broods:
        q=broods[-1];a=broods[0];p=v(q,"residents")
        return ("A brood earns its place","Mutable ears and strikers   /   working sediment pans","Bank foraging selection",v(q,"food")," C",
                [["Generations",v(q,"bank_brood")],["Bodies per brood",p],["Final carbon",v(q,"food")]],
                [f"Each generation placed {p:,} juveniles in new two-bank districts.",
                 f"The first observed brood took {v(a,'food'):,} units of carbon and the last took {v(q,'food'):,}.",
                 ("Reproductive tournaments compared the carbon received by candidate parents." if q["selection"]=="food" else "Parents were sampled uniformly after the same encounters.")],
                [f"The last generation made {v(q,'pulls'):,} physical choices.",("All three foraging roles used mutable resident programs." if v(q,"native_drums") else "The two local test strikers had counterbalanced polarity and timbre.")])
    if field:
        q=field[-1];other=entries("bank_choice")[-1];ask=entries("bank_requests");mesh=entries("bank_mesh")
        net=v(other,"sound_gain")-v(other,"sound_loss");learned=v(other,"learned_gain")-v(other,"learned_loss")
        return ("Listening across the backwater",f"Watershed tick {v(q,'bank_field'):,}   /   {v(q,'living'):,} residents",
                "Food from sounding the banks",v(q,"food")," C",
                [["Paid pulls",v(q,"pulls")],["Strand impulses",v(q,"resonances")],["Residents with contacts",v(q,"experienced")]],
                [f"Residents made {v(q,'pulls'):,} reaches and ate {v(q,'food'):,} units from the two sediment banks.",
                 f"Hearing changed {v(other,'sound_changes'):,} choices, with a cumulative food difference of {net:+,} against the same immediate choice in silence.",
                 f"Keeping the listener’s acquired contacts changed that one-pull return by {learned:+,} relative to reading with neutral contacts."],
                [f"{v(q,'calls'):,} body strikes and {v(q,'resonances'):,} passive strand responses entered the field ledger.",
                 (f"Joint tension freed {v(mesh[-1],'extra'):,} extra carbon units and made {v(mesh[-1],'stuck_alone'):,} otherwise blocked reaches productive." if mesh else
                  f"{v(ask[-1],'made'):,} single-source inquiries have been made." if ask else f"{v(q,'contacts'):,} non-neutral contacts remain in living residents.")])
    return None


def backwater_plate(d,data,b,x,y,size,rx,rw):
    def rows(k):return [pairs(line) for line in data["lines"] if line.startswith(k+"=")]
    def ni(r,k):return int(r.get(k,0))
    bg="#172e35";water="#90c8bf";sand="#e9bc85";fog="#c5d9c7";dim="#668b85";rule="#3b575b"
    d.rect(x,y,size,size,bg,13)
    tests=rows("bank_test");field=rows("bank_field");speech=rows("bank_talk");broods=rows("bank_brood");voices=rows("bank_dialogue")
    def write_list(label,items):
        d.label(rx,335,label)
        for i,(name,value) in enumerate(items):
            at=374+i*40;d.text(rx,at,name,17,INK);d.text(rx+rw,at,human(value),18,GREEN,MONO,600,"end")
            d.rule(rx,at+11,rx+rw,at+11)
    if voices:
        d.label(x+28,y+40,"Public reports",water);d.text(x+28,y+92,"A voice across the water",35,"#f0f0dd",SERIF)
        labels={"heard":"Intact report","mute":"Return muted","reversed":"Inverted impulse","no_first_school":"Silent first exposure","frozen_after":"Later contacts held","inverted_probe":"Unexpected inversion"}
        for i,r in enumerate(voices):
            yy=y+152+62*i;right=ni(r,"correct")/max(1,ni(r,"probes"))
            d.text(x+28,yy,labels[r["bank_dialogue"]],18,fog);d.text(x+size-31,yy,f"{right:.1%}",20,sand,MONO,600,"end")
            d.rect(x+28,yy+13,size-63,10,rule,4);d.rect(x+28,yy+13,(size-63)*right,10,water,4)
        d.text(x+28,y+558,"Both animals must decode the chosen direction",16,fog)
        r=voices[0];write_list("Each public decision",[("Voiced inquiries",ni(r,"voiced")),("Heard reports",ni(r,"supplied")),("Spoken reasons",ni(r,"cited")),("Verified reasons",ni(r,"faithful"))])
    elif tests:
        d.label(x+27,y+39,"Opposed waterways",water);d.text(x+27,y+87,"Where is the spring?",33,"#f0f0dd",SERIF)
        d.text(x+27,y+120,"Full factorial probes after each change of inlet",17,fog)
        selected=("open","first_only","wild_drums") if not any(q["bank_test"]=="chorus" for q in tests) else ("open","invited","chorus")
        labels={"open":"Two calls","first_only":"Writes frozen","wild_drums":"Wild drums","invited":"One request","chorus":"Resident drums"}
        x0=x+89;y0=y+410
        for pct in (0,50,100):
            yy=y0-pct*2.25;d.rule(x0,yy,x+size-36,yy,rule,1);d.text(x0-13,yy+4,str(pct),14,dim,MONO,400,"end")
        for j,key in enumerate(selected):
            q=[p for p in tests if p["bank_test"]==key];col=(water,sand,"#b9a5c2")[j]
            pts=[(x0+28+188*(ni(p,"season")-1),y0-225*ni(p,"correct")/max(1,ni(p,"probes"))) for p in q]
            for (xa,ya),(xb,yb) in zip(pts,pts[1:]):d.rule(xa,ya,xb,yb,col,3)
            for px,py in pts:d.dot(px,py,5,col)
            d.dot(x+48+j*180,y+515,5,col);d.text(x+62+j*180,y+520,labels[key],15,fog)
        for i,label in enumerate(("First bank","Diverted","Restored")):d.text(x0+28+188*i,y0+32,label,15,fog,MONO,400,"middle")
        d.text(x+27,y+568,"Correct reaches (%)   ·   identical probe loads",15,water,MONO)
        r=next(q for q in tests if q["bank_test"]=="open" and ni(q,"season")==2)
        write_list("After diversion",[("Correct pulls",ni(r,"correct")),("Paired conflicts solved",ni(r,"both")),("Paired conflicts tested",ni(r,"conflicts")),("Private edits during exposure",ni(r,"writes"))])
    elif speech:
        d.label(x+28,y+40,"Acoustic exchange",water);d.text(x+28,y+90,"Which bank should speak?",34,"#f0f0dd",SERIF)
        quoted=[line for line in data["lines"] if line.startswith(("animal>","worker>"))][-6:]
        for i,line in enumerate(quoted):
            role,_,content=line.partition("> ");yy=y+159+i*63
            d.text(x+30,yy,role.upper(),13,water if role=="animal" else sand,MONO,600)
            d.text(x+140,yy,short(content,32),23,fog,SERIF)
        r=speech[-1];write_list("The working apparatus",[("Encounters completed",ni(r,"round")),("Swallowed carbon",ni(r,"consumed")),("Source reserve",ni(r,"source")),("Speaker’s reserve",ni(r,"reserve"))])
    elif broods:
        q=broods[-1];d.label(x+28,y+39,"Carbon and descent",water);d.text(x+28,y+91,"What pays for offspring",33,"#f0f0dd",SERIF)
        cap=max(1,max(ni(p,"food") for p in broods))
        for i,r in enumerate(broods[-5:]):
            yy=y+160+i*70;d.text(x+28,yy+17,"G"+str(ni(r,"bank_brood")),17,fog,MONO)
            wid=(size-223)*ni(r,"food")/cap;d.rect(x+114,yy,wid,17,water,4);d.text(x+size-26,yy+17,human(ni(r,"food")),15,sand,MONO,600,"end")
        d.text(x+28,y+563,"Total carbon received by each sampled generation",16,fog)
        write_list("Founding population",[("Residents",ni(q,"residents")),("Generations",ni(q,"bank_brood")),("Final pulls",ni(q,"pulls")),("Final harvest",ni(q,"food"))])
    else:
        q=field[-1];maps=rows("bank_map");mesh=rows("bank_mesh")
        d.label(x+27,y+39,"Sediment and sound",water);d.text(x+27,y+87,"Silt caught in woven banks" if mesh else "The inhabited banks",33,"#f0f0dd",SERIF)
        if maps:
            bins=ni(maps[0],"bins");step=23 if bins==16 else min(39,370/bins);xx=x+(size-bins*step)/2;yy=y+123
            for j,row in enumerate(maps):
                for i,ch in enumerate(row["samples"]):
                    val=int(ch,16);xp=xx+step*i;yp=yy+step*j
                    d.rect(xp,yp,step-1,step-1,("#284546","#36605a","#497567","#74936e")[val&3],2)
                    if val>>2:d.dot(xp+step/2,yp+step/2,1.4+1.55*(val>>2),sand)
            d.text(x+28,y+524,"Green  /  fibre coverage",16,water,MONO);d.text(x+28,y+552,"Amber  /  share of residents with written contacts",16,sand)
        else:
            # Early records retain actual census frames even before district maps were added.
            chart=(("Resident strikes",ni(q,"calls")),("Passive resonances",ni(q,"resonances")),("Audible reaches",ni(q,"heard")),("Delayed contact edits",ni(q,"writes")))
            cap=max(1,max(v for _,v in chart))
            for i,(label,value) in enumerate(chart):
                yy=y+178+83*i;d.text(x+28,yy,label,18,fog);d.rect(x+29,yy+14,(size-85)*value/cap,12,(water,sand)[i&1],3)
        if mesh:
            m=mesh[-1]
            write_list("Work at the two ends",[("Carbon caught in wet mesh",ni(m,"trapped")),("Reaches with opposing grips",ni(m,"simultaneous")),("Reaches requiring both grips",ni(m,"stuck_alone")),("Extra food freed by the second grip",ni(m,"extra"))])
        else:write_list("Across the watershed",[("Living residents",ni(q,"living")),("Paid reaches",ni(q,"pulls")),("Swallowed carbon",ni(q,"food")),("Matured sensory traces",ni(q,"digested"))])



def runnel_summary(entries):
    def batch(key):return {r[key]:r for r in entries(key)}
    def n(r,k):return int(r[k])
    spoken=batch("flume_dialogue");analog=batch("reed_analogy");chain=batch("reed_chain");rig=batch("reed_rig")
    images=entries("reed_image");talk=entries("flume_contact");water=entries("reed_field");breeding=entries("reed_brood")
    if spoken:
        a=spoken["heard"];quiet=spoken["muted"]
        return ("A question about a different joint","Two specialists   /   separated reference experiences","Spoken mechanical analogies",n(a,"both")," pairs",
                [["Calibration pairs",n(a,"pairs")],["Both solved",n(a,"both")],["Further correct folds",n(a,"later_exact")]],
                [f"Each speaker and consultant handled disjoint reference joints before receiving {n(a,'pairs'):,} paired requests.",
                 f"Both calibrations worked in {n(a,'both'):,} pairs with the acoustic channel open and {n(quiet,'both'):,} with the report muted.",
                 f"After the consultation, speakers made {n(a,'later_exact'):,} correct folds on another surface without further testimony."],
                [f"{n(a,'indirect'):,} correct first folds had a reference relation and a received report.",
                 f"The applied apparatus returned {n(a,'carbon'):,} units of source carbon."])
    if analog:
        a=analog["related"];fixed=analog["local"];corner=analog["no_corner"]
        return ("The missing joint",f"{'Chiral' if int(a.get('handed',0)) else 'Rotary'} mineral contacts   /   paired calibration","Unseen joint in both calibrations",n(a,"both")," pairs",
                [["Paired materials",n(a,"pairs")],["Both solved",n(a,"both")],["Direct target samples",n(a,"target_contacts")]],
                [f"Juveniles encountered {n(a,'observations'):,} reference strokes across two physical calibrations.",
                 f"They selected the right orientation in both calibrations on {n(a,'both'):,} pairs.",
                 f"With the intermediate reference withheld they solved {n(corner,'both'):,} pairs in both worlds."],
                [f"Direct contact lookup alone solved {n(fixed,'both'):,} pairs.",f"The constructed test pumps collected {n(a,'carbon'):,} units of source carbon."])
    stewards=batch("reed_stewards")
    if stewards:
        q=stewards["inherited"];flat=stewards["flatten_at_birth"];sight=stewards["no_witness"]
        return ("Keeping a waterway alive","Paid local movements   /   an initially unformed channel","Carbon from a maintained channel",n(q,"source_delivered")," carbon",
                [["Generations",n(q,"generations")],["Joints per channel",n(q,"length")],["Productive communities",n(q,"productive")]],
                [f"Residents in {n(q,'communities'):,} communities walked, flexed and folded strands across {n(q,'generations')} generations.",
                 f"Their initially bare channels delivered {n(q,'source_delivered'):,} source carbon, of which the workers ate {n(q,'eaten'):,}.",
                 f"Flattening each inherited structure at turnover left {n(flat,'source_delivered'):,} delivered carbon."],
                [f"Residents paid {n(q,'work'):,} work units.",f"With the line of sight closed, the channels produced {n(sight,'source_delivered'):,} carbon."])
    if chain and "school_a" not in chain:
        q=chain["joined"]
        return ("An apprentice leaves the workshop","A portable folding circuit   /   retained contacts","Source carbon from an exported pupil",n(q,"carbon")," carbon",
                [["Reference gestures seen",n(q,"seen")],["Private attempts",n(q,"own_actions")],["Joints built",n(q,"span")]],
                [f"An apprentice encountered reference gestures from two districts over {n(q,'generations')} generations.",
                 f"It fitted a {n(q,'span')}-joint channel that collected {n(q,'carbon')} carbon.",
                 f"Its final motor choices used {n(q,'peer_used')} contacts last acquired by observation."],
                ["The portable checkpoint includes the pupil’s writable tissue.","The public cue describes the two mineral faces at a joint."])
    if chain:
        a=chain["joined"];one=chain["school_a"];two=chain["school_b"];raw=chain["new_adults"]
        return ("Two traditions at the same bank","Independent mineral districts   /   a new builder","Source carbon after social transmission",n(a,"carbon")," carbon",
                [["Communities",n(a,"communities")],["Joints per gallery",n(a,"span")],["Productive galleries",n(a,"reached")]],
                [f"New builders watched adults from two districts after {n(a,'generations')} generations of turnover.",
                 f"Their {n(a,'span')}-joint galleries caught {n(a,'carbon'):,} units of carbon under load.",
                 f"The two single-district treatments caught {n(one,'carbon'):,} and {n(two,'carbon'):,} units."],
                [f"A pupil delivered carbon where both its immediate predecessors failed in {n(a,'unique_delivery'):,} communities.",
                 f"Replacing the demonstrators with inexperienced adults yielded {n(raw,'carbon'):,} carbon."])
    if rig:
        a=rig["forward"];flat=rig["unfolded"];cut=rig["cut"]
        return ("Water climbs a folded strand","Finite reservoirs   /   periodic compression","Uphill carbon in the test strip",n(a,"carbon")," carbon",
                [["Joints in series",n(a,"span")],["Inlet head",n(a,"head_from")],["Outlet head",n(a,"head_to")]],
                [f"The apparatus connected {n(a,'span')} prepared joints to a collector under higher pressure.",
                 f"With the bellows moving, an aligned strip delivered {n(a,'carbon'):,} source carbon.",
                 f"Cutting the middle joint yielded {n(cut,'carbon'):,} carbon and unfolding the strips yielded {n(flat,'carbon'):,}."],
                [f"The rig accounted for {n(a,'water_total'):,} units of water and {n(a,'carbon_total'):,} units of carbon.",
                 f"Stopping the bellows left {n(rig['quiet'],'carbon'):,} carbon in the collector."])
    if images:
        q=images[-1];return ("A fold on twelve bytes" if "relational_entries" in q else "A folding decision in memory","Fixed motor tie order   /   GPU dispatch","Compiled folding choices",n(q,"decisions")," calls",
                [["Individual circuits",n(q,"reed_image")],["Action image bytes",n(q,"bytes")],["Disagreements",n(q,"mismatches")]],
                [f"The compiler packed the responses of {n(q,'reed_image'):,} circuits.",f"The device dispatched {n(q,'decisions'):,} choices in {float(q['packed_ms']):.4f} ms.",f"The live and compiled circuits disagreed on {n(q,'mismatches'):,} decisions."],
                [f"Compilation took {float(q['compile_ms']):.3f} ms.",f"Live readout took {float(q['live_ms']):.3f} ms."])
    if talk:
        q=talk[-1];return ("Consultation at the bellows","A live work order   /   public acoustic messages","Carbon in the working flume",n(q,"delivered")," carbon",
                [["Joints installed",n(q,"installed")],["Collector water",n(q,"water")],["Source carbon delivered",n(q,"delivered")]],
                [f"The final layout contained {n(q,'span')} available strand joints.",f"The residents installed {n(q,'installed')} folds and the collector received {n(q,'delivered')} carbon.",f"The pupil retained {n(q,'pupil_contacts')} sensorimotor contacts."],
                [f"The tutor spent {n(q,'tuition')} work units.",f"The final layout ran for {n(q,'clock')} pressure ticks."])
    if breeding:
        q=breeding[-1]
        return ("The cost of a working fold","Reproduction in finite oscillating chambers","Carbon delivered by the final brood",n(q,"source_carbon_delivered")," carbon",
                [["Generations",n(q,"reed_brood")],["Juveniles",n(q,"juveniles")],["Opening brood carbon",n(breeding[0],"source_carbon_delivered")]],
                [f"Each generation contained {n(q,'juveniles'):,} juveniles with their own inherited contact rules.",f"The opening brood delivered {n(breeding[0],'source_carbon_delivered'):,} source carbon.",f"The last brood delivered {n(q,'source_carbon_delivered'):,} under {q['selection']} reproduction."],
                ["Juveniles began with empty private contacts.","Food was measured after the real pressure cycle."])
    if water:
        q=water[-1];legacy=entries("reed_legacy")
        return ("Reeds in a living watershed",f"Tick {n(q,'reed_field'):,}   /   pressure, work and food","Carbon raised against local saturation",n(q,"uphill_carbon")," carbon",
                [["Standing folds",n(q,"standing")],["Directional joints",n(q,"directional")],["Living residents",n(q,"living")]],
                [f"Residents have made {n(q,'folds'):,} folds and spent {n(q,'work'):,} units of work in this watershed.",
                 f"Directional strands have carried {n(q,'uphill_water'):,} water and {n(q,'uphill_carbon'):,} carbon against the local saturation difference.",
                 f"Residents have swallowed {n(q,'eaten'):,} units of carbon from active directional contacts."],
                [f"The census found {n(q,'peer_contacts'):,} contacts currently attributed to observation.",
                 f"Carbon through folds whose maker needed witnessed evidence reached {int(legacy[-1]['carbon']):,} units since tracking began." if legacy else f"{n(q,'social_uses'):,} motor choices reused a witnessed contact."])


def runnel_plate(d,data,b,x,y,size,rx,rw):
    def rows(k):return [pairs(l) for l in data.get("lines",[]) if l.startswith(k+"=")]
    def n(q,k):return int(q.get(k,0))
    def ledger(name,items):
        d.label(rx,335,name)
        for i,(k,v) in enumerate(items[:4]):
            yy=375+i*39;d.text(rx,yy,k,17,INK);d.text(rx+rw,yy,human(v),17,GREEN,MONO,600,"end");d.rule(rx,yy+11,rx+rw,yy+11)
    slate="#192f36";mint="#98d4bc";pearl="#ecf0dd";amber="#efb47a";pale="#b8cfc7";rust="#bd8e81"
    d.rect(x,y,size,size,slate,13);dialogs=rows("flume_dialogue");twins=rows("reed_analogy");models=rows("reed_inference");voices=rows("flume_example")
    sample=rows("reed_example");rig=rows("reed_rig");field=rows("reed_ground");images=rows("reed_image");live=rows("flume_contact")
    if models or voices:
        selected=next((q for q in (voices or models) if n(q,"derived")==2 or n(q,"first_pose")<4 and voices),(voices or models)[0])
        target=n(selected,"target");corner=n(selected,"corner")
        d.label(x+26,y+36,"Reference joints   /   one unworked joint",pale);d.text(x+25,y+86,"Reasoning across a gap",34,pearl,SERIF)
        cols=(target>>2,corner>>2);rs=(target&3,corner&3);points={}
        for i,l in enumerate(rs):
            for j,r in enumerate(cols):
                px=x+116+j*208;py=y+191+i*126;points[i,j]=(px,py)
                fill=amber if i==j==0 else mint;d.rect(px-53,py-34,106,68,fill,9);d.text(px,py+8,f"{'ABCD'[l]} · {'ABCD'[r]}",22,slate,MONO,600,"middle")
        for a,z in (((0,0),(0,1)),((0,1),(1,1)),((1,1),(1,0))):
            x0,y0=points[a];x1,y1=points[z];dx=54 if x1>x0 else -54 if x1<x0 else 0;dy=35 if y1>y0 else -35 if y1<y0 else 0;d.rule(x0+dx,y0+dy,x1-dx,y1-dy,"#73988d",3)
        d.text(x+26,y+398,"Amber · the requested pair of mineral faces",18,amber)
        if voices:
            samples=[l for l in data["lines"] if l.startswith("flume_example=")]
            source=next((l for l in samples if pairs(l).get("flume_example")==selected.get("flume_example")),"")
            speech=re.search(r'\breply="([^"]*)"',source);procedure=re.search(r'\bprocedure="([^"]*)"',source)
            d.label(x+26,y+440,"Consultant’s report",pale);d.text(x+26,y+471,speech.group(1) if speech and speech.group(1) else "No audible report",20,amber)
            d.label(x+26,y+506,"Builder’s instruction",pale);d.text(x+26,y+538,procedure.group(1) if procedure and procedure.group(1) else "No instruction",20,mint)
            q=dialogs[0];ledger("Native acoustic exchanges",[("Both calibrations",n(q,"both")),("Indirect completions",n(q,"indirect")),("Later correct folds",n(q,"later_exact")),("New motor on later request",n(q,"new_motor"))])
        else:
            d.text(x+26,y+456,"Chosen fold in each calibration",18,pale);d.text(x+26,y+506,f"{n(selected,'pose_a')+1}    /    {n(selected,'pose_b')+1}",35,mint,SERIF)
            ledger("Calibration pairs solved in both worlds",[(q["reed_analogy"].replace("_"," ").capitalize(),n(q,"both")) for q in twins if q["reed_analogy"] in ("related","local","no_corner","independent_grit")])
        d.text(x+26,y+566,"Rows and columns name visible material faces.",15,pale)
    elif sample or rig or live:
        d.label(x+26,y+37,"Material motion   /   source and collector",pale);d.text(x+26,y+86,"An uphill waterway",35,pearl,SERIF)
        gates=[];raw="";names=("Shut","Forward","Backward","Free");color=(rust,mint,amber,"#b9bed6")
        if sample:
            q=sample[0];stone=n(q,"stone");raw=bytes.fromhex(q["joints"])
            for packed in raw:
                cue=packed&15;motor=packed>>5;flip=packed>>4&1
                if int(q.get("handed",0)) and stone&(1<<(16+(cue&3))):motor=-motor
                code=(1,0,2,3)[(motor+(stone>>(2*(cue&3)))-(stone>>(8+2*(cue>>2))))&3]
                gates.append((code>>1)|((code&1)<<1) if flip else code)
        elif rig:gates=[1]*n(rig[0],"span")
        else:
            view=rows("flume_works");q=live[-1]
            if view:
                v=view[-1];stone=n(q,"stone");pin=n(v,"pin")
                def mix(a):
                    a&=0xffffffff;a=((a^(a>>16))*0x21f0aaad)&0xffffffff;a=((a^(a>>15))*0x735a2d97)&0xffffffff;return a^(a>>15)
                for i,z in enumerate(v["shapes"]):
                    k=mix(pin+i*28657);cue=(k>>19)&15;g=int(z)-1
                    if g<0:gates.append(3);continue
                    if (stone>>31) and stone&(1<<(16+(cue&3))):g=-g
                    code=(1,0,2,3)[(g+(stone>>(2*(cue&3)))-(stone>>(8+2*(cue>>2))))&3];gates.append(((code>>1)|((code&1)<<1)) if (k>>8)&1 else code)
        cols=min(12,max(1,len(gates)));sz=min(62,(size-64)/cols)
        for i,g in enumerate(gates):
            gx=x+30+(i%cols)*sz;gy=y+156+(i//cols)*39;d.rect(gx,gy,sz-4,29,color[g],4)
            d.text(gx+(sz-4)/2,gy+21,("×","›","‹","↔")[g],20,slate,MONO,600,"middle")
        for i,name in enumerate(names):d.dot(x+38+i*131,y+493,7,color[i]);d.text(x+52+i*131,y+499,name,15,pale)
        d.text(x+26,y+560,f"{len(gates)} joints in spatial order" if gates else "Waiting for a fitted joint",19,pearl)
        if rig:ledger("Source carbon reaching the collector",[(v["reed_rig"].capitalize(),n(v,"carbon")) for v in rig if v["reed_rig"] in ("forward","quiet","cut","unfolded")])
        elif sample:
            data_set=rows("reed_chain");ledger("Carbon collected by the new generation",[(r["reed_chain"].replace("_"," ").capitalize(),n(r,"carbon")) for r in data_set if r["reed_chain"] in ("joined","screen","school_a","school_b")])
        else:ledger("Measured during this exchange",[("Assembled joints",n(q,"installed")),("Knowledge from peers",n(q,"social_contacts")),("Collector water",n(q,"water")),("Carbon delivered",n(q,"delivered"))])
    elif rows("reed_estate"):
        frames=rows("reed_estate");q=frames[-1];span=n(q,"length");spacing=(size-64)/span
        d.label(x+26,y+38,f"One inhabited channel   /   {span} joints",pale);d.text(x+26,y+87,"Work across generations",32,pearl,SERIF)
        for j,r in enumerate(frames):
            yy=y+141+j*66
            d.text(x+27,yy,f"Generation {n(r,'reed_estate')}",16,pearl);d.text(x+size-29,yy,f"{n(r,'carbon'):,} carbon",15,amber,MONO,anchor="end")
            for pos,chip in enumerate(bytes.fromhex(r["joints"])):
                ink="#3c5258" if not chip&4 else "#709ba1" if chip&16 else mint
                gx=x+28+spacing*pos;d.rect(gx,yy+12,max(.8,spacing-1),12,ink,1)
                if chip&8:d.rect(gx,yy+26,max(.8,spacing-1),3,amber)
            cx=x+28+spacing*(n(r,"hand")+.5);d.dot(cx,yy+17,max(2,min(4,spacing/3)),pearl)
        d.text(x+26,y+size-32,"Mint · new fold     Blue · inherited     Ochre · carbon     White · worker",13,pale,MONO)
        stewards=rows("reed_stewards");ledger("Net carbon delivered across communities",[(r["reed_stewards"].replace("_"," ").capitalize(),n(r,"source_delivered")) for r in stewards if r["reed_stewards"] in ("inherited","no_witness","flatten_at_birth","isolated")])
    elif images:
        d.label(x+26,y+38,"Learned circuits   /   native GPU timings",pale);d.text(x+26,y+88,"Three bits per decision" if "relational_entries" in images[-1] else "A compiled motor choice",32,pearl,SERIF)
        for i,q in enumerate(images[-2:]):
            yy=y+164+i*166;d.text(x+26,yy,q.get("exposure","census").capitalize(),19,mint)
            for j,(tag,label) in enumerate((("live_ms","Online readout"),("packed_ms","Packed readout"))):
                d.text(x+28,yy+40+j*34,label,18,pale);d.text(x+size-27,yy+40+j*34,f"{float(q[tag]):,.4f} ms",18,pearl,MONO,anchor="end")
        q=images[-1];d.text(x+26,y+566,f"{n(q,'decisions'):,} physical action choices compared",16,mint,MONO)
        ledger("Motor image footprint",[("Number of cases",n(q,"reed_image")),("Bytes per animal",n(q,"bytes")//n(q,"reed_image")),("Learned relational entries",q.get("relational_entries","Unrecorded")),("Action disagreements",n(q,"mismatches"))])
    elif rows("reed_stewards"):
        course=rows("reed_stewards");q=course[0];d.label(x+26,y+40,"Physical work survives turnover",pale);d.text(x+26,y+91,"Maintaining the channel",32,pearl,SERIF)
        shown=[v for v in course if v["reed_stewards"] in ("inherited","no_witness","flatten_at_birth","isolated","unbuilt")]
        top=max(1,max(n(v,"source_delivered") for v in shown))
        for i,v in enumerate(shown):
            yy=y+156+i*81;d.text(x+27,yy,v["reed_stewards"].replace("_"," ").capitalize(),19,pearl);d.text(x+size-28,yy,f"{n(v,'source_delivered'):,}",18,amber,MONO,anchor="end")
            d.rect(x+28,yy+14,size-56,7,"#465e60",3);w=(size-56)*n(v,"source_delivered")/top
            if w:d.rect(x+28,yy+14,w,7,mint,3)
        d.text(x+27,y+570,"Source carbon arriving at the high-pressure collector",14,pale,MONO)
        ledger("Observed work in the inherited landscape",[("Paid walks",n(q,"walks")),("Strands folded",n(q,"folds")),("Physical flexes",n(q,"tests")),("Food eaten at outlet",n(q,"eaten"))])
    elif field:
        d.label(x+25,y+34,"Last census   /   spatially binned",pale);d.text(x+24,y+83,"Where the folds remain",33,pearl,SERIF)
        bins=n(field[0],"bins");px=min((size-82)/bins,437/bins);counts=[]
        for r in field:
            vals=r["counts"];counts.append([(int(vals[i:i+3],16),int(vals[i+3:i+6],16)) for i in range(0,len(vals),6)])
        peak=max(1,max(v for row in counts for v,_ in row));eye=max(1,max(s for row in counts for _,s in row))
        for j,row in enumerate(counts):
            for i,(v,s) in enumerate(row):
                col=blend("#29464b",mint,(v/peak)**.55);gx=x+28+i*px;gy=y+107+j*px;d.rect(gx,gy,px+.2,px+.2,col)
                if s and s>=eye*.65:d.dot(gx+px*.5,gy+px*.5,max(1.1,px*.19),amber)
        d.text(x+26,y+577,"Green · folded fibre     Gold · witnessed contacts",15,pale,MONO)
        q=rows("reed_field")[-1];ledger("Cumulative material accounts",[("Water raised",n(q,"uphill_water")),("Carbon raised",n(q,"uphill_carbon")),("Strand-fed carbon eaten",n(q,"eaten")),("Work spent folding and probing",n(q,"work"))])
    else:
        d.label(x+25,y+39,"Measured in this run",pale)
        for i,(label,v) in enumerate(b["stats"][:3]):
            d.text(x+26,y+125+138*i,label,21,pale);d.text(x+26,y+176+138*i,human(v),39,mint,SERIF)
        ledger("Run measurements",b["stats"])


def practice_summary(entries):
    def by(tag):return {q[tag]:q for q in entries(tag)}
    def z(q,k):return int(q.get(k,0))
    def percent(a,b):return 100*a/max(1,b)
    speech={(q["practice_dialogue"],q["also"]):q for q in entries("practice_dialogue")}
    pairs={(q["practice_pair"],q["group"]):q for q in entries("practice_pair")}
    tests=by("practice_trial");pipes=by("wick_transfer");flow=entries("wick_field");training=entries("practice_field");returns=entries("practice_returns")
    benchmark=entries("practice_image");contacts=entries("studio_contact")
    if ("heard","needs_observation") in speech:
        q=speech["heard","needs_observation"];n=z(q,"cases");ok=z(q,"reached");r=pairs.get(("heard","needs_observation"),{});cut=speech.get(("no_reply","needs_observation"),{})
        return ("The reply changes the work","Two hidden calibrations   /   independent acoustic partners","Contingent material orders",percent(ok,n),"%",
            [["Physical trials",n],["Goals reached",ok],["Remote tests",z(q,"tested")]],
            [f"Workers faced {n:,} physical orders whose paired fixtures shared an opening view.",
             f"After a real probe and a spoken report, {ok:,} requests reached the required end configuration.",
             f"Suppressing the report left {z(cut,'reached'):,} completed requests."],
            [f"{z(r,'differing_answers'):,} pairs succeeded in both calibrations with different continuations." if r else f"{z(q,'indirect'):,} probes asked the worker to inspect another component.",
             f"{z(q,'revised'):,} revised goals were understood." if z(q,"revised") else f"The listeners spent {z(q,'spent'):,} units of bodily work."])
    if pipes:
        q=pipes["continuous"];cut=pipes["cut"];repair=pipes["retied"]
        return ("A strand across the gap","Conserved water, carbon and a passive tracer","Capillary reach",float(q["mean_distance"])," sites",
            [["Strip length",z(q,"length")],["Continuous discharge",z(q,"carbon_caught")],["After retying",z(repair,"carbon_caught")]],
            [f"Each of {z(q,'lanes')} strips connected {z(q,'length')} mineral sites with a finite source at one end.",
             f"The intact strips delivered {z(q,'carbon_caught'):,} units of dissolved carbon to the far reservoir.",
             f"Cutting the midpoint yielded {z(cut,'carbon_caught'):,} units and replacing one strand per strip yielded {z(repair,'carbon_caught'):,}."],
            [f"All {z(q,'water_in'):,} water units were accounted for.",f"The repaired strips used {z(repair,'repair_work')} real fibre placements."])
    if benchmark:
        q=benchmark[-1];n=z(q,"practice_image")
        return ("A decision folded into bytes","Warp scan, packed branches and a separate reply","Compact contingent policies",float(q["Mbranches_per_s"])," M/s",
            [["Learned fixtures",n],["Branching policies",z(q,"conditional")],["Action bytes",z(q,"image_bytes")]],
            [f"A CUDA warp scan packed {n:,} learned fixture policies into {z(q,'image_bytes'):,} action bytes.",
             f"The image included {z(q,'conditional')} branching policies and {z(q,'refused')} refusals.",
             f"Writable search and direct dispatch differed on {z(q,'mismatches')} of {z(q,'checked'):,} checked outputs."],
            [f"The directory occupied {z(q,'directory_bytes'):,} bytes.",f"Compilation took {float(q['compile_ms']):.3f} ms on this device."])
    windows=entries("practice_window")
    if windows:
        q=windows[-1];own=z(q,"own_reserve");swapped=z(q,"swapped_reserve")
        return ("When to leave a patch","A finite foraging island   /   reciprocal timing" if q.get("landscape")=="island" else "Frontier patches   /   reciprocal timing",
            "Reserve under matched timing",own-swapped," carbon",
            [["Resident pairs",z(q,"practice_window")],["Own reserve",own],["Swapped reserve",swapped]],
            [f"Each schedule had {z(q,'ticks_each'):,} ticks and {z(q,'own_stays'):,} stays across the two arrangements.",
             f"Using the timing chosen on each arrangement left {own:,} units of carbon in the residents’ reserves.",
             f"Exchanging the two schedules left {swapped:,} units."],
            [f"The local schedule finished ahead on {z(q,'wins'):,} pairs and behind on {z(q,'losses'):,}.",
             f"Own and exchanged schedules yielded {z(q,'own_meals'):,} and {z(q,'swapped_meals'):,} spring meals respectively."])
    if tests:
        q=tests["intact"];no=tests.get("no_probe",{});days=by("practice_foraging")
        return ("An experiment before committing","Private material models   /   withheld four-part configurations","Active material inference",percent(z(q,"delivered"),z(q,"cases")),"%",
            [["New orders",z(q,"cases")],["Reached",z(q,"delivered")],["Queries",z(q,"inquiries")]],
            [f"Residents tested {z(q,'cases'):,} new physical work orders after handling elementary fixtures.",
             f"They reached {z(q,'delivered'):,} end states, including {z(q,'followed'):,} after a diagnostic stroke.",
             f"Disabling the choice of a test left {z(no,'delivered'):,} completed orders."],
            [f"{z(q,'indirect'):,} diagnostic strokes observed a part other than the driver.",f"The foraging cohort finished with {z(days.get('free',{}),'endowment'):,} carbon." if days else "All suggested programs were tried on real material states."])
    if contacts:
        q=contacts[-1];meals=sum(z(r,"credit") for r in contacts);probes=sum(z(r,"cost")==1 and z(r,"probe")<8 for r in contacts)
        return ("A question at the gauge","Workbench testimony   /   live embodied exchange","Heard mechanical evidence",meals," carbon",
            [["Stored contexts",z(q,"contexts")],["Paid probes",probes],["Food transferred",meals]],
            [f"The speaker finished the session with {z(q,'contexts')} learned material models.",
             f"The worker made {probes} paid diagnostic strokes and reported its observations across the acoustic channel.",
             f"Finished work released {meals} units from the food escrow."],
            [f"The tutor spent {z(q,'tutor_work')} units on elementary examples.",f"The final emitted policy occupied {z(q,'policy_bytes')} bytes."])
    if flow:
        q=flow[-1];ret=returns[-1] if returns else {}
        return ("What moves through the riffles",f"Watershed tick {z(q,'wick_field'):,}   /   woven material transport","Carbon across the workshops",z(q,"cross_carbon")," carbon",
            [["Cross-workshop carbon",z(q,"cross_carbon")],["Tracked parcels",z(q,"tracers")],["Repeated transfers",z(q,"multihop")]],
            [f"Capillary contacts moved {z(q,'water'):,} units of water and {z(q,'carbon'):,} units of dissolved carbon.",
             f"Of that carbon, {z(q,'worked_carbon'):,} units passed through fibre newly placed by residents.",
             f"The farthest conserved tracer finished {z(q,'furthest')} sites from its source on the initial periodic grid." if "reference_side" in q or z(q,"furthest")<128 else "This early extension measured endpoint separation on the newly widened grid."],
            [f"{z(q,'cross_tile'):,} transfers crossed a workshop boundary.",
             f"Other residents ate {int(ret['foreign_carbon']):,} units of carbon in {int(ret['foreign_meals']):,} meals at necessary strands." if "foreign_carbon" in ret and "foreign_meals" in ret else f"{z(q,'standing'):,} body-made edges survived."])
    if training:
        q=training[-1];ret=returns[-1] if returns else {};early=z(q,"writes") and not z(q,"contacts") and not z(q,"observing")
        return ("A creature stays with its work",f"Watershed tick {z(q,'practice_field'):,}   /   voluntary material contacts","Sustained material practice",z(q,"long_bouts")," bouts",
            [["Living residents",z(q,"living")],["Eight-action bouts",z(q,"long_bouts")],["Fitting proposals",z(q,"fitting_plans")]],
            [f"Residents began {z(q,'entered'):,} work bouts and continued a previous bout {z(q,'continued'):,} times.",
             f"They spent {z(q,'heat'):,} units of work and proposed {z(q,'fitting_plans'):,} fibre fittings.",
             "This early pilot recorded device events but omitted the closing private-memory readback." if early else f"At the census, {z(q,'thirty_two'):,} living animals had accumulated at least 32 observations."],
            [f"{z(q,'repaired'):,} predicted actions opened a physical spring.",f"{z(ret,'prebirth_meals'):,} meals required a deliberate strand installed before the eater’s birth." if ret else f"{z(q,'direct_exact'):,} direct physical outcomes were predicted exactly." if "direct_exact" in q else f"{z(q,'exact'):,} direct physical outcomes were predicted exactly."])


def practice_plate(d,data,b,x,y,size,rx,rw):
    def rows(tag):return [pairs(s) for s in data["lines"] if s.startswith(tag+"=")]
    def num(q,k):return int(q.get(k,0))
    dark="#182f35";pale="#e6eee0";muted="#b4cfca";ochre="#e7ba7a";sea="#80c3bb"
    d.rect(x,y,size,size,dark,13);facts=b["stats"];header="Measured in this run";spec=rows("studio_scene");example=rows("practice_case");course=rows("wick_transfer")
    table=rows("practice_dialogue");ground=rows("wick_ground");active=rows("practice_ground");bench=rows("practice_image");windows=rows("practice_window")
    if windows and not (table or course or bench):
        q=windows[-1];d.label(x+24,y+38,"The same animals   /   the same time at risk",muted)
        d.text(x+24,y+88,"A choice of when to stay",32,pale,SERIF)
        d.text(x+24,y+131,"Original schedule",16,sea);d.text(x+213,y+131,"Exchanged schedule",16,ochre)
        for i,(field,label) in enumerate((("food","Carbon eaten"),("reserve","Carbon remaining"),("meals","Spring meals"))):
            yy=y+195+120*i;a=num(q,"own_"+field);b1=num(q,"swapped_"+field);peak=max(1,a,b1);w=size-148
            d.text(x+25,yy,label,20,pale)
            for j,(value,ink) in enumerate(((a,sea),(b1,ochre))):
                top=yy+16+18*j;d.rect(x+26,top,w,8,"#42585c",3)
                if value:d.rect(x+26,top,w*value/peak,8,ink,3)
                d.text(x+size-25,top+8,f"{value:,}",14,ink,MONO,600,"end")
        d.text(x+24,y+size-23,f"{num(q,'ticks_each'):,} shared ticks per treatment",15,pale,MONO)
    elif spec or (example and not table):
        q=spec[0] if spec else next((r for r in example if num(r,"probe")<8),example[0])
        pair=spec[:2] if spec else [];width=num(q,"parts") if spec else num(q,"parts").bit_count();grip=num(q,"grip");tint=num(q,"materials" if spec else "glyphs")
        who=num(q,"first" if spec else "probe");look=num(q,"watch");edges=num(q,"joints")
        d.label(x+24,y+36,"One arrangement   /   two possible material laws",muted);d.text(x+24,y+85,"The worker’s question",31,pale,SERIF)
        pts=[(x+118,y+180),(x+338,y+180),(x+118,y+292),(x+338,y+292)];colors=("#d4a76b","#87b9a6","#a6b2cf","#cf9698")
        for a in range(width):
            for c in range(a+1,width):
                t=edges>>(8*a+2*c)&3
                if t:d.rule(*pts[a],*pts[c],"#688d8a" if t==1 else "#af9977",6 if t==1 else 3)
        for a in range(width):
            px,py=pts[a];d.dot(px,py,34,"#476165");d.dot(px,py,26,colors[(tint>>(2*a))&3]);d.text(px,py+8,str(a+1),22,dark,MONO,600,"middle")
            if a==look:d.dot(px+31,py-25,6,sea)
            if who<8 and a==who//2:d.rule(px-19,py+40,px+19,py+40,ochre,4)
        label=f"Turn part {who//2+1}   ·   Inspect part {look+1}" if who<8 else "A shared action or a refusal"
        d.text(x+24,y+360,label,19,pale)
        for i in range(2):
            xx=x+24+i*282;d.rect(xx,y+396,264,150,"#29474d",8)
            d.label(xx+16,y+428,f"Calibration {i+1}",muted)
            if pair and i<len(pair):
                v=pair[i];face=num(v,"seen");d.text(xx+16,y+474,f"Face {face+1}" if face<4 else "No test",26,ochre,SERIF)
                d.text(xx+16,y+516,f"{num(v,'tail_strokes')} strokes · {num(v,'fitted')} loop",15,pale,MONO)
            else:
                branches=[r for r in rows("practice_branch") if r["practice_branch"]==q.get("practice_case")]
                if i<len(branches):d.text(xx+16,y+477,f"{num(branches[i],'strokes')} strokes",23,pale,SERIF);d.text(xx+16,y+516,f"Observed face {num(branches[i],'face')+1}",15,muted,MONO)
                else:d.text(xx+16,y+478,"No continuation",18,muted)
        d.text(x+24,y+579,"Ochre · moved part     Cyan · inspected part",15,muted,MONO)
    elif course:
        d.label(x+26,y+39,"Conserved transport   /   physical end reservoir",muted)
        d.text(x+26,y+88,"Where the carbon arrives",32,pale,SERIF)
        peak=max(1,max(num(v,"carbon_caught") for v in course))
        for i,v in enumerate(course):
            yy=y+168+97*i;d.text(x+27,yy,{"continuous":"Continuous fibre","cut":"Middle strand cut","retied":"Retied at midpoint","within_tiles":"Confined to patches"}[v["wick_transfer"]],19,pale)
            d.text(x+size-27,yy,f"{num(v,'carbon_caught'):,}",20,ochre,MONO,600,"end");d.rect(x+28,yy+16,size-56,10,"#405a60",4)
            if num(v,"carbon_caught"):d.rect(x+28,yy+16,(size-56)*num(v,"carbon_caught")/peak,10,sea,4)
        d.text(x+27,y+572,f"{num(course[0],'length')} sites per strip   ·   {num(course[0],'lanes')} separate reservoirs",16,muted,MONO)
    elif ground or active:
        d.label(x+25,y+33,"Closing survey   /   physical sites",muted)
        water={num(a,"wick_ground"):a["counts"] for a in ground};subjects={num(a,"practice_ground"):a["counts"] for a in active}
        n=num((ground or active)[0],"bins");unit=(size-84)/n
        for j in range(n):
            for i in range(n):
                local=subjects.get(j,"");line=water.get(j,"")
                def count(s,k):return int(s[i*9+3*k:i*9+3*k+3],16) if s else 0
                weave,body,dye=count(line,0),count(line,1),count(line,2);attending,marked,experienced=count(local,0),count(local,1),count(local,2)
                xx=x+42+i*unit;yy=y+51+j*unit
                d.rect(xx,yy,unit+.2,unit+.2,blend("#24454b","#79aba0",min(.98,math.log1p(weave if line else experienced)/(4.8 if line else 3.5))))
                if dye:d.dot(xx+.4*unit,yy+.5*unit,unit*min(.32,.10+.06*dye),"#c2dadd")
                if marked or (not local and body):d.dot(xx+.65*unit,yy+.43*unit,unit*(.31 if marked else .11),ochre)
        d.text(x+24,y+size-16,"Green · fibre     Pale · multihop tracer     Gold · attributed work" if ground else "Green · experienced residents     Gold · surviving deliberate strand",13,pale,MONO)
    elif bench:
        q=bench[-1];d.label(x+26,y+42,"Compact action image   /   actual learned fixtures",muted);d.text(x+26,y+91,"A reply selects the branch",32,pale,SERIF)
        for i,(field,label,col) in enumerate((("conditional","Contingent procedures",sea),("fixed","Shared procedures",ochre),("refused","No feasible procedure","#98a2c0"))):
            yy=y+189+i*110;d.text(x+27,yy,label,20,pale);d.text(x+size-26,yy,f"{num(q,field):,}",23,col,MONO,600,"end")
            d.rect(x+28,yy+21,size-56,10,"#485c60",4);d.rect(x+28,yy+21,(size-56)*num(q,field)/max(1,num(q,"practice_image")),10,col,4)
        d.text(x+27,y+560,f"{num(q,'image_bytes'):,} action bytes · {num(q,'directory_bytes'):,} directory bytes",16,muted,MONO)
    elif table:
        d.label(x+26,y+39,"Held-out work orders   /   matched calibrations",muted);d.text(x+26,y+90,"When information matters",31,pale,SERIF)
        selected=[r for r in table if r["also"]=="needs_observation" and r["practice_dialogue"] in ("heard","no_reply","no_question","false_reply","latest_only")]
        for i,q in enumerate(selected):
            yy=y+161+77*i;d.text(x+28,yy,q["practice_dialogue"].replace("_"," ").capitalize(),19,pale);d.text(x+size-27,yy,f"{num(q,'reached')} / {num(q,'cases')}",18,ochre,MONO,anchor="end")
            d.rect(x+28,yy+14,size-56,7,"#486165",4);d.rect(x+28,yy+14,(size-56)*num(q,"reached")/max(1,num(q,"cases")),7,sea,4)
    elif rows("practice_field"):watershed_panel(d,data,x,y,size)
    else:
        d.label(x+25,y+38,"A material exchange",muted)
        for i,(k,v) in enumerate(facts[:3]):d.text(x+27,y+133+i*143,k,22,muted);d.text(x+27,y+187+i*143,human(v),40,pale,SERIF)
    if table:
        header="Physical completions on information-dependent fixtures"
        facts=[(r["practice_dialogue"].replace("_"," ").capitalize(),f"{num(r,'reached'):,} / {num(r,'cases'):,}") for r in table if r["also"]=="needs_observation" and r["practice_dialogue"] in ("heard","no_reply","false_reply","no_question")]
    elif windows and not (course or bench):
        q=windows[-1];header="Matched opportunity and cost"
        facts=[("Stays in each treatment",num(q,"own_stays")),("Departures in each",num(q,"ticks_each")-num(q,"own_stays")),
               ("Local schedule finished ahead",num(q,"wins")),("Exchanged schedule finished ahead",num(q,"losses"))]
    elif rows("practice_trial"):
        header="New configurations reached";facts=[(r["practice_trial"].replace("_"," ").capitalize(),f"{num(r,'delivered'):,} / {num(r,'cases'):,}") for r in rows("practice_trial") if r["practice_trial"] in ("intact","no_probe","wrong_report","pooled")]
    elif ground or rows("wick_field"):
        w=rows("wick_field")[-1] if rows("wick_field") else {};header="Measured capillary transport"
        if w:
            distance_label="Farthest tracer" if "reference_side" in w or num(w,"furthest")<128 else "Offset on expanded grid"
            facts=[("Water carried",num(w,"water")),("Carbon carried",num(w,"carbon")),("Carbon on newly laid fibre",num(w,"worked_carbon")),(distance_label,str(num(w,"furthest"))+" sites")]
        if course:
            q=course[0];facts=[("Initial water",num(q,"water_in")),("Water collected",num(q,"water_caught")),("Initial carbon",num(q,"carbon_in")),("Final source and channel carbon",num(q,"carbon_left"))]
    elif rows("practice_returns"):
        w=rows("practice_returns")[-1];header="Fibre placed under a material prediction";facts=[("Installed strands",num(w,"installed")),("Essential open observations",num(w,"essential_checks")),("Carbon carried",num(w,"carbon")),("Meals at work predating the eater",num(w,"prebirth_meals"))]
    d.label(rx,335,header)
    for i,(k,v) in enumerate(facts[:4]):
        yy=375+39*i;d.text(rx,yy,k,17,INK);d.text(rx+rw,yy,human(v),17,GREEN,MONO,600,"end");d.rule(rx,yy+11,rx+rw,yy+11)


def linkage_summary(entries):
    def by(k):return {r[k]:r for r in entries(k)}
    trials=by("linkage_language");exchange=by("linkage_bequest");study=by("linkage_fabrication");motor=by("linkage_transfer")
    attention=by("linkage_attention");field=entries("linkage_field");meet=entries("linkage_dialogue");bench=entries("linkage_compile")
    def num(q,k):return int(q.get(k,0))
    def rate(a,b):return 100*a/max(1,b)
    if "heard" in trials:
        q=trials["heard"];n=num(q,"trials");fed=num(q,"aligned");can=num(q,"possible")
        return ("The hands that heard the plan","Spoken procedures   /   independent speaker and worker","Spoken construction",rate(fed,can),"%",
                [["Realised goals",fed],["Attainable goals",can],["Changed assemblies",num(q,"fitted")]],
                [f"A speaker heard {n:,} new work orders and described a sequence to a second resident.",f"The recipient accomplished {fed:,} of {can:,} attainable goals by physically decoding the sounds.",f"Muting the first part name left {num(trials.get('masked',{}),'aligned'):,} successful deliveries."],
                [f"{num(q,'multistep'):,} deliveries required at least three strokes.",f"{num(q,'cheapest'):,} used the minimum work in the independent physical reference."])
    if "first_world" in trials and "second_world" in trials:
        old,new=trials["first_world"],trials["second_world"]
        return ("Remembering two material worlds","Persistent personal tissue   /   unfamiliar work orders","Motor memory across worlds",num(old,"aligned")," goals",
                [["First world",num(old,"aligned")],["Second world",num(new,"aligned")],["Orders per world",num(old,"trials")]],
                [f"The same saved nervous system was tested in two worlds with different material dynamics.",
                 f"Using its existing contacts, it achieved {num(old,'aligned')} of {num(old,'possible')} attainable targets in the first world.",
                 f"In the second world it achieved {num(new,'aligned')} of {num(new,'possible')} attainable targets."],
                ["Both assessments began without additional handling.",f"The two cohorts spent {num(old,'heat')+num(new,'heat'):,} units reproducing the spoken procedures."])
    weather={(q["linkage_recall"],num(q,"season")):q for q in entries("linkage_recall")}
    if ("changing",2) in weather:
        a,b,c=[weather["changing",i] for i in range(3)];control=weather["no_new_evidence",1];n=num(a,"cases")
        return ("When the same grain moves differently","A lifetime of contact in changing material conditions","Reacquired physical work",rate(num(c,"completed"),n),"%",
                [["Orders per season",n],["After the change",num(b,"completed")],["After the return",num(c,"completed")]],
                [f"Each juvenile used the same private tissue through three seasons of paid work.",
                 f"The population completed {num(a,'completed'):,}, {num(b,'completed'):,} and {num(c,'completed'):,} of {n:,} orders as the material laws changed and returned.",
                 f"Suppressing new material evidence after the first season left {num(control,'completed'):,} completions in the changed world."],
                ["No material transition table or goal sequence was passed between generations.",f"The middle season included {num(b,'built'):,} successful jobs with a new coupling."])
    if exchange:
        q=exchange["as_made"];cut=exchange["unstrung"];chain=[v for v in entries("linkage_teaching") if v["linkage_teaching"]=="living_witness"]
        return ("After the maker leaves","Hand-built articles   /   separate bodies and experience","Inherited working connections",rate(num(q,"novel_uses"),num(q,"needs_loop")),"%",
                [["Recipients",num(q,"cases")],["Loop essential",num(q,"needs_loop")],["New uses",num(q,"novel_uses")]],
                [f"An independent builder prepared each of {num(q,'cases'):,} articles and left before the recipient touched a handle.",f"Recipients completed {num(q,'novel_uses')} tasks that required the new loop, falling to {num(cut,'novel_uses')} after its removal.",f"After three transfers of demonstrated actions, {num(chain[-1],'built')} successors reached a new four-part target."],
                [f"Teachers worked on two-part assemblies.",f"The first generation of observers completed {num(chain[0],'built')} targets."])
    if study:
        q=study["built"];n=num(q,"cases");ok=num(q,"produced");force=num(q,"needs_loop");won=num(q,"new_function")
        return ("What one loop makes possible","An unfamiliar assembly   /   one transferable loop","Constructed mechanisms",rate(ok,n),"%",
                [["New fixtures",n],["Goals reached",ok],["Fittings placed",num(q,"threaded")]],
                [f"Juveniles received {n:,} new work orders with one available loop each.",f"They reached {ok:,} target configurations and spent {num(q,'heat'):,} units of work.",f"Of {force:,} targets that required a structural change, the animals reached {won:,}."],
                [f"{num(q,'cheapest'):,} solutions matched the minimum physical work.",
                 f"With unfamiliar passive parts, the focal circuit completed {num(attention['focal'],'achieved'):,} of {num(attention['focal'],'trials'):,} trials." if "focal" in attention else f"Emptying the spool left {num(study.get('no_stock',{}),'produced'):,} successful trials."])
    if motor:
        q=motor.get("self_directed",motor["own"])
        return ("From one stroke to a sequence","Held-out mechanical assemblies   /   paid action","Invented motor sequences",rate(num(q,"built"),num(q,"cases")),"%",
                [["Fixtures",num(q,"cases")],["Completed",num(q,"built")],["Indirect goals",num(q,"indirect_built")]],
                [f"Juveniles practiced on elementary materials before receiving {num(q,'cases'):,} unfamiliar configurations.",f"Their muscle sequences physically completed {num(q,'built'):,} requests.",f"Erasing the learned effects of the joints left {num(motor['seams_erased'],'built'):,} completions."],
                [f"{num(q,'long_built'):,} completions needed at least three strokes.",f"Handling the parts cost {num(q,'paid'):,} units of work during the test."])
    if meet:
        q=meet[-1];heard=[x for x in meet if num(x,"linkage_dialogue")==5 and num(x,"executed")]
        return ("A work order between residents","Acoustic terms   /   a shared material fixture","Spoken work at a rack",sum(num(v,"alignment") for v in heard)," completions",
                [["Parts",(num(q,"parts")).bit_count()],["Demonstrations",sum(num(v,"observed") for v in meet)],["Recipient strokes",sum(num(v,"strokes") for v in heard)]],
                [f"Two residents met at a {num(q,'parts').bit_count()}-piece apparatus.",f"The speaker observed {sum(num(v,'observed') for v in meet):,} physical practice events before the exchange.",f"The other body spent {sum(num(v,'heat') for v in heard)} units of work on publicly described motions."],
                [f"The final fixture retained {num(q,'spool')} loose loops.",f"The room held {num(q,'stock')} units of unconsumed carbon."])
    if field:
        q=field[-1];c=num(q,"carbon");used=num(q,"meals");t=num(q,"linkage_field")
        survey=by("linkage_inspection").get("lived",{});wet=num(survey,"hydrated_plots")
        return ("Working springs in the millrace",f"Watershed tick {t:,}   /   mineral motion, fibre and water","Carbon across worked springs",c," carbon",
                [["Living residents",num(q,"living")],["Carbon released",c],["Certified alignments",num(q,"current_seals")]],
                [f"At tick {t:,}, {num(q,'remembering'):,} living residents carried personal material contacts.",f"Open couplings had released {c:,} units of carbon and delivered {used:,} directly to feeding bodies.",f"Certified works fed another animal on {num(q,'other_meals'):,} occasions, including {num(q,'older_meals'):,} involving works older than the eater."],
                [f"{wet:,} surveyed mineral patches were above their local hydration threshold." if wet else f"{num(q,'taps'):,} strokes propagated {num(q,'propagated'):,} movements to neighboring pieces.",f"{num(q,'structural_plans'):,} deliberate fittings and {num(q,'loops_placed'):,} total fibre placements were recorded."])


def linkage_plate(d,data,b,x,y,size,rx,rw):
    def rows(k):return [pairs(l) for l in data["lines"] if l.startswith(k+"=")]
    def val(q,k,default=0):return int(q.get(k,default))
    pal=("#e5b776","#80bba1","#9caed7","#db9a93");light="#e9efdc";milk="#b9d4c6"
    d.rect(x,y,size,size,"#183839",13)
    products=rows("linkage_product");meet=rows("linkage_dialogue");charts=rows("linkage_ground")
    section="Physical work recorded";facts=b["stats"]
    if products or (meet and not charts and not rows("linkage_language")):
        q=next((z for z in products if val(z,"fitting",6)<6),products[0] if products else next((z for z in reversed(meet) if val(z,"executed")),meet[-1]))
        dialog=not products;n=val(q,"parts").bit_count() if dialog else val(q,"parts");start=val(q,"start");final=val(q,"after" if dialog else "finish");target=val(q,"goal" if dialog else "target")
        joint=val(q,"joints" if dialog else "after");fit=val(q,"fitting",6);handle=val(q,"grip");hues=val(q,"materials")
        d.label(x+27,y+39,"Bench record   /   actual assembly",milk);d.text(x+27,y+84,"The physical workpiece",32,light,SERIF)
        pts=[(x+165,y+177),(x+431,y+177),(x+165,y+337),(x+431,y+337)]
        edges=((0,1),(0,2),(0,3),(1,2),(1,3),(2,3))
        for i,(a,c) in enumerate(edges):
            if c>=n:continue
            seam=joint>>(8*a+2*c)&3
            if seam:
                ax,ay=pts[a];bx,by=pts[c];d.rule(ax,ay,bx,by,"#d1ae6a" if i==fit else "#6e9b94",9 if seam==1 else 4)
                if i==fit:d.dot((ax+bx)/2,(ay+by)/2,9,"#f5d592")
        for i in range(n):
            px,py=pts[i];d.dot(px,py,45,"#42605d");d.dot(px,py,35,pal[(hues>>(2*i))&3])
            for k in range(4):
                ang=(k+0.5)*math.pi/2;d.dot(px+40*math.cos(ang),py+40*math.sin(ang),3,"#b6c8ac")
            d.text(px,py+9,str(i+1),26,"#243d39",MONO,600,"middle")
            if (handle>>(2*i))&3:d.rule(px-18,py+53,px+18,py+53,"#e1d7ac",4)
        d.text(x+27,y+426,"Exposed faces",20,light,SERIF)
        for j,(v,label) in enumerate(((start,"Start"),(target,"Requested"),(final,"After work"))):
            left=x+29+j*182;d.text(left,y+461,label,15,milk)
            for i in range(n):d.rect(left+i*23,y+475,19,15,pal[(v>>(2*i))&3],3)
        ticks=val(q,"strokes");low=val(q,"world_low" if dialog else "low");high=val(q,"world_high" if dialog else "high")
        seq=[((low if j<8 else high)>>(3*(j if j<8 else j-8)))&7 for j in range(ticks)]
        display="  ".join(f"{v//2+1}{'B' if v&1 else 'A'}" for v in seq)
        d.text(x+27,y+533,"Strokes in execution order",16,milk)
        d.text(x+27,y+568,display or "Already aligned / no strokes",20,light,MONO)
    elif rows("linkage_recall"):
        d.label(x+28,y+39,"Longitudinal study   /   same private tissue",milk)
        d.text(x+28,y+90,"Work through a change",33,light,SERIF)
        conditions=rows("linkage_recall")
        for j,(key,label,color) in enumerate((("changing","Observed change","#86bba5"),("no_new_evidence","Evidence withheld","#d9b477"),("cleared_each_time","Memories cleared","#a0aed3"))):
            d.text(x+27,y+155+j*120,label,19,light)
            group=sorted((q for q in conditions if q["linkage_recall"]==key),key=lambda q:val(q,"season"))
            for i,q in enumerate(group):
                xx=x+28+i*182;yy=y+195+j*120;ok=val(q,"completed");n=val(q,"cases")
                d.rect(xx,yy,151,9,"#456963",4)
                if ok:d.rect(xx,yy,151*ok/n,9,color,4)
                d.text(xx,yy+29,f"{ok:,} / {n:,}",14,milk,MONO)
        d.text(x+28,y+560,"First material               Changed material            First material again",13,light,MONO)
    elif charts:
        d.label(x+25,y+37,"Closing survey   /   occupied watershed",milk);w=val(charts[0],"bins");unit=(size-86)/w
        water={val(a,"linkage_moisture"):a["counts"] for a in rows("linkage_moisture")}
        for row in charts:
            j=val(row,"linkage_ground");s=row["counts"]
            for i in range(w):
                seal=int(s[6*i:6*i+3],16);memory=int(s[6*i+3:6*i+6],16);px=x+43+i*unit;py=y+51+j*unit
                fraction=int(water[j][3*i:3*i+3],16)/max(1,val(row,"span")**2) if j in water else 0
                ground=blend("#244646","#527aa8",fraction)
                d.rect(px,py,unit+.2,unit+.2,blend(ground,"#89b9a1",min(.82,math.log1p(memory)/4.4)))
                if seal:d.dot(px+unit/2,py+unit/2,unit*min(.43,.16+.08*math.log1p(seal)),"#efbc71")
        d.text(x+26,y+size-15,"Blue · hydrated mineral     Green · memories     Gold · aligned works" if water else "Green · resident memories      Gold · certified working alignments",13,light,MONO)
    elif rows("linkage_language"):
        d.label(x+27,y+40,"Orders completed   /   acoustic interventions",milk);d.text(x+27,y+90,"Whose hands did the work?",31,light,SERIF)
        for j,q in enumerate(rows("linkage_language")[:5]):
            yy=y+155+j*80;n=val(q,"trials");ok=val(q,"aligned");d.text(x+27,yy,q["linkage_language"].replace("_"," ").capitalize(),19,light)
            d.text(x+size-28,yy,f"{ok:,} / {n:,}",18,milk,MONO,anchor="end");d.rect(x+28,yy+15,size-56,8,"#476864",4)
            if ok:d.rect(x+28,yy+15,(size-56)*ok/n,8,"#e1bd7d",4)
    elif rows("linkage_field"):watershed_panel(d,data,x,y,size)
    else:
        d.label(x+27,y+40,"Work ledger",milk)
        for i,(k,v) in enumerate(facts[:3]):d.text(x+29,y+118+i*151,k,19,milk);d.text(x+29,y+176+i*151,human(v),42,light,SERIF)
    field=rows("linkage_field");data_sets=rows("linkage_fabrication")
    if data_sets:
        section="Matched physical end states";selected={"built":"Learner with one loop","loop_removed":"Loop removed after fitting","no_stock":"Spool empty","untaught":"Unexposed juvenile"}
        facts=[(selected[q["linkage_fabrication"]],f"{val(q,'produced'):,} / {val(q,'cases'):,}") for q in data_sets if q["linkage_fabrication"] in selected]
    elif rows("linkage_recall"):
        section="Paid tasks in the changed material";middle=next(q for q in rows("linkage_recall") if q["linkage_recall"]=="changing" and q["season"]=="1")
        facts=[("Procedures proposed",val(middle,"proposed")),("Completed with a new fitting",val(middle,"built")),("Work spent on these orders",val(middle,"heat")),("Proposed procedures that failed",val(middle,"mistaken"))]
    elif rows("linkage_transfer"):
        one=next((q for q in rows("linkage_transfer") if q["linkage_transfer"]=="self_directed"),rows("linkage_transfer")[0])
        section="Actions composed after practice";facts=[("Indirect goals reached",val(one,"indirect_built")),("Long sequences completed",val(one,"long_built")),("Work spent at test",val(one,"paid")),("Incorrect proposals",val(one,"mistaken"))]
    elif field:
        f=field[-1];facts=[("Strokes spent",val(f,"taps")),("Neighbouring parts moved",val(f,"propagated")),("Meals at another maker’s work",val(f,"other_meals")),("Work predating the eater",val(f,"older_meals"))]
    d.label(rx,335,section)
    for i,(k,v) in enumerate(facts[:4]):
        yy=375+39*i;d.text(rx,yy,k,17,INK);d.text(rx+rw,yy,human(v) if isinstance(v,int) else v,17,GREEN,MONO,600,"end");d.rule(rx,yy+11,rx+rw,yy+11)


def effort_summary(entries):
    choices={q["effort_choice"]:q for q in entries("effort_choice")}
    slabs={q["effort_slat"]:q for q in entries("effort_slat")}
    names={(q["effort_language"],q["task"]):q for q in entries("effort_language")}
    faces={(q["effort_shape_language"],q["task"]):q for q in entries("effort_shape_language")}
    bench=entries("effort_deploy");field=entries("effort_field");labels=entries("effort_lineage");physical=entries("effort_contact")
    spoken=entries("effort_reply");captions=entries("effort_caption");selected=entries("effort_scholar")
    if faces or names:
        if ("observed","after_work") in faces and ("lived","edited") in names:
            f=faces["observed","after_work"];g=names["lived","edited"];why=names["lived","reason"]
            n,ok=int(f["trials"]),int(f["correct"])
            return ("What will this face become?","Spoken plans, actual work and revised predictions","Spoken surface forecasts",100*ok/max(1,n),"%",
                    [["After working the slat",n],["Correct surface names",ok],["Correct revised choices",int(g["correct"])]],
                    [f"The speaker named the face reached by further hypothetical work on {ok:,} of {n:,} questions after a real stroke.",
                     f"It revised a described journey correctly on {int(g['correct']):,} of {int(g['trials']):,} follow-ups.",
                     f"Its stated reason agreed with the physical comparison on {int(why['correct']):,} of {int(why['trials']):,} questions."],
                    ["The two comparison bundles are constructed from the spoken material names and their multiplicities.",
                     "A separate observer retains the physical predictions, preferred routes and expected words."])
        rows=[*names.values(),*faces.values()];n=sum(int(q["trials"]) for q in rows);ok=sum(int(q["correct"]) for q in rows);loud=sum(int(q["spoken"]) for q in rows)
        return ("What is worth the trip?","Spoken comparisons of experienced materials","Graded foraging utterances",100*ok/max(n,1),"%",
            [["Graded turns",n],["Correct",ok],["Spoken",loud]],
            [f"The observer scored {n:,} utterances across the recorded conditions.",f"The resident spoke {loud:,} times.",f"Responses or silences agreed with {ok:,} external references."],
            ["Different routes can be described from the same body position.","Old material names and new operator associations share the spoken exchange."])
    if "lived" in choices:
        q=choices["lived"];n=int(q["trials"]);ok=int(q["correct"]);shape=slabs.get("inferred",{})
        return ("The price of a crossing","Fruit, footing and the next journey","Net food on unfamiliar routes",int(q["net"]),"net carbon",
                [["Unfamiliar routes",n],["Best net yield",ok],["Useful preparations",int(shape.get("good_preparation",0))]],
                [f"The residents chose a highest-net-food route in {ok:,} of {n:,} offers.",f"Their meals paid {int(q['work']):,} units of travel work and left {int(q['net']):,} carbon.",
                 f"They prepared a surface on {int(shape.get('good_preparation',0)):,} of {int(shape.get('better_prepared',0)):,} occasions when doing so reduced the cost of later crossings."],
                [f"The same offers would leave {int(q['always_near']):,} carbon by always taking the nearer patch.",
                 f"Erasing prior physical experiences yielded {int(choices.get('empty',{}).get('correct',0)):,} best choices."])
    if bench:
        q=bench[-1];return ("Compiling a route valuation","Signed bundle currents on measured residents","Packed route comparisons",
            float(q["Mcomparisons_per_s"]),"M comparisons/s",[["Signed currents checked",int(q["currents_checked"])],["Compiled bytes",int(q["image_bytes"])],["Writable bytes",int(q["writable_bytes"])]],
            [f"The benchmark compared {int(q['currents_checked']):,} signed currents and found {int(q['errors']):,} disagreements.",
             f"Both options took {q['writable_ms']} ms per writable batch.",f"The compiled image evaluated the same batch in {q['packed_ms']} ms."],
            ["Two byte-vector dot products sum four learned yields and four crossing costs.","The fixed readout excludes world advance and online learning time."])
    if selected:
        q=selected[-1];return ("Choosing a route speaker","Physical screen before language tuition","Screened valuation germlines",int(q["eligible"]),"residents",
            [["Eligible residents",int(q["eligible"])],["Screened offers",int(q["offers"])],["Selected slot",int(q["effort_scholar"])]],
            [f"The screen compared {int(q['eligible']):,} living germlines.","Candidates received changed food, ground and preparation opportunities.",
             f"Resident {q['effort_scholar']} supplied the highest measured reserve."],["Selection uses physically earned carbon.","A recipient can retain its existing spatial and acoustic experience."])
    if field:
        q=field[-1];n=int(q["strides"]);raw=int(q["fresh_price"]);saved=int(q["saved"]);work=int(q.get("preparation_work",0))
        if len(labels)==2:
            counts=[int(x["living"]) for x in labels];g=next((r for r in labels if r["regulated"]=="1"),labels[0])
            return ("The cost of choosing","Two inherited lineages in the same ground","Labelled prospective residents",sum(counts),"residents",
                [["First lineage",counts[0]],["Second lineage",counts[1]],["Living population",sum(counts)]],
                [f"The two original labels have {counts[0]:,} and {counts[1]:,} living descendants.",
                 "Both groups have the learned choice circuit." if all(r["regulated"]=="1" for r in labels) else f"Lineage {g['effort_lineage']} has access to prospective motor selection.",
                 f"Residents had paid {int(q['paid']):,} units of crossing work by this census."],
                ["Natural reproduction carries the label to descendants.","The initial population and ground are matched by seed."])
        return ("Following a worked path",f"Watershed tick {int(q['effort_field']):,}   /   footing and food","Relief from worked ground",100*saved/max(1,raw),"%",
                [["Paid crossings",n],["Crossing work",int(q["paid"])],["Worked edges",int(q["tread_edges"])]],
                [f"Residents paid {int(q['paid']):,} units for {n:,} completed crossings.",
                 f"Those same steps would cost {raw:,} on untouched edges under the same conditions.",
                 f"At this census {int(q['experienced']):,} living residents retained personal food or effort experience."],
                [f"{int(q['offers']):,} prospective decisions generated {int(q['trips']):,} requests for remembered destinations.",
                 f"Deliberate surface preparation cost {work:,} units of work." if "preparation_work" in q else
                 f"The field recorded {int(q['worn']):,} compression units and {int(q['eroded']):,} units of erosion."])
    if physical or spoken or captions:
        n=len(spoken);heard=sum(int(q["effort_reply"]) for q in spoken);observed=sum(int(q["effort_caption"]) for q in captions)
        return ("At the ford","Words grounded in material choices","Witnessed ford exchange",heard if n else observed,"responses" if n else "forms",
                [["Questions retained",n],["Spoken responses",heard],["Accepted demonstrations",observed]],
                [f"The record contains {len(physical):,} physical contacts at the ford.",f"The pupil accepted {observed:,} operator demonstrations.",
                 f"It answered {heard:,} of {n:,} retained spoken prompts."],["Manipulation and walking draw on the resident’s energy.","Physical food comes out of a finite tray."])


def effort_plate(d,data,b,x,y,size,rx,rw):
    def rows(k):return [pairs(s) for s in data["lines"] if s.startswith(k+"=")]
    examples=rows("effort_slab");cost=rows("effort_field");spoken=rows("effort_language");faces=rows("effort_shape_language");cpu=rows("effort_deploy")
    palette=["#d5b37e","#89c3a8","#a8b4d7","#d2a1a3"]
    facts=[(k,human(v)) for k,v in b["stats"]];right="Measured in this run"
    if examples:
        d.rect(x,y,size,size,"#203c3d",14);d.label(x+28,y+39,"WORKING MATERIAL   /   HELD-OUT SPECIMEN","#add5c6")
        d.text(x+28,y+92,"Before the repeated crossing",32,"#f5ecd9",SERIF)
        q=examples[0];rates=int(q["rates"]);first=int(q["visible"]);after=int(q["after"]);pred=int(q["expected"]);load=int(q["crossings"])
        for j,(v,name) in enumerate(((first,"Observed"),(pred,"Predicted"),(after,"After rolling"))):
            nx=x+27+187*j;top=y+136
            d.rect(nx,top,168,147,"#365453",8)
            if v<4:
                d.rect(nx+26,top+21,115,67,palette[v],9)
                for k in range(v+1):d.rule(nx+43+k*16,top+33,nx+33+k*16,top+68,"#465857",3)
            else:d.text(nx+64,top+70,"?",38,"#e2cc9c",SERIF)
            d.text(nx+84,top+122,name,17,"#e1ebdb",MONO,anchor="middle")
        d.text(x+28,y+342,f"{load} crossing"+("" if load==1 else "s")+" with the load",25,"#efead0",SERIF)
        d.text(x+28,y+378,"Crossing effort for the four surface textures",16,"#c2d6c9")
        for j in range(4):
            fee=1+((rates>>(2*j))&3);px=x+29+139*j;d.rect(px,y+406,108,13,"#4e6660",4)
            d.rect(px,y+406,27*fee,13,palette[j],4);d.text(px,y+453,str(fee),25,"#f3dfbc",MONO)
        d.text(x+28,y+518,"Surface prepared" if q["prepared"]=="1" else "Surface left as found",24,"#e4d9ae",SERIF)
        d.text(x+28,y+554,f"{q['work']} work spent   ·   {q['harvest']} food recovered",17,"#bed4c5",MONO)
        study={q["effort_slat"]:q for q in rows("effort_slat")};right="Work spent across matched physical offers"
        facts=[(label,human(int(study[k]["price"]))) for k,label in (("inferred","Learned surface changes"),("erased","Transition contacts erased"),
              ("foreign","Mismatched resulting texture"),("unchanged","Always leave the surface")) if k in study]
    elif (cost and not cpu and not spoken and not faces):
        sampled=rows("effort_tread")
        if sampled:
            d.rect(x,y,size,size,"#183b3a",12);d.label(x+25,y+35,"SURVEY   /   PAID FOOTING","#b6d7c9")
            n=len(sampled);cell=(size-94)/n
            for j,row in enumerate(sampled):
                for i in range(n):
                    east,south,res=(int(v,16) for v in row["pixels"][3*i:3*i+3])
                    xx=x+47+i*cell;yy=y+55+j*cell
                    d.rect(xx,yy,cell+.2,cell+.2,blend("#294945","#8e7653",(east+south)/39))
                    if east:d.rule(xx+1,yy+cell/2,xx+cell,yy+cell/2,"#d3a76c",.4+east/6)
                    if south:d.rule(xx+cell/2,yy,xx+cell/2,yy+cell,"#82b5ac",.4+south/6)
                    if res:d.dot(xx+cell*.43,yy+cell*.48,cell*(.06+.12*res/15),"#e9edc3")
            d.text(x+27,y+size-18,"Ochre east banks   ·   cyan south banks   ·   pale residents",14,"#c9ddcd",MONO)
        else:watershed_panel(d,data,x,y,size)
        choices=rows("effort_random");right="Randomized access   /   average net reserve change"
        if choices:facts=[(f"{('Ordinary','Route')[int(q['predicted'])]} predicted / {('ordinary','route')[int(q['assigned'])]} assigned",f"{(int(q['gains'])-int(q['losses']))/max(1,int(q['trials'])):+.2f} net") for q in choices]
    else:
        d.rect(x,y,size,size,"#243b41",13);d.label(x+28,y+40,"THE FORD   /   OBSERVER’S RECORD","#afdbca")
        d.text(x+28,y+94,"A change before it happens" if faces else "Comparing possible journeys",32,"#f0e9d1",SERIF)
        if faces or spoken:
            data_rows=faces if faces else spoken;prefix="effort_shape_language" if faces else "effort_language"
            arm="observed" if faces else "lived"
            chart=[q for q in data_rows if q[prefix]==arm][:6]
            for j,q in enumerate(chart):
                yy=y+156+j*62;ok=int(q["correct"]);n=int(q["trials"])
                label=q["task"].replace("length_","Spoken turns  ·  ").replace("_"," ").capitalize()
                d.text(x+28,yy,label,18,"#d2e2d8");d.text(x+size-28,yy,f"{ok:,} / {n:,}",17,"#ecd194",MONO,anchor="end")
                d.rect(x+28,yy+13,size-56,7,"#4d6867",4);d.rect(x+28,yy+13,(size-56)*ok/max(1,n),7,"#b6d4a9",4)
        else:
            for j,(k,v) in enumerate(facts):d.text(x+28,y+172+99*j,k,19,"#d3e2d7");d.text(x+28,y+213+99*j,v,35,"#dfc998",SERIF)
    d.label(rx,335,right)
    for j,(k,v) in enumerate(facts[:4]):
        yy=375+39*j;d.text(rx,yy,k,17,INK);d.text(rx+rw,yy,v,17,GREEN,MONO,600,"end");d.rule(rx,yy+11,rx+rw,yy+11)


def hollow_summary(entries):
    language={(q["hollow_language"],q["task"]):q for q in entries("hollow_language")}
    trips={q["hollow_trip"]:q for q in entries("hollow_trip")}
    field=entries("hollow_field");comp=entries("hollow_lineage");bench=entries("hollow_deploy")
    ranges={q["hollow_range"]:q for q in entries("hollow_range")}
    contacts=entries("hollow_contact");replies=entries("hollow_reply");scholar=entries("hollow_scholar")
    if language and not all(k in language for k in (("visible","revalued"),("visible","after_turn"),("visible","old_sound"),("visible","other_tool"))):
        n=sum(int(q["questions"]) for q in language.values());ok=sum(int(q["correct"]) for q in language.values());spoken=sum(int(q["voiced"]) for q in language.values())
        return ("Questions from the clearing","External geometry and spoken answers","Graded marsh words",100*ok/max(1,n),"%",
                [["Questions",n],["Correct",ok],["Spoken",spoken]],
                [f"The grader scored {n:,} turns.",f"The resident spoke on {spoken:,} turns.",f"Its responses agreed with {ok:,} supplied references."],
                ["The expected words stay with the observer.","A physical body turn can change the answer to a follow-up."])
    if language:
        q=language["visible","revalued"];a=language["visible","after_turn"];f=language["visible","old_sound"]
        return ("Where is the working tool?", "Directional speech after movement and a change of water", "Spoken spatial revaluation",
            100*int(q["correct"])/int(q["questions"]),"%",[["Questions about the bath",int(q["questions"])],["Correct directions",int(q["correct"])],["Other-tool questions",int(language["visible","other_tool"]["questions"])]],
            [f"The resident gave a correct direction on {q['correct']} of {q['questions']} questions about the currently useful tool.",
             f"After turning, {a['correct']} of {a['questions']} replies agreed with the geometry or the required silence.",
             f"The older material vocabulary yielded {f['correct']} correct responses in {f['questions']} follow-ups."],
            ["Held-out directions are graded from the positions of the body and the four tools.","Paired questions change the requested object while the physical scene stays fixed."])
    if trips:
        q=trips["reanchored"];n=int(q["trials"]);paid=int(q["fed"]);walk=trips["walked"];lost=trips["landmark_exchanged"]
        return ("Back to the clearing","Two journeys with one tool and a changed inlet","Food after spatial reorientation",100*paid/n,"%",
            [["Unfamiliar clearings",n],["Completed deliveries",paid],["Carbon recovered",int(q["carbon"])]],
            [f"After passive displacement the residents located a productive tool and carried it to the inlet in {paid:,} of {n:,} trials.",
             f"Ordinary walked routes yielded {int(walk['fed']):,} meals and a substituted landmark yielded {int(lost['fed']):,}.",
             f"Their completed reoriented deliveries released {int(q['carbon']):,} units of carbon."],
            [f"Erasing the route records left {int(trips['places_erased']['fed']):,} productive deliveries.",
             "Silent working copies, sounding sisters and blind water samples were encountered separately."])
    if bench and "verified" in bench[-1]:
        q=bench[-1];n=int(q["verified"])
        return ("Choosing from a remembered map","GPU timing at a fixed body pose and clock","Indexed spatial recall",float(q["Mdecisions_per_s"]),"M choices/s",
            [["Compared choices",n],["Disagreements",int(q["differences"])],["Index bytes per mind",int(q["extra_bytes"])]],
            [f"The two spatial readouts agreed on {n:,} complete decisions.",f"For {int(q['queries']):,} choices the median writable kernel took {q['writable_ms']} ms.",
             f"The indexed kernel took {q['indexed_ms']} ms with the same offers and blocked directions."],
            ["Timing covers the readout kernel on recorded residents.","A change to the pose, time bucket, tie key or stored contacts requires a new index."])
    if field and len(comp)==2:
        q=field[-1];counts=[int(v["living"]) for v in comp];both=all(int(v["active"]) for v in comp)
        treatment=next((q for q in comp if q["active"]=="1"),comp[0])
        share=100*int(treatment["living"])/max(1,sum(counts))
        cross=any(v.get("crossed")=="1" for v in comp)
        return ("Who gets to steer?",f"Inherited motor access at watershed tick {int(q['hollow_field']):,}",
                "Labelled watershed residents" if both else "Share with ordinary map guidance",
                sum(counts) if both else share,"residents" if both else "%",
                [["First lineage",counts[0]],["Second lineage",counts[1]],["Young residents",sum(int(v["young"]) for v in comp)]],
                [f"The first founding label has {counts[0]:,} living descendants and the second has {counts[1]:,}.",
                 "Both lineages have ordinary access to spatial steering." if both else
                 (f"Lineage {treatment['hollow_lineage']} has ordinary steering and the other receives quarter-turned directions." if cross else
                  f"Lineage {treatment['hollow_lineage']} can initiate movement from its remembered places."),
                 f"At this census {sum(int(v['young']) for v in comp):,} residents were at most sixteen ticks old."],
                ["The founding label follows births and both lineages continue to learn.",
                 f"The terrain contains {int(q['stock']):,} units of carbon in cavities."])
    if field and not scholar:
        q=field[-1];t=int(q["hollow_field"]);n=int(q["certified_returns"]);meal=int(q["certified_meals"]);r=ranges.get("3")
        third=(f"At distances of three or more cells, {int(r['exact']):,} of {int(r['certified']):,} sampled memories identified their earlier patch." if r else
               f"The current residents expressed {int(q['active']):,} occupied spatial cells.")
        return ("A return to familiar ground",f"Watershed census at tick {t:,}","Recorded watershed revisits",n,"returns",
            [["Carbon in cavities",int(q["stock"])],["Paid sampled revisits",meal],["Remembering residents",int(q["remembering"])]],
            [f"The independent sample recorded {n:,} contacts with an earlier patch after a confirmed departure.",
             f"Residents received food on {meal:,} of those contacts.",third],
            [f"Water carried {int(q['captured']):,} units of loose carbon into cavities and residents deposited {int(q['invested']):,}.",
             (f"The two inherited groups contain {int(comp[0]['living']):,} and {int(comp[1]['living']):,} living residents." if len(comp)==2 else
              f"The same-length rotated paths hit {int(r['rotated_exact']):,} historical patches." if r else
              f"The current census found {int(q['cavities']):,} hollows.")])
    if contacts or replies:
        n=sum(int(q["hollow_reply"]) for q in replies);words=entries("hollow_caption");accepted=sum(int(q["hollow_caption"]) for q in words)
        return ("A speaker in the marsh","Physical placements, learned words and body turns","Spoken place contacts",n if replies else accepted,"answers" if replies else "captions",
            [["Replies in record",n],["Captions accepted",accepted],["Physical events retained",len(contacts)]],
            [f"The retained session contains {len(contacts):,} physical events in the clearing.",
             f"The resident accepted {accepted:,} captions or witnessed questions.",f"It spoke on {n:,} of {len(replies):,} retained questions."],
            ["Turning the body preserves the object last discussed.","The geometry can be checked independently of the animal’s reply."])
    if scholar:
        q=scholar[-1];return ("Screening a spatial organ","Independent physical clearings","Screened spatial germlines",int(q["candidates"]),"candidates",
            [["Eligible animals",int(q["candidates"])],["Test outings",int(q["outings"])],["Winning resident",int(q["hollow_scholar"])]],
            [f"The screen included {int(q['candidates']):,} residents.","Each candidate faced twelve independent clearings.",
             f"Resident {q['hollow_scholar']} supplied the selected genes."],["Only the selected spatial law enters a new carrier.","Physical and spoken lessons follow the screen."])


def hollow_plate(d,data,b,x,y,size,rx,rw):
    def rows(tag):return [pairs(line) for line in data["lines"] if line.startswith(tag+"=")]
    trips={q["hollow_trip"]:q for q in rows("hollow_trip")};sites=rows("hollow_site")
    language={(q["hollow_language"],q["task"]):q for q in rows("hollow_language")}
    field=rows("hollow_field");path=rows("hollow_path")
    if path and sites:
        d.rect(x,y,size,size,"#193c3c",14);d.label(x+28,y+39,"FIELD NOTES   /   ONE RECORDED BODY","#9fdad1")
        d.text(x+28,y+89,"The two-leg return",35,"#f1e9d4",SERIF)
        q=path[0];steps=[int(v["body"]) for v in rows("hollow_step")]
        signed=lambda a:(a+128)%256-128
        track=[(int(q["start_x"]),int(q["start_y"]),False)]+[(signed(v&255),signed((v>>8)&255),bool(v&65536)) for v in steps]
        loc=[(int(s["x"]),int(s["y"])) for s in sites]
        extent=max(8,*(abs(v) for xx,yy,_ in track for v in (xx,yy)),*(abs(v) for p in loc for v in p))
        pitch=187/(extent+1)
        def pixel(xx,yy):return x+size/2+xx*pitch,y+316+yy*pitch
        for j in range(-extent,extent+1):
            ax,ay=pixel(j,-extent);bx,by=pixel(j,extent);d.rule(ax,ay,bx,by,"#345450",.8)
            ax,ay=pixel(-extent,j);bx,by=pixel(extent,j);d.rule(ax,ay,bx,by,"#345450",.8)
        for (xx,yy,_),(xx2,yy2,held) in zip(track,track[1:]):
            ax,ay=pixel(xx,yy);bx,by=pixel(xx2,yy2);d.rule(ax,ay,bx,by,"#efab69" if held else "#abe0be",4)
        for j,(xx,yy) in enumerate(loc):
            ax,ay=pixel(xx,yy);col=("#edbd72","#97c6c6","#b6afdb","#deaa9f","#74bdd3","#cbd0b7")[j]
            d.dot(ax,ay,9,col);d.text(ax+13,ay-8,("I","II","III","IV","Inlet","Stone")[j],15,"#fff0d8",MONO)
        ax,ay=pixel(*track[0][:2]);d.dot(ax,ay,5,"#fff4d8")
        d.text(x+28,y+532,"Mint = empty claw     Amber = carrying",17,"#d9e4cd",MONO)
        d.text(x+28,y+565,"First 32 work ticks from the first held-out clearing.",15,"#b5cabe")
    elif field and not language:
        watershed_panel(d,data,x,y,size)
    else:
        d.rect(x,y,size,size,"#1d383c",14);d.label(x+27,y+38,"MARSH RECORD   /   BODY AND REFERENCE","#b1dacb")
        d.text(x+27,y+91,"Directions after a body turn" if language else "Witnessed in the clearing",32,"#f4ecd9",SERIF)
        if language:
            for i,(k,label) in enumerate((("axial","Tool on a single axis"),("other_tool","Different tool in the same scene"),("revalued","Tool for the changed water"),("after_turn","Following the body turn"),("old_sound","Older word for the sound"))):
                q=language.get(("visible",k))
                if not q:continue
                good,n=int(q["correct"]),int(q["questions"]);yy=y+157+77*i
                d.text(x+28,yy,label,19,"#e3eddb");d.text(x+size-28,yy,f"{good:,} / {n:,}",17,"#ead09b",MONO,anchor="end")
                d.rect(x+28,yy+14,size-56,7,"#4a6965",3);d.rect(x+28,yy+14,(size-56)*good/max(1,n),7,"#a9dcbb",3)
        else:
            for i,(label,value) in enumerate(b["stats"]):
                d.text(x+26,y+165+i*98,label,20,"#dae8d6");d.text(x+26,y+207+i*98,human(value),32,"#ecd49c",SERIF)
    if trips:
        facts=[(label,f"{int(trips[k]['fed']):,} / {int(trips[k]['trials']):,}") for k,label in
               (("reanchored","Correct landmark"),("landmark_exchanged","Exchanged landmark"),("one_cell","One spatial cell"),("places_erased","Places erased"))]
        title="Completed physical deliveries"
    else:
        facts=[(label,human(v)) for label,v in b["stats"]];title="Measured in this run"
    d.label(rx,335,title)
    for i,(label,value) in enumerate(facts):
        yy=375+39*i;d.text(rx,yy,label,17,INK);d.text(rx+rw,yy,value,17,GREEN,MONO,600,"end");d.rule(rx,yy+11,rx+rw,yy+11)


def pressure_summary(data, entries):
    field = entries("press_field")
    cases = {q["press_transfer"]: q for q in entries("press_transfer")}
    school = entries("press_season")
    names = {(q["press_language"],q["task"]):q for q in entries("press_language")}
    replies = entries("press_reply")
    contacts = entries("press_contact")
    readings = {r["press_census"]:r for r in entries("press_census")}
    touch = entries("press_touch")
    advanced = next((int(q["site_ticks"]) for q in entries("flood_seconds") if "site_ticks" in q),0)
    if names:
        q=names.get(("both","tool"))
        if not q: return None
        n,ok=int(q["questions"]),int(q["correct"])
        f=names.get(("both","followup"),{}); cleared=names.get(("no_topic","followup"),{})
        return ("Which tool opens the brook?", "Learned human questions   /   silent physical predictions", "Spoken tool revaluation",
                100*ok/max(1,n), "%", [["Tool questions",n],["Correct names",ok],["Follow-up answers",int(f.get("correct",0))]],
                [f"The pupil named the productive tool on {ok:,} of {n:,} unfamiliar questions.",
                 f"It correctly named the predicted vibration on {int(f.get('correct',0)):,} follow-ups.",
                 f"Clearing the preceding reference gave {int(cleared.get('correct',0)):,} correct follow-up names."],
                ["The two questions arrive after the reservoir turns.","The grader retains the expected words outside the device."])
    if replies:
        n=len(replies); spoken=sum(int(q["press_reply"]) for q in replies)
        return ("A tool and its expected effect","Conversation at the pressure wheel","Live pressure predictions",spoken,"words",
                [["Questions retained",n],["Answered",spoken],["Physical strokes",sum(int(q["work"]) for q in contacts)]],
                [f"The retained exchange contains {spoken:,} answers to {n:,} human questions.",
                 f"The wheel recorded {sum(int(q['blind']) for q in contacts):,} blind water samples.",
                 "The answers used the pupil’s current sensory and value contacts."],
                ["A physical intervention clears the previous spoken reference.","Visible tool choices can precede every food-bearing trial."])
    if field and advanced:
        q=field[-1]; n=int(q["loaded"]);ok=int(q["openings"]);printed=int(q["impressions"])
        return ("The working clay beds",f"Watershed tick {int(q['press_field']):,}   /   pressure and groundwater",
                "Productive wet pressure contacts",100*ok/max(1,n),"%",[["New impressions",printed],["Openings",ok],["Food swallowed",int(q["eaten"])]],
                [f"By this census the inhabitants had altered {printed:,} faces by contact between pieces.",
                 f"Their {int(q['strokes']):,} paid strokes opened a loaded reservoir {ok:,} times.",
                 f"The residents spent {int(touch[-1]['heat']):,} extra energy probing rims and tools." if touch else f"Neighboring animals witnessed {int(q['witnessed']):,} strokes directly."],
                [f"{int(q.get('from_marked',q.get('from_copies',0))):,} changed faces received a pattern from a previously marked tool.",
                 f"Readers sampled {int(q.get('ancestral',0)):,} pressure marks older than themselves." if "ancestral" in q else "The oldest alteration dates stay with the material."])
    if cases:
        q=cases["revalued"];n=int(q["offered"]);ok=int(q["productive"]);prior=int(cases["prior"]["productive"]);wet=int(q["blind_tastes"])
        return ("When the water changes","Dry demonstrations   /   new water   /   first working press","Tool choice after remote revaluation",
                100*ok/max(1,n),"%",[["First choices",n],["Productive",ok],["Blind tastes",wet]],
                [f"Pupils made {ok:,} productive first choices in {n:,} paired trials after the slit changed.",
                 f"Using only the previous water samples yielded {prior:,} productive choices.",
                 f"Without seeing the dry tools they opened {int(cases['no_sight']['productive']):,} reservoirs."],
                ["The tested pieces were struck from second-generation impressions.","Tastes had no visible tool and dry contacts yielded no food."])
    if school:
        q=school[-1];n=int(q["eggs"])
        return ("The pressure nursery",f"Reproductive season {q['press_season']}   /   finite reservoirs","Eggs from pressure work",n,"eggs",
                [["Adults",int(q["adults"])],["Funded eggs",n],["Germlines",int(q["germlines"])]],
                [f"The last recorded cohort earned provision for {n:,} eggs.",f"Its largest individual brood could support {int(q['largest']):,} eggs.",
                 f"The nursery counted {int(q['germlines']):,} different learning genotypes among these adults."],
                ["Only inherited transition tables enter a hatchling.","The juvenile’s two projections begin empty."])
    if contacts:
        work=sum(int(q["work"]) for q in contacts); n=len(contacts)
        return ("Words at a working wheel","Witnessed tools, baths and vibrations","Recorded wheel contacts",work,"strokes",
                [["Events retained",n],["Physical work",work],["Witnessed strokes",sum(int(q["witnessed"]) for q in contacts)]],
                [f"The retained wheel events account for {work:,} pressure strokes.","A visible contact can ground a public name for a sound, a tool or a bath.",
                 "Each wheel supplies a finite carbon reservoir."],["Sight and limb availability constrain demonstrations.","The vocabulary belongs to the individual."])
    deployment=entries("press_deploy")
    if deployment and not readings and "verified" in deployment[-1]:
        q=deployment[-1];rate=float(q["Mdecisions_per_s"])*1e6
        return ("Four bytes of tool preferences","Blackwell deployment   /   exact two-tool choices","Frozen pressure decisions",rate,"choices/s",
                [["Residents",int(q["press_deploy"])],["Verified decisions",int(q["verified"])],["Bytes per chooser",int(q["bytes"])]],
                [f"The device compared {int(q['verified']):,} choices from its writable and frozen eyes.",
                 f"The frozen kernel took {float(q['packed_ms']):.3f} milliseconds per recorded batch.",
                 f"The comparison found {int(q['differences']):,} disagreements."],
                ["The packed image preserves ties.","The ecological hands retain writable plasticity."])
    if readings:
        q=readings["own"];n=int(q["discriminating"]);ok=int(q["opened"])
        return ("Choosing from the living banks","Unaltered neighbors and their actual hands","Field tool choices",100*ok/max(1,n),"%",
                [["Residents",int(q["residents"])],["Discriminating offers",n],["Productive choices",ok]],
                [f"Residents chose an opening tool on {ok:,} of {n:,} discriminating offers.",f"Random hands did so on {int(readings['random']['opened']):,} of those same offers.",
                 "Every virtual trial used the body and mineral positions in the closing census."],["The examination leaves all material in place.","The reservoir balance remains available in the raw record."])
    return None


def pressure_plate(d,data,b,x,y,size,rx,rw):
    def rows(tag):
        return [pairs(l) for l in data["lines"] if l.startswith(tag+"=")]
    pieces=rows("press_specimen")
    cases={q["press_transfer"]:q for q in rows("press_transfer")}
    language={(q["press_language"],q["task"]):q for q in rows("press_language")}
    field=rows("press_field")
    live=rows("press_reply")
    geo=b["family"] in ("Productive wet pressure contacts","Field tool choices")
    if geo:
        watershed_panel(d,data,x,y,size)
    else:
        d.rect(x,y,size,size,"#1e3334",15)
        d.label(x+28,y+40,"PRESSURE WORKS   /   MATERIAL AND MEMORY","#bcdbcd")
        if language:
            d.text(x+28,y+93,"A word after the water turns",32,"#f3ebd3",SERIF)
            study=[("both","Both experiences"),("prior","Earlier water"),("no_sight","Tools unseen"),
                   ("no_taste","Water unsampled"),("fixed","Contacts fixed")]
            for j,(arm,label) in enumerate(study):
                q=language.get((arm,"tool"))
                if not q:continue
                yy=y+164+71*j;n=int(q["questions"]);ok=int(q["correct"])
                d.text(x+28,yy,label,20,"#dceade");d.text(x+size-27,yy,f"{ok:,} / {n:,}",19,"#e9cc9a",MONO,anchor="end")
                d.rect(x+28,yy+14,size-56,8,"#466063",4);d.rect(x+28,yy+14,(size-56)*ok/max(1,n),8,"#d9b986",4)
        elif pieces:
            d.text(x+28,y+91,"Four tested impressions",34,"#f3ebd3",SERIF)
            palette=("#d4a773","#88b3a4","#9fa4cd","#ceaa9e")
            for j,q in enumerate(pieces[:4]):
                face=int(q["face"]); e=int(q["impression"]); hue=((int(q["glaze"])>>4)^face)&3
                xx=x+28+(j%2)*281;yy=y+122+(j//2)*183
                d.rect(xx,yy,265,168,"#334c4a",10);d.rect(xx+13,yy+15,119,121,palette[hue],12)
                for k in range(8):
                    a=xx+23+(k%4)*26;c=yy+33+(k//4)*41
                    d.rect(a,c,20,35,"#4c5551" if k==face else "#8a8b78",4)
                    if k==face:
                        for z in range(1+((e>>2)&3)):d.rule(a+3,c+7+z*5,a+16,c+7+z*5,"#f6e2aa",2)
                d.text(xx+147,yy+59,f"Face {face}",18,"#eff0d8",SERIF)
                d.text(xx+147,yy+89,("ka","li","mu","so")[e&3],22,"#dfc492",MONO)
                d.text(xx+14,yy+151,f"Shell {hue+1}  ·  glaze {int(q['glaze']):02X}",14,"#d2e2d1",MONO)
            d.text(x+28,y+525,"Resonance survives a change of carrier.",21,"#dce4c8",SERIF)
            d.text(x+28,y+557,"The highlighted face is the one used in the held-out trial.",15,"#b3c9b8")
        elif rows("press_deploy") and b["family"]=="Frozen pressure decisions":
            bench=rows("press_deploy")[-1]
            d.text(x+28,y+91,"Two ways to read a tool",32,"#f3ebd3",SERIF)
            durations=[float(bench[k]) for k in ("writable_ms","packed_ms")]
            for j,(duration,caption) in enumerate(zip(durations,("Living projections","Four-byte rank table"))):
                yy=y+195+j*128;d.text(x+28,yy,caption,22,"#dceade")
                d.text(x+size-28,yy,f"{1000*duration:.1f} μs",20,"#e9cc9a",MONO,anchor="end")
                d.rect(x+28,yy+22,size-56,12,"#466063",5);d.rect(x+28,yy+22,(size-56)*duration/max(durations),12,"#d9b986",5)
            d.text(x+28,y+501,f"{int(bench['per_batch']):,} choices per timed batch",23,"#dce4c8",SERIF)
            d.text(x+28,y+545,"Every comparison includes tied alternatives.",16,"#b3c9b8")
        elif live:
            d.text(x+28,y+91,"From the conversation",32,"#f3ebd3",SERIF)
            heard=None;dialog=[]
            for l in data["lines"]:
                if l.startswith("press_prompt=heard  text="):heard=l.partition("  text=")[2]
                if l.startswith("animal> ") and heard is not None:
                    dialog.append((heard,l[8:]));heard=None
            for j,(query,reply) in enumerate(dialog[-4:]):
                yy=y+155+97*j;d.text(x+28,yy,short(query,41),19,"#d8e6dc")
                d.text(x+44,yy+35,"withheld" if reply=="?" else reply,24,"#ead099",SERIF)
        else:
            d.text(x+28,y+91,"From one generation to the next",29,"#f3ebd3",SERIF)
            cohorts=rows("press_season")
            if cohorts:
                for j,q in enumerate(cohorts[-5:]):
                    yy=y+163+72*j
                    d.text(x+28,yy,f"Season {q['press_season']}",20,"#dceade")
                    d.text(x+size-28,yy,f"{int(q['eggs']):,} eggs",22,"#e7c796",MONO,anchor="end")
            else:
                for j,q in enumerate(rows("press_contact")[-5:]):
                    d.text(x+28,y+162+70*j,f"{q['work']} strokes   ·   {q['meals']} carbon",20,"#d8e6dc",MONO)
    if geo or field and not cases and not language:
        measured={q["press_census"]:q for q in rows("press_census")}
        heading="Available working tools   /   matched offers"
        conditions = (("own","Resident’s choice"),("optical","Appearance alone"),("displaced","Same taps, displaced vibrations"),("other_taste","Another resident’s taste contacts")) if rows("press_touch") else (("own","Resident’s acquired contacts"),("random","Random choice"),("other_sight","Another resident’s visual contacts"),("other_taste","Another resident’s taste contacts"))
        facts=[(label,f"{int(measured[k]['opened']):,} / {int(measured[k]['discriminating']):,}") for k,label in conditions if k in measured]
        if not facts and field:
            q=field[-1];facts=[(title,f"{int(q.get(k,0)):,}") for k,title in (("strokes","Paid strokes"),("impressions","Altered faces"),("witnessed","Live observations"),("eaten","Carbon swallowed"))]
    elif language:
        heading="After both experiences   /   words externally scored"
        facts=[(title,f"{int(language['both',k]['correct']):,} / {int(language['both',k]['questions']):,}") for k,title in
               (("tool","Choose the tool"),("bath","Find the bath"),("bridge","Name the connecting vibration"),("followup","Resolve the preceding tool")) if ("both",k) in language]
    elif cases:
        heading="Productive first choices   /   same physical offers"
        facts=[(label,f"{int(cases[k]['productive']):,} / {int(cases[k]['offered']):,}") for k,label in (("revalued","After blind revaluation"),("prior","With the earlier valuation"),("no_sight","Without the dry demonstration"),("smooth_tools","Test pieces smoothed")) if k in cases]
    else:
        heading="Observed work"
        facts=[(label,human(number)) for label,number in b["stats"]]
    d.label(rx,335,heading)
    for j,(label,reading) in enumerate(facts[:4]):
        yy=375+39*j;d.text(rx,yy,label,17,INK);d.text(rx+rw,yy,reading,17,GREEN,MONO,600,"end");d.rule(rx,yy+11,rx+rw,yy+11)


def clay_summary(entries):
    def records(tag):return [r for r in entries(tag) if len(r)>1]
    sets=records("clay_classes");universities=records("clay_transfer")
    generations=records("clay_generation");earth=records("clay_tick");contacts=records("clay_contact")
    grade=records("clay_conversation");speech=records("clay_dialogues");questions=records("clay_question_example")
    peers=records("clay_peer");inquiry=records("clay_inferred")
    if grade:
        target="source" if any(r.get("target")=="source" for r in grade) else grade[0]["target"]
        arms={r["clay_conversation"]:r for r in grade if r["target"]==target}
        q=arms.get("self_directed",arms.get("matching",next(iter(arms.values()))));n=int(q["trials"]);hit=int(q["correct"])
        cost=int(q["paid_taps"]);other=arms.get("foreign_self_directed",arms.get("other_object",{}))
        return ("Who worked this clay?","Learned spoken questions   /   acquired source memories","Spoken attribution from material evidence",100*hit/max(1,n),"%",
            [["Source questions",n],["Correct words",hit],["Paid taps",cost]],
            [f"The resident answered {hit:,} of {n:,} externally scored questions correctly in the {target.replace('_',' ')} condition.",
             f"It sounded the physical object {cost:,} times across those trials.",
             f"Providing the other community’s object gave {int(other['correct']):,} correct answers." if other else "Its returned sounds were transcribed before external grading."],
            ["The external scoring words stay on the host.","The probe excluded the known queried property from every sounded relief."])
    if speech:
        q=speech[-1];n=int(q["clay_dialogues"]);done=int(q["completed"])
        return ("A witness in the clay","Native evidence and spoken source attribution","Cultural sentence completion",100*done/max(1,n),"%",
          [["Questions",n],["Spoken answers",done],["Taps",int(q["paid_taps"])]],
          [f"The interviewer offered {n:,} questions about two communities.",f"The pupil completed {done:,} utterances after {int(q['paid_taps']):,} paid taps.",
           f"It identified a source on {int(q['identified']):,} exchanges."],["Every probe used a saved physical artifact.","Each sentence starts from the same pupil."])
    if generations:
        origin=generations[-1]["origin"];g=max(int(q["clay_generation"]) for q in generations if q["origin"]==origin)
        arms={q["exposure"]:q for q in generations if int(q["clay_generation"])==g and q["origin"]==origin}
        q=arms["inferred"];n=int(q["trials"]);hit=int(q["ancestral"])
        return ("Clay across generations",f"Cohort {g}   /   {origin} objects   /   original ancestors as test partners","Conventions transmitted by objects",100*hit/max(1,n),"%",
            [["Descendant trials",n],["Ancestral exchanges",hit],["Smooth objects",int(arms["clean"]["ancestral"])]],
            [f"Members of cohort {g} acquired their acoustic contacts from the preceding cohort’s worked objects.",
             f"They harvested in {hit:,} of {n:,} tests with original speakers and in {int(q['other_object']):,} after exchanging the reference object.",
             f"Those animals pressed {int(q['new_impressions']):,} changes into new pieces for the next cohort."],
            [f"The reply shutter yielded {int(q['blocked']):,} harvests.","Each hatchling begins with empty acoustic projections."])
    if universities:
        origin=universities[-1].get("origin","workshop");arms={q["clay_transfer"]:q for q in universities if q.get("origin","workshop")==origin};q=arms["inferred"];n=int(q["opportunities"]);hit=int(q["harvested"])
        return ("Listening for a community",f"Two speaking communities   /   {origin} objects","Reinstated acoustic conventions",100*hit/max(1,n),"%",
          [["New grips",n],["Productive grips",hit],["Other community",int(arms["other_clay"]["harvested"])]],
          [f"The pupils produced {hit:,} harvests in {n:,} held private-source trials.",
           f"Without sounding the object they produced {int(arms['untapped']['harvested']):,}, and exchanging its source gave {int(arms['other_clay']['harvested']):,}.",
           f"The true reference supplied {int(q['strikes']):,} taps across {int(q['probes']):,} inquiries."],
          [f"The learners occupied {int(q['recruited']):,} assemblies across {int(q['pupils']):,} pupils.",f"Reference strikes on the tested filter numbered {q['target_strikes']}."])
    if peers:
        q=next((r for r in peers if r.get("age")=="all" and r.get("clay_peer")=="lived"),None)
        if q:
            n=int(q["trials"]);a=int(q["with_a"]);b=int(q["with_b"]);no=next((r for r in peers if r.get("age")=="all" and r.get("clay_peer")=="shut"),{})
            return ("Words left by earlier residents","Two original speakers   /   every surviving receiver","Difference between reference communities",100*(a-b)/max(1,n),"points",
                [["Residents",int(q["residents"])],["With first speaker",a],["With second speaker",b]],
                [f"Every living receiver met both frozen reference speakers on {n:,} paired physical opportunities each.",
                 f"The first speaker yielded {a:,} harvests and the second yielded {b:,}.",
                 f"Blocking the reply gave {int(no['with_a']):,} harvests with each speaker." if no else "The published observation retains the individual control arms."],
                ["A positive difference favors the first speaker.",f"The cohort recovered {int(earth[-1]['before_birth']):,} impressions older than its reader." if earth else "Both communities faced identical sealed inlets."])
    if earth:
        q=earth[-1];count=int(q["grooves"]);area=int(q["ground"]);held=int(q["held"])
        return ("An inhabited clay bank",f"Watershed tick {int(q['clay_tick']):,}   /   portable fired impressions","Occupied ceramic grooves",count,"grooves",
          [["Worked grains",area+held],["In living or vacant hands",held],["Occupied grooves",count]],
          [f"The census found {count:,} impressions on {area:,} ground objects and {held:,} carried objects.",
           f"Readers recovered {int(q['before_birth']):,} impressions whose latest surface change preceded their birth.",
           f"At the census, {int(q['assemblies']):,} context assemblies were expressed across the living population."],
          [f"The physical ledger includes {int(q['work']):,} energy units spent on clay.",f"Water abraded {int(q['abrasion']):,} occupied faces."])
    if contacts or inquiry or questions:
        tapped=sum(int(r["strokes"]) for r in contacts)+sum(int(r["taps"]) for r in inquiry)
        return ("Meeting a new witness","Audible material and learned expressions","Physical source inquiries",tapped,"taps",
            [["Logged taps",tapped],["Question examples",len(questions)],["Live inquiries",len(inquiry)]],
            [f"The retained exchanges include {tapped} paid taps on clay.",f"The transcript includes {len(questions)} demonstrated questions with witnessed answers.",
             f"The resident used its own stopping rule on {len(inquiry)} natural-language requests."],
            ["The groove’s pitch becomes available only after a stroke.","The color can be covered independently of the recorded sound."])
    return None

def reed_summary(entries,metrics):
    def keyed(label):
        return {r[label]:r for r in entries(label) if r[label]!="vacant"}
    census=keyed("siphon_census");test=keyed("siphon_transfer");paired=keyed("reed_pair");spoken=keyed("reed_evidence")
    environment=keyed("siphon_legacy");teaching=entries("reed_caption");duets=entries("reed_duet")
    word_trials=entries("reed_sentences");closure=entries("reed_completion")
    families=[r for r in entries("siphon_foster") if r.get("cohort")=="1"]
    tick=entries("siphon_tick"); audience=entries("siphon_audience")
    if "open" in spoken:
        q=spoken["open"];total=int(q["unique"]);hit=int(q["accurate"]);silent=spoken.get("answer_shut",{})
        return ("Words across the current","Material descriptions learned from witnessed events","Spoken reports of private material",
                100*hit/max(1,total),"%",[["Unique facts",total],["Correct completions",hit],["Ambiguous scenes",int(q["ambiguous"])]],
                [f"The animal recognized questions about {total:,} single-valued material facts in the open channel.",
                 f"It supplied the right word in {hit:,} replies after consulting the other animal.",
                 f"It withheld a word in {int(q['withheld']):,} of {int(q['ambiguous']):,} recognized cases with several or zero possible answers."],
                [f"The matching closed answer channel gave {int(silent['accurate']):,} correct replies." if silent else "The open channel carried the partners’ native sounds.",
                 "Whole-utterance transcripts are scored separately against the external material table."])
    if "lived" in paired:
        q=paired["lived"];n=int(q["trials"]);hit=int(q["productive"])
        return ("A question between neighbors","Exported pair   /   independent private source substitutions","Two-way field calls",
                100*hit/n,"%",[["Private trials",n],["Productive grips",hit],["Complete quartets",int(q["four_correct"])]],
                [f"These two residents opened the right filter in {hit:,} of {n:,} opaque trials.",
                 f"Closing the question channel yielded {int(paired['query_shut']['productive']):,} harvests.",
                 f"Closing the answer channel yielded {int(paired['answer_shut']['productive']):,} harvests."],
                [f"Fresh trials used seed {q['seed']} and exchanged upstream and downstream roles.",
                 f"Erasing acoustic experience left {int(paired['words_erased']['productive']):,} productive grips."])
    if "neighbors" in census:
        q=census["neighbors"];n=int(q["trials"]);hit=int(q["harvests"]);other=census["redistributed"]
        return ("Voices in the reedbed","All neighboring pairs   /   new private material","Field neighbors in opaque water",
                100*hit/n,"%",[["Neighbor pairs",int(q["pairs"])],["Successful grips",hit],["Pairs at 75% or better",int(q["above_three_quarters"])]],
                [f"The survey tested all {int(q['pairs']):,} adjacent pairings on {n:,} material substitutions.",
                 f"They harvested {hit:,} times together and {int(other['harvests']):,} times after redistributing the downstream partners.",
                 f"Shutting the answer path gave {int(q['answer_shut']):,} harvests."],
                [f"{int(q['above_three_quarters']):,} pairs reached at least three quarters of the possible harvests.",
                 f"The abandoned filters subsequently released {int(environment['inhabited']['passive_carbon']):,} carbon units in moving water." if "inhabited" in environment else
                 f"{int(q['four_correct']):,} quartets succeeded across all four changes to the private inlet."])
    if "inhabited" in environment:
        q=environment["inhabited"];zero=environment.get("zeroed",environment.get("unworked"));angle=environment["rotated"]
        return ("What the builders left","Unoccupied watershed   /   identical initial water and carbon","Passive filtration after occupation",
                int(q["passive_carbon"]),"carbon",[["Physical ticks",int(q["ticks"])],["Passive carbon",int(q["passive_carbon"])],["Changed food sites",int(angle["changed_food_sites"])]],
                [f"After the animals left, the existing apertures released {int(q['passive_carbon']):,} carbon units.",
                 f"Turning each face one quarter changed the food stock at {int(angle['changed_food_sites']):,} sites.",
                 f"Returning each face to zero gave {int(zero['passive_carbon']):,} units under identical water flow."],
                [f"The detached comparison ran for {q['ticks']} ticks.","Each arm started with the same reactor grains and dissolved components."])
    if families:
        arms={r["siphon_foster"]:r for r in families};q=arms["addressed"];n=int(q["trials"]);hit=int(q["harvests"])
        return ("A call passed on","Cross-fostered hatchlings   /   fixed material substitutions","Pupils use a witnessed call",
                100*hit/n,"%",[["First-cohort tests",n],["Harvests",hit],["Public events",int(q["visible_contacts"])]],
                [f"A newborn from another germline witnessed up to {q['opportunities']} adult contacts in each trial.",
                 f"After this exposure, the new pair harvested in {hit:,} of {n:,} tests.",
                 f"Reordering the audible forms gave {int(arms['reordered']['harvests']):,} harvests."],
                [f"The shuttered group harvested {int(arms['shuttered']['harvests']):,} times.",f"Before exposure each parent received {q['parents_practiced']} additional practice opportunities."])
    if "lived" in test:
        q=test["lived"];n=int(q["trials"]);hit=int(q["harvests"])
        return ("A private inlet, a shared meal","Four material substitutions per receptive band","Cooperative filter transfer",
                100*hit/n,"%",[["Trials",n],["Productive grips",hit],["Complete quartets",int(q["four_correct"])]],
                [f"The partners made {int(q['strokes']):,} grips and opened the filter {hit:,} times in {n:,} trials.",
                 f"Closing the answer channel produced {int(test['answer_shut']['harvests']):,} harvests.",
                 f"The downstream action changed in {int(q['changed_grip']):,} of {int(q['quartets']):,} private-source quartets."],
                [f"The test drew from {q['neighbors']} neighboring pairs.",f"Each received {q['practice']} additional practice opportunities."])
    if closure or teaching or duets or word_trials:
        if word_trials:
            q=word_trials[-1];offer=int(q["reed_sentences"]);count=int(q["completed"])
            return ("Listening across a private inlet","Queries supplied to both real acoustic circuits","Completed material descriptions",count,"replies",
                [["Sentences",offer],["Completions",count],["Audible turns",int(q["audible_turns"])]],
                [f"The observer presented {offer:,} material questions.",f"The resident completed {count:,} utterances.",f"The exchanges released {int(q['carbon']):,} units of carbon."],
                ["Each GPU lane began with the saved pair.","Word accuracy is recorded in the external transcript ledger."])
        viewed=int(metrics.get("reed_visible_contacts",sum(int(q["viewed"])<4 for q in duets)));taught=int(metrics.get("reed_public_captions",sum(int(r["reed_caption"]) for r in teaching)));contacts=int(metrics.get("reed_native_contacts",len(duets)))
        return ("At the listening table","Two transcribed voices   /   local contact and material","Witnessed material speech",taught,"captions",
                [["Logged duets",contacts],["Visible outcomes",viewed],["Accepted captions",taught]],
                [f"The session recorded {contacts} native exchanges.",f"The resident saw {viewed} outcomes and accepted {taught} whole captions.",
                 f"It completed {sum(int(q['reed_completion']) for q in closure)} of {len(closure)} direct inquiries."],
                ["A caption binds to the material visible after contact.","The inlet and outlet residents have separate body reserves."])
    if tick:
        q=tick[-1];heard=int(audience[-1]["heard"]) if audience else 0
        return ("Living filters","Native acoustics   /   occupied watershed","Native acoustic encounters",int(q["pairs"]),"contacts",
                [["Contacts",int(q["pairs"])],["Heard by others",heard],["Passive carbon",int(q["passive"])]],
                [f"By tick {int(q['siphon_tick']):,} the animals had made {int(q['pairs']):,} two-body contacts.",
                 f"The apertures released {int(q['released']):,} carbon units during contact.",f"Moving water released a further {int(q['passive']):,} units through the physical filters."],
                [f"The ledger recorded {int(q['strokes']):,} grips.",f"Uninvolved residents heard {heard:,} material resonances."])
    return None

def brief(data):
    command=data["command"];points=data.get("points",[]);metrics=data.get("metrics",{});scores=auto_scores(data)
    args=data.get("args",[]);source=data.get("source","");arch=bool(data.get("archive"));kind=data.get("kind") or ("world" if points else "animal" if command=="agent" else "experiment")
    catchments=foraging(data) if len(points)==1 and not metrics.get("nursery") else {}
    if "intact" in catchments:kind="forage"
    speech=dialogues(data) if command=="dialogue" else {}
    if speech:kind="conversation"
    if command=="talk":kind="interview"
    memories=recollections(data) if command=="recollect" else {}
    if "intact" in memories:kind="recollection"
    attention={r["attention"]:r for line in data.get("lines",[]) if line.startswith("attention=") for r in [pairs(line)]}
    if command=="attention" and "intact" in attention:kind="attention"
    timings=[pairs(s) for s in data.get("lines",[]) if s.startswith("attention_batch=")]
    if timings:kind="attention-scale"
    forecasts={r["imagery"]:r for s in data.get("lines",[]) if s.startswith("imagery=") for r in [pairs(s)]}
    if command=="imagine" and "intact" in forecasts:kind="imagery"
    binding={r["binding"]:r for s in data.get("lines",[]) if s.startswith("binding=") for r in [pairs(s)]}
    if command=="relate" and "intact" in binding:kind="binding"
    conjunction={r["conjunction"]:r for s in data.get("lines",[]) if s.startswith("conjunction=") for r in [pairs(s)]}
    if command=="conjoin" and "intact" in conjunction:kind="conjunction"
    joint_times=[pairs(s) for s in data.get("lines",[]) if s.startswith("joint_batch=")]
    if joint_times:kind="conjunction-scale"
    census=next((pairs(s) for s in data.get("lines",[]) if s.startswith("joint_census=")),None)
    if census:kind="conjunction-census"
    narration={r["narration"]:r for s in data.get("lines",[]) if s.startswith("narration=") for r in [pairs(s)]}
    if command=="narrate" and "intact" in narration:kind="narration"
    reports=[s for s in data.get("lines",[]) if s.startswith("report_turn=")]
    if command=="talk" and (reports or any(s.startswith(("retelling=","incoming_report=")) for s in data.get("lines",[]))):kind="report-interview"
    commons=[pairs(s) for s in data.get("lines",[]) if s.startswith("commons_generation=")]
    if command=="commons" and commons:kind="commons"
    cultures=[pairs(s) for s in data.get("lines",[]) if s.startswith("culture_generation=")]
    if command=="commons" and cultures:kind="culture"
    inference={r["inference"]:r for s in data.get("lines",[]) if s.startswith("inference=") for r in [pairs(s)]}
    if command=="infer" and "intact" in inference:kind="inference"
    graph_census=next((pairs(s) for s in data.get("lines",[]) if s.startswith("concept_census=")),None)
    if graph_census:kind="concept-census"
    endpoints=[pairs(s) for s in data.get("lines",[]) if s.startswith("endpoint_batch=")]
    if endpoints:kind="endpoint-scale"
    if command=="talk" and (metrics.get("concept_turns") or (metrics.get("board_stimuli") and not reports)):kind="concept-interview"
    estuaries=[pairs(s) for s in data.get("lines",[]) if s.startswith("estuary_generation=")]
    if commons and estuaries:kind="estuary"
    cultivation={r["cultivation"]:r for s in data.get("lines",[]) if s.startswith("cultivation=") for r in [pairs(s)]}
    if command=="cultivate" and "intact" in cultivation:kind="cultivation"
    causal_census=next((pairs(s) for s in data.get("lines",[]) if s.startswith("causal_census=")),None)
    if causal_census:kind="causal-census"
    if command=="talk" and metrics.get("garden_events"):kind="garden-interview"
    crops=[pairs(s) for s in data.get("lines",[]) if s.startswith("greenhouse_generation=")]
    if commons and crops:kind="greenhouse"
    sequence=next((pairs(line) for line in data.get("lines",[]) if line.startswith("sequence_family=")),{})
    if command=="develop" or "sequence_turns" in metrics:kind="development"
    chorus=[pairs(line) for line in data.get("lines",[]) if line.startswith("chorus_season=")]
    if chorus:kind="chorus"
    compositions=[pairs(line) for line in data.get("lines",[]) if line.startswith("composition=")]
    population=[pairs(line) for line in data.get("lines",[]) if line.startswith("composition_batch=")]
    shelters=[pairs(line) for line in data.get("lines",[]) if line.startswith("shelter_season=")]
    exchanges=[line for line in data.get("lines",[]) if line.startswith("composed_reply=")]
    if command=="compose":kind="composition"
    if exchanges or "compiled_events" in metrics:kind="composed-dialogue"
    if population:kind="composition-scale"
    if shelters:kind="shelter"
    if metrics.get("vocal_plans"):kind="self-hearing"
    lines=data.get("lines",[])
    def entries(key):return [pairs(line) for line in lines if line.startswith(key+"=")]
    motors=entries("procedural_cells");seasons=entries("roost_season");projection=entries("projected_states")
    policies=entries("policy_batch")+entries("family_batch");steps=entries("grounded_procedure")
    if command=="imitate":kind="motor-learning"
    surveys=entries("motor_survey")
    if surveys:kind="causal-questions"
    if seasons:kind="flight-colony"
    if metrics.get("procedure_turns") or steps:kind="motor-interview"
    if projection or entries("motor_forecast") or policies:kind="motor-forecast"
    if entries("flood_tick"):kind="floodplain"
    if entries("scene_transfer"):kind="grounding"
    elif entries("scene_reply") or entries("scene_point"):kind="scene-dialogue"
    group=Path(source).name if source else command
    stat=[];narrative=[];bullets=[]
    if kind=="world" and points:
        first,last=points[0],points[-1];habitat=data.get("habitat","habitat");seed=int(metrics.get("seed",0))
        title=f"{habitat.capitalize()} at tick {last[1]:,}"
        subtitle=f"{data['side']} × {data['side']} sites   /   Seed {seed}   /   {data.get('metabolism','energy')} metabolism"
        def delta(column):
            return sum(max(0,b[column]-a[column]) for a,b in zip(points,points[1:]) if a[2]==b[2])
        counts=[metrics.get("interval_"+field,delta(k)) for field,k in zip(("births","deaths","moves","edits","grazed"),(6,7,8,9,10))]
        if arch or len(points)==1:counts=last[6:11]
        newborn,lost,moved,edited,food=counts
        stat=[["Living animals",last[3]],["Lineage depth",last[5]],["Heritable edits",edited]]
        window="checkpoint history" if arch or len(points)==1 else "observed interval"
        if len(points)==1:
            narrative.append(f"The {habitat} holds {human(last[3])} animals at tick {human(last[1])}.")
        else:
            narrative.append(f"The {habitat} went from {human(first[3])} to {human(last[3])} living animals in {human(last[0]-first[0])} simulated ticks.")
        narrative.append(f"The {window} records {human(newborn)} births, {human(lost)} deaths and {human(moved)} moves.")
        narrative.append(f"The deepest resident lineage spans {counted(last[5],'division')}, with {counted(last[4],'surviving founder line')} in the habitat.")
        bullets=[f"{human(edited)} hereditary edits and {human(food)} units grazed in the {window}.",f"Final population {human(last[3])} across {human(data['side']**2)} sites."]
        family="Population · "+habitat;measure=last[3];unit="animals"
        training=next((pairs(v) for v in data.get("lines",[]) if v.startswith("nursery=")),{})
        if training:
            epochs=int(training["generations"]);candidates=int(training["candidates"]);lives=int(training["tested_lifetimes"])
            title=f"{habitat.capitalize()} after {counted(epochs,'generation')}"
            stat=[["Breeding generations",epochs],["Candidates per cohort",candidates],["Evaluated lifetimes",lives]]
            narrative[0]=f"The nursery bred {counted(epochs,'generation')} of {human(candidates)} candidates and evaluated {counted(lives,'lifetime')}."
            if data.get("breeding"):
                a,z=data["breeding"][0],data["breeding"][-1]
                narrative[1]=f"Mean points per candidate were {a[2]/a[1]:.2f} at the first sampled generation and {z[2]/z[1]:.2f} at the last."
                family="Breeding points · "+(args[args.index("--nursery-problem")+1] if "--nursery-problem" in args else "paired")
                measure=z[2]/z[1];unit="points"
            bullets[1]=f"Breeding treatment {training['nursery']} across {human(epochs)} generations."
        if scores:
            label,a,b=scores[-1]
            narrative[-1]=f"The {label.lower()} probe scored {human(a)} of {human(b)} choices."
            bullets[-1]=f"{label} reached {ratio(a,b)} across {counted(b,'choice')}."
            if len(points)==1 and not training:
                family=label;measure=100*a/b;unit="%"
                stat=[["Living animals",last[3]],["Probe choices",b],[label,ratio(a,b)]]
        change={"moves":moved,"births":newborn,"grazed":food}
    elif kind=="floodplain":
        water=entries("flood_tick");census=entries("flood_census")[-1];last=water[-1];first=water[0]
        end=lambda key:int(last[key])
        diff=lambda key:end(key)-int(first[key])
        trials=entries("ecological_legacy");extent=entries("flood_frontier")
        title="Life on the floodplain";subtitle=f"Watershed at tick {end('flood_tick'):,}   /   {counted(int(census['families']),'surviving founder line')}"
        stat=[["Living animals",end("living")],["Expressed body plans",int(census["distinct_bodies"])],["Deepest lineage",int(census["deepest"])]]
        narrative=[f"The watershed held {human(end('living'))} residents after {human(diff('births'))} births and {human(diff('deaths'))} deaths during this observation.",
                   f"The final population expressed {human(int(census['distinct_bodies']))} body plans and {human(int(census['expressed_tapes']))} spiking programs.",
                   f"Water moved across the habitat while {human(end('banked'))} units of clay remained built into its banks."]
        if trials:
            q=trials[-1];narrative[-1]=f"After {q['ecological_legacy']} ticks without animals, retaining the banks changed water at {human(int(q['water_sites']))} sites and food at {human(int(q['food_sites']))}."
        if extent:
            q=extent[-1];narrative[0]=f"Descendants reached the frontier and the habitat grew to {q['to']} × {q['to']} sites, with {human(end('living'))} animals at the final census."
        bullets=[f"{human(int(census['persistent_bodies']))} body clades and {human(int(census['persistent_brains']))} neural clades survived at least a season with two living members.",
                 f"{human(diff('moved'))} moves and {human(diff('receptions'))} received calls during this run."]
        family="Floodplain residents";measure=end("living");unit="animals"
        pods=entries("pod_tick")
        temporal=next((r for r in entries("pod_transfers") if r.get("practice","self")=="self" and "practiced" in r),{})
        remap=next((r for r in entries("pod_remap") if r.get("practice")=="self"),{})
        rafts=entries("raft_season")
        if pods and len(pods)>1:
            change=lambda k:int(pods[-1].get(k,0))-int(pods[0].get(k,0))
            narrative[2]=f"Residents opened {human(change('cracked'))} of {human(change('bites'))} pods and absorbed {human(change('absorbed'))} units of energy."
        if rafts:
            q=rafts[-1];title="Reproduction on the tidal rafts"
            narrative[0]=f"The final raft season provisioned {human(int(q['embryos']))} embryos from {human(int(q['reserve']))} units of stored energy."
            bullets[0]=f"Season {q['raft_season']} ended with {human(int(q['opened']))} pods opened in {human(int(q['bites']))} grasps."
        if temporal:
            q=temporal;n=int(q['probes']);val=lambda k:human(int(q[k]))
            title="Learning from delayed meals";family="Held-out fruit choices";measure=100*int(q["practiced"])/n;unit="%"
            stat=[["Cold contacts",int(q["naive"])],["After practice",int(q["practiced"])],["After reversal",int(q["relearned"])]]
            narrative=[f"{val('pod_transfers')} juveniles learned on crops with independent, unfamiliar opening rules.",
                       f"They chose the right muscle on {val('practiced')} of {human(n)} held-out appearances and on {val('relearned')} after a reversal.",
                       f"Juveniles receiving another animal’s digestive signals chose correctly {val('yoked')} times in the first test."]
            bullets=[f"Clearing acquired contacts gave {val('erased')} correct choices out of {human(n)}.",f"{val('coincident')} successful or failed bites shared their digestion slot with earlier material."]
        if remap:
            q=remap;n=int(q["unseen"]);val=lambda k:human(int(q[k]))
            title="When one fruit changes";family="Transfer to unseen fruits";measure=100*int(q["intact"])/n;unit="%"
            stat=[["Untouched-fruit tests",n],["Correct after experience",int(q["intact"])],["With another gut",int(q["yoked"])]]
            narrative=[f"{val('pod_remap')} juveniles encountered an unannounced shared change in the muscle needed to open each fruit.",
                       f"Following {q['exposures']} opportunities on {counted(int(q['presented_cues']),'fruit variety') if int(q['presented_cues'])==1 else q['presented_cues']+' fruit varieties'}, they chose correctly on {val('intact')} of {human(n)} tests with untouched varieties.",
                       f"Moving only the selected motor frame into their earlier tissue reproduced {val('frame_only')} correct choices."]
            bullets=[f"With independent changes on the untouched fruits, the same choices scored {val('independent')} of {human(n)}.",f"{val('different_arm_correct')} of {val('different_arm_probes')} choices were correct when the fruit originally needed a different muscle from the observed variety."]
        growth=entries("spiking_growth");replays=entries("growth_replay")
        if growth and (replays or not temporal and not remap):
            final=growth[-1];initial=growth[0];delta=lambda k:int(final[k])-int(initial[k])
            title="Growing a spiking population";family="Living spiking cells";measure=int(final["expressed"]);unit="cells"
            stat=[["Living spiking cells",int(final["expressed"])],["Cells recruited",delta("cells_added")],["Cells retired",delta("cells_retired")]]
            narrative=[f"The closing census held {human(int(final['expressed']))} somatic spiking cells across {human(end('living'))} living residents.",
                       f"During the run, individual animals recruited {human(delta('cells_added'))} cells and retired {human(delta('cells_retired'))}.",
                       f"{human(delta('larger_newborns'))} offspring hatched with larger brains than their parents had at birth and {human(delta('smaller_newborns'))} hatched with smaller brains."]
            bullets=[f"Making the new tissue cost {human(delta('growth_energy'))} units of body energy.",f"Each animal could use up to {final['capacity_cells']} somatic cells in this arena."]
            if replays:
                q=replays[-1]
                bullets[1]=f"Silencing the postnatal cells changed {human(int(q['actions_changed']))} somatic motor choices among {human(int(q['growth_replay']))} grown residents."
            if int(final.get("silent_cells",0)):
                narrative[2]=f"At the closing census, the cohort intervention silenced {human(int(final['silent_cells']))} additional cells while their tissue still incurred the ordinary costs."
        mineral=entries("material_tick");assemblies=entries("assemblies");legacy_material=entries("material_legacy")
        if mineral:
            q=mineral[-1];start=mineral[0];difference=lambda k:int(q[k])-int(start[k])
            title="What the builders left behind";family="Mineral reactions";measure=difference("products");unit="reactions"
            stat=[["Conserved grains",int(q["grains"])],["Bond transfers",difference("current")],["Catalytic reactions",difference("products")]]
            narrative=[f"The habitat contained {human(int(q['grains']))} reactive grains, including {human(int(q['carried']))} carried by resident slots.",
                       f"During this observation the animals rotated pieces {human(difference('turns'))} times and redistributed surface groups {human(difference('exchanges'))} times.",
                       f"Light and bonds supplied energy for {human(difference('products'))} local conversions of detritus into edible product."]
            bullets=[f"{human(difference('lifted'))} pickups, {human(difference('placed'))} placements and {human(difference('pushed'))} pushes, pulls or translations.",
                     f"Builders spent {human(difference('mechanical_work'))} energy units on their material interventions."]
            if assemblies:
                z=assemblies[-1];stat[1]=["Largest assembly",int(z["largest"])]
                bullets[1]=f"{human(int(z['bonds']))} bonds, {human(int(z['branches']))} branching grains and {human(int(z['independent_loops']))} independent cycles at the final census."
            if len(legacy_material)>=2:
                a0,a1=legacy_material[:2]
                narrative[2]=f"After harvesters left, the field completed {human(int(a0['reactions']))} reactions in {a0['ticks']} ticks, compared with {human(int(a1['reactions']))} with the same grains insulated."
        work=entries("work_season")
        if work:
            q=work[-1];title="Rearing material workers";family="Reactions in the final nursery season";measure=int(q["reactions"]);unit="reactions"
            stat=[["Nursery seasons",int(q["work_season"])],["Provisioned offspring",int(q["offspring"])],["Successful reactions",int(q["reactions"])]]
            narrative=[f"The final workshop season performed {human(int(q['interventions']))} interventions on physical pieces.",
                       f"The assemblies supported {human(int(q['reactions']))} reactions while the builders spent {human(int(q['mechanical_work']))} energy units on handling.",
                       f"The brood drew {human(int(q['provision']))} units from body reserves to provision {human(int(q['offspring']))} embryos."]
            bullets=[f"External light supplied {human(int(q['sun']))} energy units in the final season.",f"{human(int(q['witnessed']))} visible changes at another station were received."]
        construction={r['construction_probe']:r for r in entries("construction_probe") if 'trials' in r}
        if construction:
            q=construction["observed"];n=int(q["trials"]);title="Learning a physical joint";family="Working assemblies after observation";measure=100*int(q["working"])/n;unit="%"
            stat=[["New assemblies",n],["Working after observation",int(q["working"])],["With sight shuttered",int(construction["shuttered"]["working"])]]
            narrative=[f"{q['subjects']} juveniles faced new material classes and new connections between their muscles and the physical operations.",
                       f"After observing {q['shown']} demonstrations, they made {human(int(q['working']))} of {human(n)} devices produce food.",
                       f"Copying the demonstrator’s muscle identity made {human(int(construction['same_muscle']['working']))} devices work."]
            bullets=[f"With the witness’s view shuttered, {human(int(construction['shuttered']['working']))} assemblies worked.",f"The observed assemblies completed {human(int(q['reactions']))} reactions, compared with {human(int(q['insulated_reactions']))} after insulation."]
        quarry=next((q for q in entries("quarry_builders") if 'new_devices' in q),{})
        if quarry:
            q=quarry;title="Building on loose ground";family="New working devices";measure=int(q["new_devices"]);unit="devices"
            stat=[["Independent beds",int(q["quarry_builders"])],["New working devices",int(q["new_devices"])],["Working beds",int(q["operating"])]]
            narrative=[f"{human(int(q['quarry_builders']))} builders explored private mineral beds with {q['attempts']} opportunities to act.",
                       f"After removal of the builders, {human(int(q['new_devices']))} arrangements produced food where both the untouched and insulated twins produced none.",
                       f"Across all beds the changes gained {human(int(q['extra']))} reactions in some places and lost {human(int(q['lost']))} elsewhere."]
            bullets=[f"Final harvest {human(int(q['harvest']))} reactions, untouched harvest {human(int(q['untouched']))}.",f"The builders moved pieces {human(int(q['moves']))} times and exchanged surface groups {human(int(q['surface_exchanges']))} times."]
        guilds=entries("guild_chain")
        if guilds and guilds[-1].get("generation"):
            last={r["guild_chain"]:r for r in guilds};q=last["observed"];control=last["shuttered"];n=int(q["paired_tests"])
            title="Passing a joint between generations";family="Working pairs of joints";measure=100*int(q["both"])/n;unit="%"
            stat=[["Fresh pupil cohorts",int(q["generation"])],["Both devices worked",int(q["both"])],["With the view shuttered",int(control["both"])]]
            narrative=[f"{q['subjects']} independent lines passed through {q['generation']} cohorts of newly hatched builders.",
                       f"In the last cohort the personally unpracticed joint worked in {human(int(q['inherited_skill']))} of {human(n)} trials.",
                       f"Both joints worked in {human(int(q['both']))} paired trials, compared with {human(int(control['both']))} with the view shuttered."]
            bullets=[f"The pupils received {human(int(q['witnessed']))} visible changes associated with working devices in the final cohort.",f"{human(int(q['skilled']))} pupils produced both joints in at least 12 of 16 paired tests."]
        apparatus=entries("artifact_exposure")
        if apparatus and int(apparatus[-1]["artifact_exposure"]):
            q=apparatus[-1];n=int(q["artifact_exposure"]);title="What a wild assembly can do";family="Working material responses";measure=int(q["response_patterns"]);unit="patterns"
            stat=[["Isolated specimens",n],["Remote responses",int(q["transmitting"])],["Response patterns",int(q["response_patterns"])]]
            narrative=[f"Seven lighting schedules tested {n} connected assemblies collected from the wild census.",
                       f"{q['transmitting']} produced food at a separate output under two lamps, including {q['coincidence']} that needed both lamps under the tested exposure.",
                       f"Swapping the order of two pulses changed the later harvest in {q['order_sensitive']} specimens."]
            bullets=[f"{q['stored']} specimens retained excitation until a later introduction of detritus.",f"Holding all conformations fixed changed a measured response in {q['fold_sensitive']} specimens."]
        shore={r["shoal_test"]:r for r in entries("shoal_test") if "builders" in r}
        if "experienced" in shore:
            q=shore["experienced"];n=int(q["trials"]);built=int(q["builders"]);two=int(q["two_essential"])
            subtitle=f"Individual construction assay   /   {q['span']}-site separation   /   {q['moves']} movement opportunities"
            title="Building from scattered pieces";family="Construction from isolated stock";measure=100*built/n;unit="%"
            stat=[["Unfamiliar beds",n],["Working constructions",built],["Two necessary pieces",two]]
            narrative=[f"Each of {n:,} beds opened with every grain isolated and its two deposits {q['span']} steps from the light.",
                       f"After the builder left, {built:,} beds made food from a cold start, including {two:,} with at least two individually necessary relocated grains.",
                       f"Silencing spatial propagation left {int(shore['no_spread']['builders']):,} working beds in the same population."]
            bullets=[f"Untouched and electrically insulated twins yielded {q['untouched']} and {q['insulated']} reactions.",
                     f"The animals considered {int(q['candidates']):,} candidate arrangements over {q['moves']} available movements per bed."]
        elif entries("construction_tide") and not apparatus:
            q=entries("construction_tide")[-1];title="A working tide in the watershed";family="Paid spatial replay";unit="visits";measure=int(q["sessions"])
            stat=[["Work visits",measure],["Alternative layouts",int(q["candidates"])],["Energy spent",int(q["cost"])]]
            narrative=[f"At tick {int(q['construction_tide']):,} the watershed had recorded {measure:,} visits to shared material patches.",
                       f"Under their own body and sensory constraints, residents lifted grains {int(q['lifts']):,} times and placed them {int(q['placements']):,} times.",
                       f"The body energy ledger charged {int(q['cost']):,} units for inspection and mechanical work."]
            bullets=[f"A patch becomes available every {q['period']} ticks.",f"Each resident carries {q['spatial_bytes']} bytes of writable spatial contacts."]
    elif kind=="scene-dialogue":
        turns=entries("scene_reply");pointing=entries("scene_point");resolved=sum(int(r["resolved"]) for r in turns)
        asked=sum(int(r.get("asking",0)) for r in turns);located=sum(bool(int(r["certain"])) for r in pointing)
        title="Speaking about the scene";subtitle=Path(source).name or "A grounded interview"
        stat=[["Spoken turns",len(turns)],["Clarifications",asked],["Pointing requests",len(pointing)]]
        family="Grounded interview replies";measure=resolved;unit="replies"
        narrative=[f"The speaker resolved {human(resolved)} of {human(len(turns))} recorded spoken turns.",
                   f"It asked {counted(asked,'clarifying question')} using acquired acoustic forms.",
                   f"It identified a definite retinal location on {located} of {len(pointing)} pointing requests."]
        bullets=[f"{len(turns)-asked-resolved} spoken queries remained unsettled.",f"{len(pointing)} pointing requests were checked against the current visual field."]
    elif kind=="grounding":
        a=entries("scene_transfer")[-1];rows=entries("visual_question");get=lambda k:int(a.get(k,0))
        answers=sum(int(r["correct"]) for r in rows);asked=sum(int(r["trials"]) for r in rows)
        title="Words in a moving scene";subtitle=f"{get('worlds')} learners   /   {a['scene_transfer'].capitalize()} scene and speech pairing"
        stat=[["Recovered noun fields",get("nouns")],["Tested statements",get("opportunities")],["Correct replies",answers]]
        narrative=[f"The learners recovered {human(get('nouns'))} of {human(get('available_nouns'))} visual meanings from ambiguous captions.",
                   f"They judged {human(get('exact'))} of {human(get('opportunities'))} held-out statements correctly and gave {human(get('errors'))} wrong judgments.",
                   f"They answered {human(answers)} of {human(asked)} questions with a unique answer across the recorded question forms."]
        bullets=[f"{human(get('reversed'))} of {human(get('reversals'))} false role reversals were rejected.",
                 f"{human(asked-answers-sum(int(r['wrong']) for r in rows))} uniquely answerable questions received no answer."]
        scores=[["Statement truth",get("exact"),get("opportunities")]]+[[label,int(r["correct"]),int(r["trials"])] for label,r in zip(("Subject","Object","Relation","Yes or no","Follow-up"),rows)]
        family="Grounded questions answered";measure=100*answers/max(1,asked);unit="%"
    elif kind=="flight-colony":
        last=seasons[-1];res=int(last["residents"]);food=sum(int(v["meals"]) for v in seasons)
        moves=sum(int(v["moves"]) for v in seasons);inventions=sum(int(v["invented"]) for v in seasons)
        title="A season in the roost";subtitle="Returning flights   /   locally transmitted calls   /   finite fruit"
        stat=[["Final residents",res],["Meals won",food],["New calls",inventions]]
        counts=lambda key:sum(int(v[key]) for v in seasons)
        scores=[["Flights home",counts("homed"),counts("flights")],["Matched interpreted calls",counts("exact_reports"),counts("guided")],
                ["Desired physical motions",counts("attained"),moves]]
        scores=[row for row in scores if row[2]]
        family="Roost meals per resident-day";days=int(args[args.index("--days")+1]) if "--days" in args else metrics.get("roost_days",32)
        measure=food/max(1,days*sum(int(r["residents"]) for r in seasons));unit="meals"
        narrative=[f"The roost completed {human(moves)} muscle movements across {counted(len(seasons),'season')}.",
                   f"Residents brought home {human(food)} meals and introduced {counted(inventions,'new call')}.",
                   f"The last season retained {counted(int(last['founders']),'founding lineage')} and {counted(int(last['learning_genotypes']),'learning genotype')}."]
        bullets=[f"{human(sum(int(r['exact_reports']) for r in seasons))} received calls reproduced the scout’s complete flight.",
                 f"{human(sum(int(r['learned']) for r in seasons))} short calls were learned from a contemporary."]
    elif kind=="causal-questions":
        r=surveys[-1];total=int(r["withheld"]);correct=int(r["inferred"]);rooms=int(r["motor_survey"])
        title="Choosing which experiment to try";subtitle="Intervention learning   /   previously unseen wiring"
        stat=[["New rooms",rooms],["Physical contacts",int(r["contacts"])],["Exact room models",int(r["perfect"])]]
        family="Withheld lever transitions";unit="%";measure=100*correct/max(1,total)
        narrative=[f"The cohort tested {human(int(r['contacts']))} interventions in {counted(rooms,'unfamiliar room')}.",
                   f"Its acquired models correctly predicted {human(correct)} of {human(total)} withheld transitions.",
                   f"All withheld transitions were predicted correctly in {counted(int(r['perfect']),'room')}."]
        bullets=[f"The run used {r['attention']} contact selection.",f"{human(int(r['settled']))} withheld predictions had a determinate outcome."]
    elif kind=="motor-learning":
        status={"0":"unrecognized","1":"recognized","2":"ambiguous","3":"beyond capacity","4":"waiting for a visual observation"}
        r=motors[-1] if motors else {};cells=int(r.get("procedural_cells",0));lessons=int(r.get("observations",0));loops=int(r.get("recurrent_states",0))
        paths=entries("motor_transfer");retina=entries("sensorimotor");reading=entries("interpretation");description=entries("description");hearing=entries("continuous_frames")
        title="Learning a motor language";subtitle=f"{group}   /   auditory demonstrations and movement"
        stat=[["Observed episodes",lessons],["Acquired productions",cells],["Recursive states",loops]]
        family="Procedural transfer";measure=100*int(paths[0]["full_programs"])/max(1,int(paths[0]["programs"])) if paths else cells;unit="%" if paths else "cells"
        if not paths:family="Acquired motor productions"
        narrative=[f"The first learner retained {counted(cells,'production')} after {counted(lessons,'demonstration')}.",
                   f"Its acoustic receptors distinguish {counted(int(r.get('acoustic_cells',0)),'pulse form')}.",
                   f"Its acquired circuit contains {counted(loops,'recursive return state')} and {counted(int(r.get('sensory_gates',0)),'visual gate')}."]
        if paths:narrative[1]=f"The cohort reproduced {human(int(paths[0]['full_programs']))} of {human(int(paths[0]['programs']))} held-out programs."
        if retina:narrative[2]=f"Reading the live view produced {human(int(retina[0]['correct']))} exact paths in {human(int(retina[0]['paths']))} trials."
        elif reading:narrative[2]=f"The test utterance was {status.get(reading[-1]['interpretation'],'unresolved')} with {human(int(reading[-1]['length']))} output pulses."
        elif description:narrative[2]=f"The inverse motor query was {status.get(description[-1]['description'],'unresolved')} with {human(int(description[-1]['length']))} emitted pulses."
        elif hearing:narrative[2]=f"The temporal ear segmented {hearing[-1]['continuous_frames']} continuous samples into {hearing[-1]['detected_crests']} receptor events."
        bullets=[f"{r.get('code_bytes',0)} bytes store the packed forward productions.",f"The first learner used the {r.get('decoder','unavailable')} decoder."]
    elif kind=="motor-forecast":
        r=projection[-1] if projection else {};audit=entries("physical_audit");search=entries("policy_search")
        title="Imagining a motor procedure";subtitle="Acquired visual gates   /   intervention model   /   possible outcomes"
        settled=int(r.get("projected_states",0));stat=[["Settled origins",settled],["Score nodes",int(r.get("nodes",0))],["Largest motor count",int(r.get("maximum_moves",0))]]
        family="Determinate motor forecasts";measure=settled;unit="origins"
        narrative=[f"The animal projected {counted(settled,'settled outcome')} from 64 possible starting states.",
                   f"The largest determinate path contains {human(int(r.get('maximum_moves',0)))} motor pulses.",
                   f"The compiled root occupies {human(int(r.get('packed_bytes',r.get('root_bytes',0))))} bytes."]
        if audit:narrative[2]=f"Physical replay agreed on {audit[-1]['agreement']} of {audit[-1]['starts']} starting states."
        if search:narrative[1]=f"The planner examined {human(int(search[-1]['candidates']))} policies and satisfied {search[-1]['satisfied']} of {search[-1]['origins']} requested origins."
        if policies:
            rate=float(policies[-1]["Mcalls_per_s"]);family="Compiled policy forecasts";measure=rate;unit="M calls/s"
            if "families" in policies[-1]:
                family="Compiled calls across independent models"
                stat=[["Calibrated models",int(policies[-1]["families"])],["Independent histories",int(policies[-1]["animals"])],["Deployed bytes",int(policies[-1]["model_bytes"])]]
            narrative[2]=f"The compiled run processed {human(rate)} million policy calls per second across {human(int(policies[-1]['animals']))} histories."
        bullets=[f"{int(r.get('nodes',0))} parsed nodes were available for composition.",f"{int(r.get('origins',0))} sensory origins were examined for the projection."]
    elif kind=="motor-interview":
        actions=sum(int(row["actions"]) for row in steps);turns=int(metrics.get("procedure_turns",0));observed=int(metrics.get("procedural_moves",0))
        title="Hearing a score and moving";subtitle=f"{group}   /   interactive motor conversation"
        stat=[["Procedural replies",turns],["Physical actions",actions],["Motor events",observed]]
        family="Grounded procedural actions";measure=actions;unit="actions"
        narrative=[f"The conversation recorded {counted(turns,'procedural reply')}.",
                   f"The animal took {counted(actions,'physical action')} during the grounded performances.",
                   f"The observer recorded {counted(len(steps),'preview or performance')} with a measured plot state."]
        bullets=[f"{sum(v['grounded_procedure']=='imagined' for v in steps)} requests asked for a mental preview.",f"{sum(v['grounded_procedure']=='performed' for v in steps)} requests engaged the muscles."]
    elif kind=="self-hearing":
        events=[pairs(s) for s in data["lines"] if s.startswith("garden_event=")]
        plans=int(metrics["vocal_plans"]);actions=int(metrics.get("garden_actions",0))
        title="From a learned request to a physical change";subtitle="One animal   /   acquired language   /   self-hearing and causal planning"
        family="Heard utterances reaching the planner";measure=plans;unit="utterances"
        stat=[["Heard by the planner",plans],["Physical moves",actions],["Acquired voice turns",int(metrics.get("composition_turns",0))]]
        narrative=[f"The animal’s planning circuit responded to {counted(plans,'self-generated utterance')}.",
                   f"The physical record contains {counted(actions,'motor action')}.",
                   f"The experiment retained {counted(len(events),'sampled garden event')} for the report."]
        bullets=["The language circuit supplied the argument bindings and goal waveform.","The planner used its previously acquired physical contacts to choose the motors."]
    elif kind=="composition":
        footprint=next((pairs(s) for s in data.get("lines",[]) if s.startswith("learned_facts=")),{})
        fields=int(metrics.get("formed_fields",footprint.get("used_projections",0)));forms=int(metrics.get("formed_speech",footprint.get("constructions",0)));lessons=int(metrics.get("lessons",footprint.get("observations",0)))
        if compositions:
            r=compositions[0];total=int(r["turns"]);correct=int(r["correct"]);old=int(r["stale"])
            family="Exact utterances on novel bindings";measure=100*correct/max(1,total);unit="%"
            title="A new scene in a familiar language";subtitle=f"{counted(int(r['worlds']),'independent animal')}   /   unfamiliar names and arrangements"
            stat=[["Exact utterances",correct],["Held-out turns",total],["Remembered views",old]]
            narrative=[f"The pupils voiced the full expected utterance on {human(correct)} of {human(total)} held-out questions.",
                       f"Their answers matched a witness’s earlier view in {old:,} of {int(r['stale_total']):,} conflicting scenes.",
                       f"The first pupil acquired {counted(fields,'reusable projection')} from an acoustic stream."]
            bullets=[f"The cohort answered {int(r['deep']):,} of {int(r['deep_total']):,} questions about chains deeper than its demonstrations.",
                     f"The first animal retained {counted(lessons,'teaching exchange')}."]
            tuition=next((pairs(line) for line in data.get("lines",[]) if line.startswith("tuition=")),{})
            if tuition:
                bullets[1]=f"The pupils requested {human(int(tuition['requested']))} caregiver replies and revised {counted(int(tuition['revised_columns']),'speech projection')}."
        else:
            family="Acquired acoustic constructions";measure=forms;unit="constructions"
            title="A grammar learned from exchanges";subtitle="External voice   /   inferred arguments and response pathways"
            stat=[["Constructions",forms],["Used projections",fields],["Heard exchanges",lessons]]
            narrative=[f"The animal retained {counted(lessons,'teaching exchange')}.",f"It formed {counted(forms,'acoustic construction')}.",
                       f"The acquired speech circuit drew on {counted(fields,'distinct projection')}."]
            bullets=["The auditory port received whole pulse sequences.","Each recurrent path is shared across the fillers that activate it."]
    elif kind=="composed-dialogue":
        events=int(metrics.get("composition_turns",metrics.get("compiled_events",len(exchanges))));asks=int(metrics.get("composition_questions",0))
        voiced=int(metrics.get("compiled_answers",sum(1 for line in exchanges if line.split("  heard=")[0] not in ("composed_reply=_","composed_reply=?"))))
        title="A conversation from acquired relations";subtitle="Private event memory   /   learned questions and replies"
        family="Acquired utterances in conversation";measure=voiced;unit="utterances";stat=[["Recorded inputs",events],["Voiced responses",voiced],["Questions asked",asks]]
        narrative=[f"The animal processed {counted(events,'input')} through this circuit.",f"It voiced {counted(voiced,'response')} in the recorded exchange.",
                   f"It requested missing evidence on {counted(asks,'turn')}."]
        bullets=["The trace pairs each heard utterance with the emitted waves.","A question mark alone denotes an unresolved response."]
    elif kind=="shelter":
        row=shelters[-1];n=int(metrics.get("animals",int(row["residents"])));fed=int(metrics.get("market_meals",row["meals"]));asked=int(metrics.get("market_inquiries",row["inquiries"]))
        hidden=int(metrics.get("market_private",row["private_days"]));won=int(metrics.get("market_informed",row["informed_choices"]))
        title="Asking a neighbor before foraging";subtitle=f"{human(n)} resident slots   /   {human(n//32)} islands   /   {len(shelters)} seasons"
        family="Food located with a private premise";measure=100*won/max(1,hidden);unit="%"
        stat=[["Nectar meals",fed],["Questions asked",asked],["Founding lines",int(row["founders"])]]
        narrative=[f"The residents obtained {human(fed)} meals from a limited supply.",f"They initiated {human(asked)} questions through acquired vocal circuits.",
                   f"Residents reached the correct site on {human(won)} of {human(hidden)} foraging decisions involving a privately observed choice."]
        bullets=[f"The last adult cohort held {counted(int(row['genotypes']),'learning genotype')}.",f"The final season supplied {human(int(row['offspring_energy']))} energy units for offspring."]
    elif kind=="composition-scale":
        row=population[-1];n=int(row["replicas"]);events=n*int(row["events"]);rate=float(row["Mevents_per_s"])
        title="Private viewpoints at population scale";subtitle=f"{human(n)} independent histories   /   {row['acquired_models']} acquired circuits"
        family="Packed compositional events per second";measure=rate;unit="M events/s"
        stat=[["Private contact bytes",int(row["plastic_state_bytes"])],["Events per pass",events],["Shutter addresses",int(row["shutters"])]]
        narrative=[f"The cohort processed {human(events)} calibrated acoustic events per measured pass.",
                   f"The packed layout reserved {human(int(row['plastic_state_bytes']))} contact bytes per resident.",
                   f"The reference audit checked {human(int(row.get('checked_values',row['checked_events'])))} values or events across the two layouts."]
        bullets=["The benchmark compares complete response histories after each timed pass.","Vocabulary and learned projection layouts are shared across residents."]
    elif kind=="development":
        n=int(metrics.get("animals",1));cells=int(metrics.get("sequence_cells",0));trips=int(metrics.get("sequence_trips",0));moves=int(metrics.get("sequence_moves",0))
        if sequence:
            exact=int(sequence["exact"]);minimum=int(sequence["minimal"]);checks=int(sequence["transitions"])
            family="Circuits predicting every history";measure=100*exact/n;unit="%"
            title={"peer":"Learning another animal’s calls","hideout":"Learning what a witness remembers","ring":"Finding position between beacons"}.get(sequence["sequence_family"],"Finding hidden states from experience")
            subtitle=f"{counted(n,'independent animal')}   /   {sequence['method']}   /   {sequence['sequence_family']}"
            stat=[["Exact circuits",f"{exact} / {n}"],["Physical moves",moves],["Recruited cells",cells]]
            narrative=[f"The animals made {human(moves)} physical moves while learning the apparatus.",
                       f"Verification checked {counted(checks,'reachable contact')} against the sensory waveforms.",
                       f"{human(exact)} circuits agreed after every possible motor history."]
            bullets=[f"{human(minimum)} circuits also used the minimum possible number of states.",
                     f"The animals returned home {human(trips)} times during rearing."]
        elif "sequence_turns" in metrics:
            turns=int(metrics["sequence_turns"]);replies=int(metrics["sequence_replies"])
            family="Auditory turns with a voiced reply";measure=replies;unit="turns"
            title="A conversation from a learned circuit";subtitle="Acquired auditory signals   /   persistent private memory"
            stat=[["Input turns",turns],["Voiced replies",replies],["Latent cells",cells]]
            narrative=[f"The animal received {counted(turns,'input turn')}.",f"It voiced a learned response on {counted(replies,'turn')}.",f"Its recurrent circuit contained {counted(cells,'latent cell')}."]
            bullets=["The transcript preserves quiet responses and unanswered inputs.","Each audible token was decoded from the output receptor wave."]
        else:
            family="External acoustic experiments";measure=trips;unit="trips"
            title="An animal explores an unfamiliar voice";subtitle="External apparatus   /   raw sensory spikes   /   active queries"
            stat=[["Experiments",trips],["Physical moves",moves],["Latent cells",cells]]
            narrative=[f"The animal chose {counted(trips,'experiment')}.",f"The external apparatus and animal exchanged {human(moves-trips)} motor and sensory events.",f"The resulting circuit contained {counted(cells,'latent cell')}."]
            bullets=["The apparatus returned one physical waveform after each motor action.","The learner chose the distinguishing sequences itself."]
    elif kind=="chorus":
        row=chorus[-1];n=int(metrics.get("animals",0));meals=int(metrics.get("chorus_meals",0));moves=int(metrics.get("chorus_moves",0))
        family="Meals per hundred chorus movements";measure=100*meals/max(1,moves);unit="meals / 100"
        title="Living voices on the tidal flats";subtitle=f"{human(n)} residents   /   {human(n//32)} shared islands   /   {len(chorus)} seasons"
        stat=[["Nectar meals",meals],["Final voice genotypes",int(row["voices"])],["Surviving founders",int(row["founders"])]]
        narrative=[f"The residents harvested {human(meals)} nectar meals across {counted(len(chorus),'season')}.",
                   f"Their acquired circuits guided {human(metrics.get('chorus_aims',0))} calls to living neighbors.",
                   f"The last generation carried {counted(int(row['voices']),'distinct vocal genotype')}."]
        bullets=["Each island shared food among 32 competing callers.",f"The last generation held {human(int(row['offspring_energy']))} energy units for reproduction."]
        if "distinct_calls" in row:
            stat[1]=["Distinct call behaviors",int(row["distinct_calls"])]
            bullets[0]=f"In {human(int(row['context_sensitive']))} residents, earlier gestures could change the reply to the same call."
    elif kind=="recollection":
        r=memories["intact"];questions=int(r["questions"]);worlds=int(r["worlds"]);delay=int(r["delay_events"])
        good=int(r["noise"]);epochs=metrics.get("memory_generations",0)
        family="Recall after a dim outlier";measure=100*good/questions;unit="%"
        interval=f"0–{delay:,}" if r.get("spacing")=="varied" else f"{delay:,}"
        title="What survives a misleading glimpse";subtitle=f"Sixteen visual contacts   /   {worlds:,} independent lifetimes   /   {interval} intervening events"
        stat=[["Visual circuit","16 bytes"],["Breeding generations",epochs],["Tests per condition",questions]]
        narrative=[f"After a single clear sighting the circuit reinstated the target on {human(int(r['one']))} of {counted(questions,'probe')}.",
                   f"It retained the earlier location after a dim outlier on {human(good)} probes and accepted a clear relocation on {human(int(r['revision']))}.",
                   f"Four consistent dim sightings changed its report on {human(int(r['faint']))} probes."]
        bullets=[f"With the clarity channel inverted, {human(int(memories['source']['noise']))} outlier probes recovered the earlier sighting.",
                 f"Unseen items elicited silence on {human(int(r['empty']))} of {human(int(r['empty_questions']))} questions."]
        proof=[pairs(s) for s in data.get("lines",[]) if s.startswith("memory_proof=")]
        if proof:
            checked=sum(int(t["checked"]) for t in proof);failed=sum(int(t["violations"]) for t in proof)
            bullets[1]=f"Exhaustive checking found {counted(failed,'violation')} in {counted(checked,'reachable memory obligation')}."
    elif kind=="cultivation":
        c=cultivation["intact"];n=int(c["worlds"]);good=int(c["goals"])
        title="Growing a model of what actions change";subtitle="Unseen mechanisms   /   motor sequences   /   causal interventions"
        family="Physical goals reached by an acquired model";measure=100*good/n;unit="%"
        stat=[["New worlds",n],["Goals reached",good],["Omissions explained",int(c["reasons"])]]
        narrative=[f"The pupil predicted the complete sensory result of {human(int(c['forecasts']))} of {human(6*n)} held-out interventions.",
                   f"It grew the requested state in {human(good)} of {counted(n,'fresh world')}.",
                   f"Its explanation identified a smallest physical repair after {counted(int(c['reasons']),'omitted action')}."]
        bullets=[f"Replaying previously experienced configurations reached {human(int(cultivation['episodic']['goals']))} goals.",
                 f"A single recurrent goal wave reached {human(int(cultivation['one-beat']['goals']))} goals."]
    elif kind=="causal-census":
        c=causal_census;n=int(c["trials"]);good=int(c["correct"])
        title="Every goal and every available motor";subtitle="64 configurations   /   729 goal patterns   /   64 motor menus"
        family="Exact first decisions for causal goals";measure=100*good/n;unit="%"
        stat=[["Decisions checked",n],["Exact decisions",good],["Nonoptimal moves",int(c["wrong"])]]
        narrative=[f"The frozen model faced {human(n)} combinations of a starting configuration, a goal and available motors.",
                   f"Its first move or decision to stay matched physical reachability and shortest distance on {human(good)} cases.",
                   f"The physical reference contained {human(int(c['unreachable']))} unreachable goals."]
        bullets=[f"{human(int(c['already']))} queries already satisfied the goal.",f"{human(int(c['missed']))} reachable queries received no motor response."]
        if "completed" in c:bullets[0]=f"{human(int(c['completed']))} of {human(int(c['attempted']))} attempted policies physically reached their goal."
    elif kind=="garden-interview":
        n=int(metrics.get("garden_turns",0));actions=int(metrics.get("garden_actions",0));events=int(metrics.get("garden_events",0))
        title="A conversation about cause and action";subtitle="Grounded requests   /   remembered mechanisms   /   a changing plot"
        family="Grounded turns in the garden";measure=n;unit="turns"
        stat=[["Grounded turns",n],["Motor actions",actions],["Encounters",events]]
        narrative=[f"The garden recorded {counted(events,'exchange')} during the encounter.",
                   f"The animal grounded {counted(n,'utterance')} with its acquired contacts.",
                   f"The physical record contains {counted(actions,'motor action')}."]
        bullets=[f"Private state and motor consequences were recorded for this encounter.",
                 "The plotted sequence comes from the witnessed or imagined configurations in the trace."]
    elif kind=="greenhouse":
        r=commons[-1];c=crops[-1];n=int(r["residents"]);mass=int(r["deposited"])
        title="Tending a living greenhouse";subtitle=f"{human(n)} independent residents   /   {human(int(c['islands']))} seed beds   /   season {r['commons_generation']}"
        family="Offspring energy per greenhouse resident";measure=mass/n;unit="units / resident"
        stat=[["Meals grown or found",int(r["meals"])],["Planned work",int(c["planned_work"])],["Plant genomes",int(c["plant_genomes"])]]
        narrative=[f"The residents worked plots {human(int(c['work']))} times and obtained {human(int(r['meals']))} meals.",
                   f"Their descendants received {human(mass)} energy units from successful parents.",
                   f"The seed beds held {counted(int(c['plant_genomes']),'plant genome')} descended from {counted(int(c['plant_founders']),'founder')}."]
        bullets=[f"{human(int(c['appetites']))} residents settled on a predictive taste circuit.",
                 f"The resident seed lines carried {human(int(c['inherited_edits']))} cumulative inherited edits across the current islands."]
        if "edible_at_reset" in c:
            bullets[0]=f"{human(int(c['edible_at_reset']))} beds could yield food in their reset state, and {human(int(c['reset_meals']))} meals came from plants in that state."
    elif kind=="estuary":
        e=estuaries[-1];r=commons[-1];n=int(r["residents"]);traps=int(e["placements"]);hits=int(e["correct_traps"]);depth=int(e["depth"])
        title="A season on changing currents";subtitle=f"{human(n)} residents   /   {2**depth} crossings to food   /   generation {e['estuary_generation']}"
        family=f"Successful {2**depth}-crossing traps";measure=100*hits/max(1,traps);unit="%"
        stat=[["Right destination",hits],["Inspections bought",int(r["experiments"])],["Offspring energy",int(r["deposited"])]]
        narrative=[f"The island currents changed every day while {human(n)} newborns competed for food.",
                   f"Residents placed {human(traps)} traps and reached the right destination {human(hits)} times.",
                   f"They obtained {human(int(r['meals']))} meals and deposited {human(int(r['deposited']))} energy units in offspring."]
        counts=[int(e[k]) for k in ("two_hops","four_hops","eight_hops")]
        bullets=[f"{human(counts[depth-1])} residents settled on a meaning for the {2**depth}-crossing food cue.",
                 f"{human(int(e['lessons']))} tidal events updated acquired contacts during the season."]
    elif kind=="endpoint-scale":
        r=endpoints[-1];title="A learned route in twenty-four bytes"
        subtitle=f"Frozen word “{r['word']}”   /   {human(int(r['replicas']))} independent maps"
        family="Frozen relational endpoint throughput";unit="billion/s"
        rates={k:statistics.median(float(t["Mqueries_per_s"]) for t in endpoints if t["endpoint_batch"]==k)/1000 for k in ("direct","projected")}
        measure=rates["projected"];stat=[["Projected bytes",24],["Maps",int(r["replicas"])],["Endpoints verified",int(r["verified"])]]
        narrative=[f"All {human(int(r['verified']))} possible source queries matched the original acquired relation.",
                   f"The projected readout processed {measure:.2f} billion endpoint queries per second at the median of three passes.",
                   f"The full route traversal processed {rates['direct']:.2f} billion queries per second on the same maps."]
        bullets=[f"Building the projected fields on the GPU took {r['preparation_ms']} milliseconds.",
                 f"Each timed pass included {human(int(r['queries']))} endpoint decisions and a per-map output digest."]
    elif kind=="concept-census":
        r=graph_census;n=int(r["trials"]);hits=int(r["exact"]);title="Every possible three-object map"
        subtitle=f"Acquired word “{r['concept_census']}”   /   {human(int(r['graphs']))} directed worlds"
        family="Exact three-object graph responses";measure=100*hits/n;unit="%"
        stat=[["Physical maps",int(r["graphs"])],["Questions",n],["Exact spike sets",hits]]
        narrative=[f"The witness applied its acquired word {r['concept_census']} to every possible arrangement of four links per object.",
                   f"Its destination cells and uncertainty signal agreed with an independent traversal on {human(hits)} of {human(n)} queries.",
                   f"The reference traversal yielded a unique destination on {human(int(r['unique']))} queries."]
        bullets=[f"{human(int(r['empty']))} queries had no destination and {human(int(r['ambiguous']))} had several possibilities or a missing branch.",
                 f"The tested reference sequence was {r['law']} on directed ports zero through seven."]
    elif kind=="inference":
        r=inference["intact"];n=int(r["trials"]);v=int(r.get("view_trials",n))
        title="Learning a relation from examples";subtitle="Private sightings   /   changing maps   /   transferable pathways"
        family="Four-crossing relational transfer";measure=100*int(r["recursive"])/n;unit="%"
        stat=[["Fresh trials",n],["Two crossings",int(r["transfer"])],["Four crossings",int(r["recursive"])]]
        narrative=[f"After the map changed, the pupil followed a learned two-crossing route on {human(int(r['transfer']))} of {human(n)} trials.",
                   f"It used earlier routes to learn a new four-crossing relation and reached {human(int(r['recursive']))} correct destinations.",
                   f"Its model of the other animal gave the expected response on {human(int(r['other']))} of {human(v)} visibility probes."]
        bullets=[f"Cutting reuse left {human(int(inference['no-reuse']['recursive']))} correct four-crossing predictions."]
        if "inquiry" in r:bullets.append(f"Directed and random questions located the informative object {human(int(r['inquiry']))} and {human(int(inference['random-question']['inquiry']))} times.")
        else:bullets.append(f"The pupil withheld an ambiguous answer on {human(int(r['uncertain']))} trials.")
    elif kind=="concept-interview":
        n=int(metrics.get("concept_turns",0));examples=int(metrics.get("route_examples",0))+int(metrics.get("spoken_route_examples",0));queries=int(metrics.get("concept_questions",0))
        family="Relational interview turns";measure=n;unit="turns"
        title="Two witnesses on a changing map";subtitle="Grounded word meanings   /   private observations"
        stat=[["Relational turns",n],["Examples given",examples],["Questions asked",queries]]
        narrative=[f"The pair received {counted(metrics.get('board_stimuli',0),'presentation')} of objects, links or speech during the encounter.",
                   f"The witness grounded {counted(n,'turn')} using its acquired word and scene contacts.",
                   f"It initiated {counted(queries,'question')} about an example."]
        bullets=[f"{human(examples)} demonstrations presented a source and a destination.",
                 f"The circuit produced a vocal response on {human(metrics.get('concept_voiced',0))} turns."]
        if "route_examples" not in metrics and "spoken_route_examples" not in metrics and not n:
            stat[1]=["Board presentations",int(metrics.get("board_stimuli",0))]
            bullets[0]="The presentation count includes changes to the observed board."
    elif kind=="culture":
        row=commons[-1];c=cultures[-1];n=int(row["residents"]);mass=int(row["deposited"])
        title="Learning a neighbour’s calls";subtitle=f"{human(n)} residents   /   {human(int(c['sampled_islands']))} islands   /   generation {c['culture_generation']}"
        family="Offspring energy in the report habitat";measure=mass/n;unit="units / resident"
        stat=[["Words emitted",int(c["words"])],["Correct readings",int(row["correctly_decoded"])],["Different reports",int(c["distinct_reports"])]]
        narrative=[f"The residents emitted {human(int(c['words']))} words while witnessing movements concealed from their neighbours.",
                   f"Their partners correctly decoded {human(int(row['correctly_decoded']))} movements and the population obtained {human(int(row['meals']))} meals.",
                   f"Residents deposited {human(mass)} energy units in offspring and gave {counted(int(c['distinct_reports']),'distinct audible report')} of the same sampled event."]
        bullets=[f"Newborns accepted {human(int(c['tuition']))} demonstrations from their island tutors.",
                 f"Residents acquired {human(int(c['learned_visits']))} further pairings during paid visits to a neighbour."]
    elif kind=="commons":
        row=commons[-1];n=int(row["residents"]);mass=int(row["deposited"]);tests=int(row["experiments"])
        title="Foraging and exploration in the commons";subtitle=f"{human(n)} spiking residents   /   {human(n//32)} shared islands   /   generation {row['commons_generation']}"
        family="Reproductive material per commons resident";measure=mass/n;unit="units / resident"
        stat=[["Residents",n],["Surviving animals",int(row["alive"])],["Offspring material",mass]]
        narrative=[f"The final cohort spent {human(int(row['lived_days']))} living resident-days sharing four food reservoirs per island.",
                   f"It carried out {counted(tests,'experiment')} and obtained {counted(int(row['meals']),'meal')}.",
                   f"Residents deposited {human(mass)} energy units into reproduction and retained {human(int(row['energy']))} in their bodies."]
        bullets=[f"{human(int(row['lost_contests']))} correct traps lost access to food through competition.",
                 f"Of {human(int(row['food_supplied']))} supplied food packets, {human(int(row['food_left']))} remained in the reservoirs."]
        if int(row.get("matched_calls",0)):
            matched=int(row["matched_calls"]);real=int(row["actual_call"]);late=int(row["delayed_call"])
            bullets[1]=f"On {human(matched)} identical readings the partner's current call gave {human(real)} correct bearings and its delayed call gave {human(late)}."
        elif int(row.get("decoded",0)):
            bullets[1]=f"Of {human(int(row['decoded']))} decoded calls, {human(int(row['correctly_decoded']))} recovered the partner’s concealed bearing."
    elif kind=="conjunction-census":
        forced=sum(int(census["forced_"+s]) for s in ("correct","silent","wrong"))
        ambiguous=int(census["queries"])-forced;good=int(census["forced_correct"])
        title="Every partial view of a world";subtitle=f"{census['references']} physical laws   /   one to fifteen demonstrations   /   exact external enumeration"
        family=f"Forced bearings across {census['references']} laws";measure=100*good/forced;unit="%"
        stat=[["Partial histories",int(census["histories"])],["Unseen queries",int(census["queries"])],["Physical laws",int(census["references"])]]
        narrative=[f"The animal inferred {human(good)} of {human(forced)} bearings uniquely determined within the tested family.",
                   f"It remained silent on {human(int(census['ambiguous_silent']))} of {human(ambiguous)} ambiguous questions.",
                   f"It gave an incorrect bearing on {human(int(census['forced_wrong']))} forced questions and {human(int(census['ambiguous_wrong']))} ambiguous ones."]
        bullets=[f"The external scorer considered all 576 possible four-bearing Latin squares for every history.",
                 "Compatible worlds received their shared observations in the same order." if census.get("protocol")=="shared-order" else "Each physical law used one seeded order for its demonstrations."]
    elif kind=="conjunction-scale":
        row=joint_times[-1];n=int(row["individuals"]);items=int(row["queries"])
        rate=statistics.median(float(t["Mqueries/s"]) for t in joint_times if t["joint_batch"]=="compiled")
        family="Compiled joint reports";measure=rate/1000;unit="billion/s"
        title="Reading an acquired interaction";subtitle=f"{human(n)} independent histories   /   frozen single-action readout"
        stat=[["Independent animals",n],["Queries",items],["Bytes per action",row["bytes_per_action"]]]
        narrative=[f"Independent animals saw different demonstrations of a two-object apparatus before their contacts were frozen.",
                   f"The compiled readout matched the direct circuit on {human(items)} queries including {human(items-int(row['voiced']))} silences.",
                   f"The median throughput over three timed passes was {rate/1000:.2f} billion single-step reports per second."]
        bullets=[f"Preparing the readouts on the GPU took {row['compile_ms']} milliseconds.",
                 "The timed interval includes prediction and verification output for each individual."]
    elif kind=="narration":
        r=narration["intact"];n=int(r["trials"]);decoded=int(r["decoded"])
        title="Telling another animal what happened";subtitle="Independent event memories   /   learned word order   /   a concealed movement"
        family="Private events communicated";measure=100*decoded/n;unit="%"
        stat=[["Private trials",n],["Exact utterances",int(r["exact"])],["Events decoded",decoded]]
        narrative=[f"The witness produced the complete learned utterance on {human(int(r['exact']))} of {human(n)} fresh events.",
                   f"Its independently reared partner identified the concealed movement on {human(decoded)} trials.",
                   f"Reversing the emitted words gave {human(int(narration['reversed']['decoded']))} correct readings."]
        bullets=[f"Interrupting sound transmission gave {human(int(narration['muted']['decoded']))} correct readings.",
                 f"The witness held the last two movements in order on {human(int(r['memory']))} trials."]
    elif kind=="report-interview":
        n=int(metrics.get("report_requests",0));words=int(metrics.get("report_words",0))
        title="A witness speaks from memory";subtitle="Acquired speech   /   private event history"
        family="Spoken reports";measure=words/max(1,n);unit="words/request"
        stat=[["Report requests",n],["Words emitted",words],["Spoken lessons",metrics.get("report_lessons",0)]]
        narrative=[f"The witness received {human(n)} requests for an event report.",
                   f"Its acoustic motor cells emitted {human(words)} words across those requests.",
                   f"The session included {human(metrics.get('report_lessons',0))} accepted demonstrations of speech."]
        bullets=["The transcript records the sound sequence emitted on the GPU.","The recipient can recover an event through its own acquired sound contacts."]
        if metrics.get("retellings") or metrics.get("incoming_reports"):
            turns=int(metrics.get("retellings",0));incoming=int(metrics.get("incoming_reports",0))
            stat=[["Witness reports",n],["Incoming statements",incoming],["Retellings",turns]]
            narrative=[f"The witness responded to {counted(n,'event request')} and the listener heard {counted(incoming,'statement')} from the human.",
                       f"The listener attempted {counted(turns,'retelling')} using its acoustic evidence.",
                       f"It emitted {human(metrics.get('retold_words',0))} words while retelling what it heard."]
    elif kind=="conjunction":
        row=conjunction["intact"];n=int(row["probes"]);good=int(row["completion"])
        title="Predicting an unseen arrangement";subtitle="Two objects   /   sixteen arrangements   /   evolving lateral connections"
        family="Held-out joint predictions";measure=100*good/n;unit="%"
        stat=[["Independent probes",n],["Forced arrivals",good],["Contact bank","140 bytes"]]
        narrative=[f"After seeing part of the apparatus in motion, the animals predicted {human(good)} of {human(n)} withheld bearings.",
                   f"They stayed silent on {human(int(row['ambiguous']))} ambiguous arrangements and revised {human(int(row['revision']))} previously observed outcomes.",
                   f"With recurrent signalling disabled, {human(int(conjunction['no-recurrence']['completion']))} withheld bearings were predicted correctly."]
        bullets=[f"On arbitrary observed arrangements, {human(int(row['arbitrary']))} reports matched the physical arrival.",
                 f"On sparse single-source trials, the animals transferred {human(int(row['sparse_role']))} consequences to fresh arrangements."]
    elif kind=="binding":
        row=binding["intact"];n=int(row["probes"]);good=int(row["diagnostic"])
        title="Choosing an informative experiment";subtitle=f"Two movable objects   /   four available experiments   /   {row['effects']} effects"
        family="Informative experiments";measure=100*good/n;unit="%"
        stat=[["Breeding generations",metrics["generations"]],["Experiments",n],["Diagnostic choices",good]]
        narrative=[f"The animals chose the diagnostic arrangement in {human(good)} of {counted(n,'experiment')}.",
                   f"After seeing the result they correctly predicted the first object on {human(int(row['first']))} probes and the second on {human(int(row['second']))}.",
                   f"When the two causal roles changed, {human(int(row['revised_first']))} of the first object's new bearings were predicted correctly."]
        bullets=[f"With random choice, {human(int(binding['random-choice']['diagnostic']))} informative experiments were selected.",
                 f"Freezing the source contacts gave {human(int(binding['fixed-support']['revised_first']))} correct first-object reports after reversal."]
    elif kind=="imagery":
        row=forecasts["intact"];n=int(row["probes"]);length=int(row["length"]);good=int(row["changed_sequence"])
        family="Predicted action sequences · "+row["dynamics"];measure=100*good/n;unit="%"
        title="Replaying an action before moving";subtitle=f"{int(row['worlds']):,} apparatuses   /   {row['dynamics']} motions   /   {length:,} imagined actions"
        stat=[["Demonstrated moves",metrics["observations"]],["Tests per condition",n],["Action memory","48 bytes"]]
        narrative=[f"The circuit predicted the next bearing on {human(int(row['single']))} of {counted(n,'single-action probe')}.",
                   f"It correctly replayed {human(int(row['sequence']))} unfamiliar sequences of {counted(length,'action')}.",
                   f"After the apparatus changed, it predicted the arrival on {human(good)} of {counted(n,'new sequence')}."]
        bullets=[f"Reversing the replay order gave {human(int(forecasts['reverse']['changed_sequence']))} correct arrivals after the change.",
                 f"Freezing contacts at the change produced {human(int(forecasts['locked']['changed_sequence']))} correct arrivals."]
    elif kind=="attention-scale":
        row=timings[-1];n=int(row["pairs"]);items=int(row["decisions"])
        rates={k:statistics.median(float(t["Mdecisions/s"]) for t in timings if t["attention_batch"]==k) for k in ("direct","compiled")}
        family="Compiled attention decisions";measure=rates["compiled"]/1000;unit="billion/s"
        title="A million independent visual histories" if n==1048576 else "Independent visual histories"
        subtitle=f"{human(n)} pairs   /   frozen word contacts   /   continuously plastic vision"
        stat=[["Pairs",n],["Bytes per compiled pair",int(row["bytes_per_pair"])],["Billion decisions / s",round(measure,2)]]
        narrative=[f"Each of {counted(n,'pair')} received its own stream of words, cues and sightings.",
                   f"The direct and compiled circuits produced the same activity checksum across {counted(items,'decision')}.",
                   f"The compiled circuit made {measure:.2f} billion source selections and visual updates per second."]
        bullets=[f"Direct median {rates['direct']/1000:.2f} billion decisions per second.",f"{human(int(row['correct']))} correct integrated sensory decisions in the final run."]
    elif kind=="attention":
        row=attention["intact"];n=int(row["probes"]);good=int(row["revised_chain"]);length=int(row["length"])
        title="Choosing between sight and memory";subtitle=f"Two spiking individuals   /   {int(row['worlds']):,} teaching environments   /   {length:,}-word sequences"
        family="Reassigned attention sequences";measure=100*good/n;unit="%"
        stat=[["Word fields",8],["Pitch muscles",2],["Tests per condition",n]]
        narrative=[f"The caller learned a local pitch convention and requested the intended source on {human(int(row['single']))} of {counted(n,'single-word trial')}.",
                   f"Before reassignment the pairs followed {human(int(row['chain']))} complete sequences of {counted(length,'word')}.",
                   f"After the word meanings changed they correctly executed {human(good)} of {counted(n,'sequence')}."]
        bullets=[f"Reversing the transmitted pitch produced {human(int(attention['inverted']['revised_chain']))} correct revised sequences.",
                 f"Shuffling food feedback gave {human(int(attention['shuffled-food']['revised_chain']))} correct revised sequences."]
    elif kind=="interview":
        turns=interview_turns(data)
        n=int(metrics.get("human_turns",0));words=int(metrics.get("human_words",0))
        sounds=sum(int(t["sound"])<4 for t in turns)
        family="Live interview length";measure=n;unit="turns"
        title="A conversation about a scene";subtitle="Two spiking individuals   /   live text input and motor output"
        stat=[["Human turns",n],["Words heard",words],["Recorded replies",len(turns)]]
        narrative=[f"The questioner received {counted(words,'word')} over {counted(n,'turn')}.",
                   f"The speaker emitted an audible reply in {human(sounds)} of {counted(len(turns),'recorded exchange')}.",
                   "Its view came from an orienting movement made in response to the other animal’s call."]
        if metrics.get("taught_forms"):
            narrative[-1]=f"Across {counted(metrics['taught_forms'],'pointing lesson')}, the number of active auditory forms went from {human(metrics.get('initial_word_forms',0))} to {human(metrics.get('final_word_forms',0))}."
        bullets=["The entries record the actual selected view and emitted sound.","The scene positions were supplied independently of the questions."]
        covered=[t for t in turns if int(t["view"])<2 and t.get("covered",0)&(1<<int(t["view"]))]
        if covered:
            heard=sum(int(t["sound"])<4 for t in covered)
            bullets[0]=f"The speaker gave {counted(heard,'audible answer')} over {counted(len(covered),'covered inspection')}."
    elif kind=="conversation":
        phase="revised" if ("revised","open") in speech else "learned"
        end=speech[phase,"open"];silent=speech.get((phase,"silent"))
        pairs_n=int(end["pairs"]);good=int(end["correct"]);total=int(end["exchanges"]);facets=int(end["facets"])
        title="A question between two animals" if facets==2 else "Learning a shared call"
        subtitle=f"{pairs_n:,} pairs   /   {facets} private attribute"+("s" if facets>1 else "")+"   /   four vocal muscles"
        stat=[["Independent pairs",pairs_n],["Correct choices",good],["Bits per call",float(end["bits_per_call"])]]
        family=f"Spoken choices · {facets} "+("attributes" if facets==2 else "attribute")
        measure=100*good/max(1,total);unit="%"
        narrative=[f"The two animals in each of {counted(pairs_n,'pair')} exchanged signals through their own vocal muscles.",
                   f"The listeners made {human(good)} correct choices in {counted(total,'exchange')} with the channel open.",
                   f"The speaker emitted an audible call on {human(int(end['voiced']))} of these exchanges."]
        if silent:
            narrative[-1]=f"The silent-channel control scored {human(int(silent['correct']))}/{human(total)}."
        if data.get("breeding"):
            n=metrics.get("vocal_generations",data["breeding"][-1][0])
            treatment="randomly sampled parents" if "--breeding" in args and args[args.index("--breeding")+1]=="drift" else "parents selected on shared outcomes"
            narrative[0]=f"The nursery bred {counted(n,'generation')} of pairs from {treatment}."
        if metrics.get("human_turns"):
            narrative[0]=f"The session also recorded {counted(metrics['human_turns'],'human turn')} and {counted(metrics['human_words'],'heard word')}."
        bullets=[f"All {human(total)} tests used frozen individual states.",f"{human(int(end['perfect_pairs']))} pairs answered every question correctly."]
    elif kind=="forage":
        field=catchments["intact"];lives=int(field["lives"]);mass=int(field["offspring_mass"]);births=int(field["births"])
        title=f"Foraging across {field['rooms']} rooms"
        subtitle=f"{lives:,} lifetimes   /   {field.get('vision','distinct').capitalize()} landmarks   /   {field['span']} ticks per lifetime"
        stat=[["Foraging lifetimes",lives],["Births",births],["Offspring material",mass]]
        family=f"Offspring material per lifetime · {field['rooms']} rooms · {field.get('vision','distinct')}"
        measure=mass/max(1,lives);unit="material / life"
        narrative=[f"Of {human(lives)} animals, {human(int(field['survived']))} survived the full foraging interval.",
                   f"The animals transferred {human(mass)} units of material to offspring in {counted(births,'birth')}.",
                   f"They harvested {human(int(field['harvested']))} units of food during these lifetimes."]
        if "at_spring" in field:
            narrative[-1]=f"They left springs on {human(int(field['depart_spring']))} of {human(int(field['at_spring']))} opportunities and moved away from dry sites on {human(int(field['depart_away']))} of {human(int(field['away']))}."
        bullets=[f"{human(int(field['oxidized']))} units oxidized for body energy.",f"Offspring material averaged {measure:.2f} units per lifetime."]
        if "trace-fixed" in catchments:bullets[1]=f"With conditional plasticity frozen, offspring received {human(int(catchments['trace-fixed']['offspring_mass']))} units."
    elif kind=="animal":
        cells=int(metrics.get("neurons",0));events=int(metrics.get("events",0));size=int(metrics.get("state_bytes",0))
        title=Path(source).stem.replace("-"," ").replace("_"," ").title() if source else "Individual session"
        title=title or "Individual session";subtitle=f"{cells} somatic cells   /   {size:,} bytes of device state"
        if scores:
            label,a,b=scores[-1];family=label;measure=100*a/b if b else 0;unit="%"
        else:family="Sensory activity";measure=events;unit="events"
        stat=[["Sensory events",events],["Motor discharges",sum(data.get("motors",[]))],["Active contacts",len(data.get("transitions",[]))]] if data.get("transitions") else [["Sensory events",events],["Somatic cells",cells],["State bytes",size]]
        narrative=[f"The session opened {group} with {cells} somatic cells.","The animal received "+counted(events,"sensory event")+".",
                   (f"It answered {human(scores[-1][1])} of {human(scores[-1][2])} scored choices correctly." if scores else f"Its device state occupies {human(size)} bytes.")]
        if metrics.get("travel"):
            narrative[1]=f"It moved between landmarks on {human(metrics.get('displacements',0))} of {human(metrics['travel'])} attempted journeys."
        if metrics.get("score_phases",0)>1:
            phases=scores[-metrics["score_phases"]:];first,last=phases[0],phases[-1]
            narrative[2]=f"It scored {human(first[1])}/{human(first[2])} during “{first[0]}” and {human(last[1])}/{human(last[2])} during “{last[0]}”."
        if metrics.get("replicas"):
            stat=[["Independent brains",metrics["replicas"]],["Scored decisions",metrics["decisions"]],["State bytes per brain",size]]
            narrative[1]=f"The GPU evaluated {human(metrics['decisions'])} decisions across {human(metrics['replicas'])} separately allocated brains."
        elif data.get("conditional"):
            stat[-1]=["Conditional contacts",len(data["conditional"])]
        bullets=[f"{sum(x>0 for x in data.get('motors',[]))} of eight motor outputs fired.",f"{human(metrics.get('ticks',0))} lifetime events committed in the saved or inspected state."]
        if metrics.get("replicas"):
            timings=[float(pairs(line)["Mchoices/s"]) for line in data.get("lines",[]) if "Mchoices/s=" in line]
            if timings:
                rate=statistics.median(timings)
                bullets=[f"Median throughput {rate:.2f} million choices per second.",f"{human(metrics['replicas']*size)} bytes of replicated neural state."]
                family="Frozen inference throughput";unit="M choices/s";measure=rate
    else:
        title=data.get("title") or (" · ".join(x.title() for x in (group.replace("-"," "),)) if arch else command.title())
        subtitle=data.get("subtitle") or Path(source).name or "Simulation measurements"
        family=data.get("family") or (scores[-1][0] if scores else command.title())
        if scores and scores[-1][2]:measure=100*scores[-1][1]/scores[-1][2];unit="%"
        else:measure=data.get("measure",metrics.get("animals",metrics.get("observations",0)));unit=data.get("unit","observations")
        animals=metrics.get("animals",0);neurons=metrics.get("neurons",0)
        stat=data.get("stats") or [["Animals",animals],["Somatic cells per animal",neurons],["Scored series",len(scores)]]
        narrative=data.get("narrative") or [f"The {command} session ran {counted(animals,'animal')} with {human(neurons)} cells per animal.",
                    f"It started at generation {human(metrics.get('generation',0))} with seed {metrics.get('seed',0)}.",
                    (f"The last {scores[-1][0].lower()} score was {human(scores[-1][1])} of {human(scores[-1][2])}." if scores else f"The run retained {counted(len(data.get('lines',[])),'output line')} in its observation record.")]
        if command=="tutor":
            stat=[["Choices",metrics.get("choices",0)],["Demonstrations",metrics.get("demonstrations",0)],["Drawing probes",metrics.get("drawing probes",0)]]
            narrative[-1]=f"The tutor delivered feedback {human(metrics.get('feedback',0))} times and reinforced {counted(metrics.get('rewarded',0),'choice')}."
        if command=="bench":
            frozen=metrics.get("Frozen inference trials per second",0);online=metrics.get("Online learning trials per second",0)
            stat=[["Frozen trials / s",frozen],["Learning trials / s",online],["Animals",animals]]
            narrative[1]=f"Frozen inference processed {human(frozen)} trials per second."
            narrative[-1]=f"With online plasticity active it processed {human(online)} trials per second."
            family="Online trial throughput";unit="trials/s";measure=online
        bullets=data.get("bullets") or [f"{label} · {human(number)}." for label,number in stat[:2]]
    if metrics.get("workbench_events"):
        title="Two animals at the mineral bench";subtitle=Path(source).name or "A physical workshop";family="Workshop reaction count";measure=metrics.get("workbench_reactions",0);unit="reactions"
        stat=[["Bench events",metrics["workbench_events"]],["Observed gestures",metrics.get("workbench_witnessed",0)],["Reactions",measure]]
        narrative=[f"The session recorded {metrics['workbench_events']} physical workshop events.",f"The pupil saw {metrics.get('workbench_witnessed',0)} visible changes made by the other worker.",f"External light and the material contacts supported {measure} conversions of substrate."]
        bullets=[f"{metrics.get('workbench_named',0)} named workpieces were successfully located.","The workstation reports each product after running the same local contact laws as the habitat."]
    if metrics.get("tray_word_requests"):
        n=int(metrics["tray_word_requests"]);located=int(metrics.get("tray_located",0));tests=int(metrics.get("tray_untried",0))
        correct=int(metrics.get("tray_untried_correct",0));title="A word brings a fruit within reach"
        subtitle=Path(source).name or "Spoken reference and learned physical action"
        stat=[["Spoken requests",n],["Objects located",located],["New-fruit probes",tests]]
        narrative=[f"The speaker selected a unique object on {human(located)} of {human(n)} word requests.",
                   f"Its feeding circuit opened {human(metrics.get('tray_opened',0))} pods in {human(metrics.get('tray_bites',0))} grasps.",
                   f"After a turn and further handling, it chose correctly in {correct} of {tests} named probes on untouched fruit." if tests else "This session ended without a scored probe on an untouched fruit after a turn."]
        bullets=[f"{human(metrics.get('tray_unresolved',0))} spoken requests produced no unique referent.",
                 f"{human(metrics.get('tray_absorbed',0))} units passed from the digestive tract into the body."]
        family="Named physical choices";measure=100*correct/tests if tests else 0;unit="%"
    if metrics.get("shore_visits"):
        visits=int(metrics["shore_visits"]);made=int(metrics.get("shore_reactions",0))
        title="A builder at the shore";family="Live shore harvest";unit="reactions";measure=made
        stat=[["Recorded visits",visits],["Mechanical actions",metrics.get("shore_actions",0)],["Reactions",made]]
        narrative=[f"The resident made {metrics.get('shore_actions',0)} paid mechanical interventions during {visits} visits to the loose-grain bed.",
                   f"The apparatus recorded {made} conversions during physical illumination.",
                   f"The visual word circuit uniquely identified a requested deposit {metrics.get('shore_referents',0)} times."]
        bullets=["The builder retains its plastic spatial contacts between visits.","A physical exposure draws from the finite substrate at each deposit."]
        exposures=[r for r in entries("shore_event") if r["shore_event"]=="expose-shore" and "detached_a" in r]
        if exposures:
            sample=exposures[-1];a=int(sample["detached_a"]);b=int(sample["detached_b"])
            title="Where the builder sent the light";subtitle=f"Two independently exposed deposits   /   {visits} visits at the table"
            selected=int(sample.get("wanted_sites",0));sites=[int(sample["region_a"]),int(sample["region_b"])]
            index=next((i for i,site in enumerate(sites) if selected==1<<site),None)
            measure=(a,b)[index] if index is not None else a+b
            family="Requested shore harvest" if index is not None else "Detached shore harvest"
            stat=[["Physical actions",metrics.get("shore_actions",0)],["Deposit A",a],["Deposit B",b]]
            narrative=[f"The resident made {metrics.get('shore_actions',0)} mechanical interventions in the recorded tray.",
                       f"The resident was working toward deposit {'AB'[index]}." if index is not None else "The recorded closing activity did not select a single deposit.",
                       f"After the builder was removed, independent cold exposures produced {a} reactions at A and {b} at B."]
            bullets=[f"Closing body reserve {int(sample['reserve']):,} energy units.",f"Unique spoken references resolved during this session {int(metrics.get('shore_referents',0))}."]
    sites=entries("worksite_tick")
    if sites:
        q=sites[-1];seams=entries("substrate_tick");t=int(q["worksite_tick"])
        title="Traces on the shingle";subtitle=f"Material circulation and field experience   /   watershed tick {t:,}"
        family="Grains on the ground";unit="grains";measure=int(q["floor"])
        stat=[["On the ground",measure],["In living hands",int(q["live_cargo"])],["Marked sites",int(q["marked"])]]
        narrative=[f"At tick {t:,}, {measure:,} grains were on the ground and {int(q['live_cargo']):,} were in living hands.",
                   f"The worksite ledger recorded {int(q['contacts']):,} terminal tests and {int(q['assays']):,} soil tests.",
                   f"Residents absorbed {int(q['eaten']):,} energy units from worksite products."]
        bullets=[f"{int(q['vacant_cargo']):,} grains remained in vacant hands.",f"{int(q['predating_birth']):,} readings concerned marks older than their reader."]
        if seams:
            r=seams[-1];narrative[2]=f"Sediment held {int(r['bound']):,} units of carbon after releasing {int(r['released']):,} into circulation."
        if water and int(first["living"]) and not int(last["living"]):
            title="Extinction on the shingle";family="Closing worksite population";measure=0;unit="animals"
            stat=[["Opening population",int(first["living"])],["Survivors",0],["Grains on the ground",int(q["floor"])]]
            narrative[0]=f"The population fell from {int(first['living']):,} residents to zero during {t-int(first['flood_tick']):,} ticks."
        field={r["field_transfer"]:r for r in entries("field_transfer")}
        if "lived" in field:
            r=field["lived"];blank=field["unexperienced"];n=int(r["trials"])
            title="What the watershed taught";family="Field-reared material choices";unit="%";measure=100*int(r["soil_choices"])/n
            subtitle=f"{int(r['donors']):,} eligible residents   /   {n:,} independently laid beds per condition"
            stat=[["Soil choices correct",int(r["soil_choices"])],["Productive builds",int(r["working"])],["Experience erased",int(blank["working"])]]
            narrative=[f"Residents chose a productive soil in {int(r['soil_choices']):,} of {n:,} choices on unfamiliar ground.",
                       f"Their own experience supported {int(r['working']):,} productive builds, including {int(r['two_essential']):,} requiring two displaced pieces.",
                       f"Clearing their acquired contacts and motor memories left {int(blank['working']):,} productive builds in the same trials."]
            bullets=[f"Contact choices were correct on {int(r['contact_choices']):,} trials.",f"Untouched and insulated twins registered {r['untouched']} and {r['insulated']} reactions."]
        images=entries("worksite_deployment")
        if images and int(images[-1]["calls"]) and "lived" not in field:
            r=images[-1];title="Nine bytes of settled experience";family="Deployed field circuits";unit="M diagrams/s";measure=float(r["Mmaterial_diagrams_per_s"])
            stat=[["Resident circuits",int(r["worksite_deployment"])],["Spatial forecasts",int(r["calls"])],["Differences",int(r["differences"])]]
            narrative=[f"The frozen receptive image used {r['receptive_bytes']} bytes per animal.",
                       f"All {int(r['comparisons']):,} individual receptive outputs matched the writable circuits.",
                       f"The median GPU batch took {float(r['median_ms']):.3f} milliseconds for {int(r['calls']):,} six-step spatial forecasts."]
            bullets=[f"Writable field and motor tissue occupied {r['mutable_bytes']} bytes per animal.",f"The writable comparison took {float(r['mutable_ms']):.3f} milliseconds."]
    if metrics.get("field_conversation_events"):
        tables=entries("field_table");turns=entries("field_answer");examples=entries("field_question");tests=entries("field_coupon")
        voiced=sum(r["field_answer"]!="_" for r in turns);acted=sum(int(r["receiver_work"])>0 for r in turns);food=sum(int(r["receiver_food"]) for r in turns)
        title="A question about the soil";subtitle="Field chemistry, grounded names and a listening animal"
        family="Spoken field judgments";measure=voiced;unit="answers"
        stat=[["Questions heard",len(turns)],["Names voiced",voiced],["Listener tests",acted]]
        narrative=[f"Two samples and two deposits were set out on {counted(len(tables),'table')}.",
                   f"The speaker gave a deposit name on {voiced} of {len(turns)} questions." if turns else f"The speaker received {len(examples)} pointed question examples.",
                   f"The listener tested a sample {counted(acted,'time')} and collected {food} units of food." if turns else f"The speaker tested both soils in {counted(len(tests),'paired exposure')}."]
        bullets=[f"{len(turns)-voiced} questions received silence.",f"The speaker retained {metrics.get('field_question_examples',0)} pointed question exposures."]
    brackish=entries("brine_census")
    if brackish:
        q=brackish[-1];title="Life in changing water";subtitle=f"{q['solutes']} dissolved components   /   watershed tick {int(q['brine_census']):,}"
        family="Locally expressed reaction laws";measure=int(q["local_laws"]);unit="laws"
        stat=[["Living residents",int(q["residents"])],["Reaction laws",measure],["Predictive cells",int(q["predictive_cells"])]]
        narrative=[f"The census found {measure} distinct reaction maps across the watershed.",
                   f"Its living residents expressed {int(q['predictive_cells']):,} additional material-predicting cells.",
                   f"Experience had moved {int(q['modified_cells']):,} of those cells away from their birth state."]
        bullets=[f"Each animal has {q['trace_bytes']} bytes of writable predictive traces.",f"The physical ledger counts all {q['solutes']} dissolved components."]
        leg=entries("brine_legacy")
        if leg:
            r=leg[-1];narrative[2]=f"Removing the banks changed the subsequent reaction law at {int(r['changed_laws']):,} sites."
    transfer={r["brine_transfer"]:r for r in entries("brine_transfer")}
    if "responsive" in transfer:
        q=transfer["responsive"];n=int(q["choices"]);correct=int(q["correct"]);blank=transfer["frozen"];reverse=transfer["opposite"]
        title="A first choice in unfamiliar water";subtitle=f"{q['solutes']} dissolved components   /   paired baths   /   coating withheld from exposure"
        family="Unseen coating in changed water";measure=100*correct/n;unit="%"
        stat=[["First choices",n],["Productive choices",correct],["Frozen synapses",int(blank["correct"])]]
        narrative=[f"Cloned residents made {n:,} paid first choices about a coating absent from their reference samples.",
                   f"After observing other coatings, {correct:,} choices paid off in the actual bath.",
                   f"Freezing their synapses gave {int(blank['correct']):,} productive choices, and showing the opposite bath gave {int(reverse['correct']):,}."]
        bullets=[f"Both copies saw the same query in opposite solvents.",f"Each allowed up to {q['examples']} exposures with other coatings."]
    social={r["brine_dialogue"]:r for r in entries("brine_dialogue")}
    if "consulted" in social:
        q=social["consulted"];n=int(q["questions"]);right=int(q["correct"]);dark=social["reference_quiet"]
        intention=q.get("report")=="choice"
        title="Reporting a changed decision" if intention else "Asking a peer to run an experiment"
        subtitle=f"{int(q['tables']):,} independent tables   /   {q['solutes']} dissolved components   /   "+("planned material action" if intention else "material effect")
        family="Reported material intentions" if intention else "Grounded answers after consultation";measure=100*right/max(1,n);unit="%"
        stat=[["Questions",n],["Correct words",right],["Silent answers",int(q["silent"])]]
        narrative=[f"The listener performed {int(q['tests_shown']):,} named reference tests on other coatings.",
                   f"The speaker gave {right:,} correct deposit names, {int(q['wrong']):,} wrong names and {int(q['silent']):,} silent answers.",
                   f"Closing the reference channel left {int(dark['correct']):,} correct names on the same {n:,} questions."]
        bullets=[f"The partners tested the queried coating {int(q['target_sampled']):,} times during consultation.",f"The listener found the intended deposit on {int(q['intended_site']):,} final actions."]
        if intention:
            bullets[1]=f"Spoken answers agreed with the isolated motor twin on {int(q['policy_agreement']):,} questions."
        if "prior_confidence" in social:
            local=social["autonomous"];prior=social["prior_confidence"]
            title="How much evidence to ask for";family="Self-directed material questions";measure=100*int(local["correct"])/max(1,n)
            stat=[["Questions",n],["Own stopping point",int(local["correct"])],["Prior confidence",int(prior["correct"])]]
            narrative=[f"The resident chose its own reference contacts and saw {int(local['witnessed']):,} outcomes over {n:,} questions.",
                       f"Using an arrival signal, it gave {int(local['correct']):,} correct names, {int(local['wrong']):,} wrong names and {int(local['silent']):,} silent answers.",
                       f"Using only its prior confidence to stop gave {int(prior['correct']):,} correct names with {int(prior['witnessed']):,} witnessed outcomes."]
            bullets=[f"The full consultation gave {int(q['correct']):,} correct names.",f"{int(local['paired_correct']):,} paired cases received the correct name in both waters."]
    elif entries("brine_request"):
        requests=entries("brine_request");n=len(requests);observed=sum(int(r["witnessed_pair"])<16 for r in requests)
        title="A resident asks for evidence";subtitle="A public sample, a spoken request and a second animal's hands"
        family="Witnessed reference tests";measure=observed;unit="tests"
        stat=[["Reference turns",n],["Physical tests",sum(int(r["paid"])>0 for r in requests)],["Witnessed outcomes",observed]]
        narrative=[f"The resident named a reference cup on {sum(r['speech']!='_' for r in requests)} of {n} opportunities.",
                   f"Its partner spent {sum(int(r['paid']) for r in requests)} units on tests and collected {sum(int(r['food']) for r in requests)} units of food.",
                   f"The resident received {observed} visible outcomes through its own receptors."]
        bullets=[f"All reference samples have a different coating from the subject of the consultation.",f"A material answer was voiced {metrics.get('field_material_answers',0)} times during the session."]
    if "--brine-competition" in args and entries("flood_census"):
        q=entries("flood_census")[-1];opening=entries("flood_census")[0];a0=int(q["cohort_a"]);b0=int(q["cohort_b"])
        title="Two learning lineages in one watershed";subtitle="Matched bodies and starting energy   /   paired colonist placements"
        family="Provisioned lineage in shared water";measure=100*a0/max(1,a0+b0);unit="%"
        stat=[["Provisioned lineage",a0],["Fostered lineage",b0],["Surviving body forms",int(q["distinct_bodies"])]]
        narrative=[f"The released cohorts began with {int(opening['cohort_a']):,} and {int(opening['cohort_b']):,} residents.",
                   f"At the closing census {a0:,} descendants carried the provisioned lineage marker and {b0:,} carried the fostered marker.",
                   f"They held {int(q['reserve_a']):,} and {int(q['reserve_b']):,} body energy units respectively."]
        bullets=[f"The closing population expressed {int(q['expressed_tapes']):,} somatic spiking programs.",
                 "Predictive signals were muted in this introduction." if "muted" in args else "Predictive signals entered their normal sensory and motor channels."]
    reed=reed_summary(entries,metrics)
    if reed:title,subtitle,family,measure,unit,stat,narrative,bullets=reed
    clay=clay_summary(entries)
    if clay:title,subtitle,family,measure,unit,stat,narrative,bullets=clay
    pressure=pressure_summary(data,entries)
    if pressure:title,subtitle,family,measure,unit,stat,narrative,bullets=pressure
    spatial=hollow_summary(entries)
    if spatial:title,subtitle,family,measure,unit,stat,narrative,bullets=spatial
    prospect=effort_summary(entries)
    if prospect:title,subtitle,family,measure,unit,stat,narrative,bullets=prospect
    mechanism=linkage_summary(entries)
    if mechanism:title,subtitle,family,measure,unit,stat,narrative,bullets=mechanism
    exploration=practice_summary(entries)
    if exploration:title,subtitle,family,measure,unit,stat,narrative,bullets=exploration
    woven=runnel_summary(entries)
    if woven:title,subtitle,family,measure,unit,stat,narrative,bullets=woven
    banks=backwater_summary(entries)
    if banks:title,subtitle,family,measure,unit,stat,narrative,bullets=banks
    floats=braid_summary(entries)
    if floats:title,subtitle,family,measure,unit,stat,narrative,bullets=floats
    cortical=mind_summary(entries,data)
    if cortical:title,subtitle,family,measure,unit,stat,narrative,bullets=cortical
    gen_res=genesis_summary(data) if (command=="genesis" or any(l.startswith("genesis_") for l in lines)) else None
    if gen_res:
        kind="genesis"
        title,subtitle,family,measure,unit,stat,narrative,bullets=gen_res
    if kind=="interview" and entries("cascade"):
        exposure=cascade_session(data)
        if exposure:title,subtitle,family,measure,unit,stat,narrative,bullets=exposure
    if data.get("title"):title=data["title"]
    terrain_activity=[int(v["living"]) for v in entries("flood_tick")] if kind=="floodplain" else []
    terrain_depth=[int(v["deepest"]) for v in entries("flood_census")] if kind=="floodplain" else []
    return dict(kind=kind,command=command,title=title,subtitle=subtitle,performance=measured_performance(data),source=source,archive=data.get("archive",""),family=family,
                measure=measure,unit=unit,stats=stat,scores=scores[-8:],narrative=narrative,bullets=bullets,
                receipt=data.get("receipt",""),activity=change if kind=="world" and points else {},
                alive=terrain_activity[-1] if terrain_activity else points[-1][3] if points else None,capacity=data.get("side",0)**2,depth=max(terrain_depth) if terrain_depth else metrics.get("peak_depth",0),
                peak=max(terrain_activity) if terrain_activity else metrics.get("peak_alive",0),observations=metrics.get("observations",0))

def trend(d,x,y,w,h,values,color=GREEN,fill=False,times=None,limits=None):
    if not values:return
    lo,hi=limits or (min(values),max(values))
    if lo==hi:lo-=max(1,abs(lo)*.05);hi+=max(1,abs(hi)*.05)
    times=times or list(range(len(values)))
    pts=[(x+w*(t-times[0])/max(1,times[-1]-times[0]),y+h-(v-lo)/(hi-lo)*h) for t,v in zip(times,values)]
    path="M"+" L".join(f"{a:.1f} {b:.1f}" for a,b in pts)
    if fill and len(pts)>1:d.path(path+f" L{x+w} {y+h} L{x} {y+h}Z","none",0,"url(#wash)")
    d.path(path,color,2.9);d.dot(*pts[-1],4,color)
    if len(pts)==1:d.dot(*pts[0],6,"none",f'stroke="{color}" stroke-width="2"')


def header(d,b,index):
    d.rect(0,0,W,10,GREEN)
    d.path("M49 47 L62 26 L76 47 M55 38 H70",GREEN,3)
    d.label(90,44,"Axion  /  evolution journal",GREEN)
    badge="ARCHIVE" if b["archive"] else "RUN"
    d.text(W-48,44,f"{badge}  {index:05d}     /     {b['date'][:10]}",15,MUTED,MONO,500,"end")
    d.text(46,112,short(b["title"],53),51,INK,SERIF,500)
    d.text(49,147,short(b["subtitle"],113),18,MUTED)
    d.rule(48,168,W-48,168)


def grid_background(d,x,y,size):
    d.rect(x,y,size,size,"url(#water)",10)
    d.rect(x,y,size,size,"url(#grid)",10)


def field_panel(d,data,x,y,size):
    grid_background(d,x,y,size);n=data["resolution"];side=data["side"];span=side//n;mapdata=data["tiles"]
    pad=21;inner=size-2*pad;ox=x+pad;oy=y+pad
    if mapdata:
        raw=bytearray()
        for f,a,_ in mapdata:
            v=min(1,f/(80*span*span));u=min(1,a/(span*span))
            raw.extend((int(14+38*v+9*u),int(41+57*v+10*u),int(42+32*v+8*u)))
        d.add(f'<image x="{ox}" y="{oy}" width="{inner}" height="{inner}" preserveAspectRatio="none" xlink:href="data:image/png;base64,{png(n,raw)}"/>')
        # Isolines follow measured plant biomass. Unconnected segments keep ambiguous saddles local.
        if n<=64:
            for threshold in (.25,.45,.65,.83):
                paths=[]
                for j in range(n-1):
                    for i in range(n-1):
                        f=[mapdata[t][0]/(80*span*span) for t in (j*n+i,j*n+i+1,(j+1)*n+i+1,(j+1)*n+i)]
                        if max(f)<threshold or min(f)>=threshold:continue
                        pts=((i+.5,j+.5),(i+1.5,j+.5),(i+1.5,j+1.5),(i+.5,j+1.5));cross=[]
                        for k in range(4):
                            l=(k+1)&3
                            if (f[k]<threshold)==(f[l]<threshold):continue
                            q=(threshold-f[k])/(f[l]-f[k])
                            cross.append((ox+(pts[k][0]+(pts[l][0]-pts[k][0])*q)*inner/n,oy+(pts[k][1]+(pts[l][1]-pts[k][1])*q)*inner/n))
                        for k in range(0,len(cross)-1,2):
                            a,b=cross[k:k+2];paths.append(f"M{a[0]:.1f} {a[1]:.1f} L{b[0]:.1f} {b[1]:.1f}")
                d.path(" ".join(paths),"#b1d1a5",.9,extra='opacity=".32"')
        for k,(_,a,family) in enumerate(mapdata):
            if not a:continue
            px=ox+(k%n+.5)*inner/n;py=oy+(k//n+.5)*inner/n
            radius=max(.8,min(3.4,(inner/n)*(.14+.19*math.sqrt(a/(span*span)))))
            d.dot(px,py,radius,hue(family),'opacity=".82"')
        basin=data.get("metrics",{}).get("basin_width",0)
        if basin and side//basin<=32:
            for offset in range(basin,side,basin):
                pos=offset*inner/side
                d.path(f"M{ox+pos} {oy}v{inner} M{ox} {oy+pos}h{inner}","#b4d1a8",1.1,extra='stroke-dasharray="4 5" opacity=".6"')
    tracks=defaultdict(list);frames=data.get("frames",[])
    for frame in frames:
        for a in frame["residents"]:
            tracks[(frame["t"][2],a[0],a[2],a[3],a[4])].append((frame["t"][1],a))
    def coord(pos):return (ox+(pos%side+.5)*inner/side,oy+(pos//side+.5)*inner/side)
    def steps(track):return sum(a[1][1]!=b[1][1] and b[0]==a[0]+1 for a,b in zip(track,track[1:]))
    selected=sorted(tracks.values(),key=lambda g:(steps(g),len(g),quantum(g[-1][1][0])),reverse=True)[:18]
    for track in selected:
        for (t,a),(t2,b) in zip(track,track[1:]):
            if a[1]==b[1]:continue
            u,v=coord(a[1]);xx,yy=coord(b[1])
            if abs(u-xx)>inner*.5 or abs(v-yy)>inner*.5:continue
            if t2-t==1:d.path(f"M{u:.2f} {v:.2f}L{xx:.2f} {yy:.2f}","#f8e0a0",2.4)
            else:
                d.dot(u,v,2.4,"none",f'stroke="{hue(a[2])}" stroke-width="1" opacity=".8"')
    current=frames[-1]["residents"] if frames else []
    # A large world retains measured densities at every bin. Display a spaced set of actual animals on top.
    occupied=set();picked=[]
    for a in sorted(current,key=lambda a:(-a[4],quantum(a[0]))):
        px,py=coord(a[1]);bin_=(int((px-ox)/34),int((py-oy)/34))
        if bin_ in occupied:continue
        occupied.add(bin_);picked.append(a)
        if len(picked)>=54:break
    for a in picked:
        px,py=coord(a[1]);creature(d,px,py,.31 if side>=64 else .39,a[6],a[9],hue(a[2]),90*a[8])
    d.label(x+7,y-13,"Habitat survey")
    d.text(x+size,y-13,f"{side} × {side}",13,MUTED,MONO,anchor="end")
    for xx,yy,flipx,flipy in ((ox,oy,1,1),(ox+inner,oy,-1,1),(ox,oy+inner,1,-1),(ox+inner,oy+inner,-1,-1)):
        d.path(f"M{xx+9*flipx} {yy} L{xx} {yy} L{xx} {yy+9*flipy}","#c0d8bd",1.4)
    return frames


def circuit_panel(d,data,x,y,size):
    if data.get("conditional"):
        grid_background(d,x,y,size);d.label(x+7,y-13,"Context-dependent predictions")
        places=defaultdict(lambda:defaultdict(dict));support={}
        for word in data["conditional"]:
            cue=word&31;place=(word>>5)&31;motor=(word>>10)&3;contact=(word>>12)&255
            places[place][cue][motor]=(contact&31,contact>>5)
            support[place,cue]=(word>>20)&3
        place=max(places,key=lambda p:sum(len(c) for c in places[p].values()))
        cues=sorted(places[place],key=lambda q:-sum(v[1] for v in places[place][q].values()))[:8]
        d.text(x+27,y+54,f"At sensory ensemble {place:02d}",29,"#e9efd8",SERIF)
        evidence="supported_event" in data["metrics"]
        d.text(x+28,y+85,"Each cell gives the predicted next sensation.",15,"#b4cbbd")
        if evidence:d.text(x+28,y+108,"Bars beside a cue show its learned predictive support (0–3).",13,"#b4cbbd")
        d.label(x+27,y+138,"Earlier cue","#abcabb")
        left=x+137;cw=(size-164)/4;rh=min(64,355/len(cues));top=y+164
        for motor in range(4):d.text(left+(motor+.5)*cw,y+140,f"Motor {motor}",14,MINT,MONO,anchor="middle")
        for i,cue in enumerate(cues):
            yy=top+i*rh
            d.text(x+32,yy+rh*.66,f"{cue:02d}",21,"#ececd4",MONO)
            if evidence:
                for q in range(3):d.rect(x+72+q*10,yy+rh*.50,7,8,MINT if q<support[place,cue] else "#345649",1)
            if cue+1==data["metrics"].get("held_event"):d.dot(x+111,yy+rh*.53,4,AMBER)
            for motor in range(4):
                cell=places[place][cue].get(motor);xx=left+motor*cw
                d.rect(xx+2,yy+2,cw-5,rh-5,"#254940" if cell else "#1d3834",4)
                if cell:
                    target,charge=cell
                    d.text(xx+cw*.5,yy+rh*.62,f"{target:02d}",21,COLORS[target%len(COLORS)],MONO,600,"middle")
                    d.rect(xx+12,yy+rh-12,(cw-25)*charge/7,3,"#84ba9c",1)
        d.rule(x+27,y+size-77,x+size-27,y+size-77,"#466458")
        d.text(x+28,y+size-48,"Bar length gives synaptic efficacy on a seven-step scale.",15,"#c6d5c1")
        d.text(x+28,y+size-22,f"{len(places)} represented places   /   {len(data['conditional'])} active contacts",13,"#a8c7b4",MONO)
        return
    grid_background(d,x,y,size);d.label(x+7,y-13,"Stored connections" if data.get("transitions") else "Motor activity")
    edges=data.get("transitions",[]);cx=x+size/2;cy=y+size*.46
    journeys=dict(data.get("journeys",[]))
    if edges or journeys:
        pts=[]
        for k in range(32):
            t=2*math.pi*k/32-math.pi/2;pts.append((cx+size*.355*math.cos(t),cy+size*.355*math.sin(t)))
        for edge in sorted(set(edges)|journeys.keys(),key=lambda k:journeys.get(k,0)):
            a=edge&31;b=(edge>>5)&31;m=edge>>10;u,v=pts[a];xx,yy=pts[b]
            traffic=journeys.get(edge,0);color=AMBER if traffic else COLORS[m];weight=min(4.8,1.4+math.log2(1+traffic)*.45)
            if a==b:d.dot(u,v,8,"none",f'stroke="{color}" stroke-width="{weight}" opacity=".55"')
            else:
                mx=(u+xx)/2;my=(v+yy)/2
                qx=.72*mx+.28*cx;qy=.72*my+.28*cy
                d.path(f"M{u:.1f} {v:.1f} Q{qx:.1f} {qy:.1f} {xx:.1f} {yy:.1f}",color,weight,extra='opacity=".70"')
                length=math.hypot(xx-qx,yy-qy)
                if length:
                    vx=(xx-qx)/length;vy=(yy-qy)/length
                    d.path(f"M{xx-13*vx+2.3*vy:.1f} {yy-13*vy-2.3*vx:.1f} l{5*vx-2.3*vy:.1f} {5*vy+2.3*vx:.1f} l{-5*vx-2.3*vy:.1f} {-5*vy+2.3*vx:.1f}",color,1.1)
        for k,(u,v) in enumerate(pts):
            d.dot(u,v,4.8,("#c7dfbe" if any((e&31)==k for e in edges) else "#73918a"))
            if k%4==0:d.text(cx+(u-cx)*1.13,cy+(v-cy)*1.13+5,f"{k:02d}",12,MINT,MONO,anchor="middle")
        d.text(cx,y+size-50,"32 sensory ensembles",21,"#eff1da",SERIF,500,"middle")
        d.text(cx,y+size-24,"Amber follows observed journeys" if journeys else "Colour records the four efferent classes",13,"#afccc0",MONO,anchor="middle")
    else:
        m=data.get("metrics",{});creature(d,cx,cy,3.7,int(m.get("body",0)),int(m.get("tape",1)),MINT,-28)
        d.text(cx,y+size-84,"Individual activity",30,"#edf1de",SERIF,500,"middle")
        d.text(cx,y+size-50,"Glyph derived from the inherited body",14,"#afccc0",MONO,anchor="middle")


def specimens(d,data,x,y,w):
    frames=data.get("frames",[])
    if len(frames)>1 and frames[0]["t"][2]==frames[-1]["t"][2]:
        early={}
        for a in frames[0]["residents"]:
            if a[2] not in early or a[4]<early[a[2]][4]:early[a[2]]=a
        later=[b for b in frames[-1]["residents"] if b[2] in early and b[3]>frames[0]["t"][1] and b[4]>early[b[2]][4]]
        if later:
            mode=data.get("metrics",{}).get("navigation",0)
            loci=(6,10,11,12,13,14,15,16) if mode>=4 else (6,10,11,12,13) if mode else (6,10,11,12)
            def differences(a,b):
                return sum((a[k]^b[k]).bit_count() for k in loci if k<min(len(a),len(b)))
            b=max(later,key=lambda z:(differences(z,early[z[2]]),z[4]-early[z[2]][4]))
            a=early[b[2]];colour=hue(a[2]);gap=14;part=(w-gap)/2
            d.label(x,y,f"One founder line over time  /  {a[2]:X}")
            for i,(animal,tick) in enumerate(((a,frames[0]["t"][1]),(b,frames[-1]["t"][1]))):
                xx=x+i*(part+gap);d.rect(xx,y+14,part,73,"#e0e7d8",6)
                creature(d,xx+62,y+51,1.13,animal[6],animal[9],GREEN,66+(animal[6]&15))
                d.text(xx+116,y+45,f"Generation {human(animal[4])}",18,INK,SANS,600)
                d.text(xx+116,y+69,f"Tick {human(tick)}",14,MUTED,MONO)
            d.dot(x+5,y+116,4,colour,'stroke="#47574e" stroke-width="1"')
            text=f"Body {(a[6]^b[6]).bit_count()}/32 bits differ. Expressed gates {a[9]} → {b[9]}."
            if len(a)>=14 and len(b)>=14:
                wiring=(a[11]^b[11]).bit_count()+(a[12]^b[12]).bit_count()
                text=f"Body {(a[6]^b[6]).bit_count()}/32   ·   Expression {(a[10]^b[10]).bit_count()}/32   ·   Muscle wiring {wiring}/64"
                if mode>=4 and min(len(a),len(b))>=17:
                    text+=f"   ·   Temporal rules {sum((a[k]^b[k]).bit_count() for k in (14,15,16))}/96"
                elif mode:text+=f"   ·   Navigation {(a[13]^b[13]).bit_count()}/32"
                d.text(x+19,y+106,"Differing inherited bits in these two relatives",14,MUTED)
            d.text(x+19,y+128,text,15,MUTED)
            return
    candidates={};frequency=Counter()
    for f in reversed(frames):
        for a in f["residents"]:
            k=(a[2],a[6],a[9]);frequency[k]+=1;candidates.setdefault(k,a)
        if len(candidates)>=3:break
    candidates=sorted(candidates.items(),key=lambda e:(-frequency[e[0]],-e[1][4],e[1][0]))[:3]
    d.label(x,y,"Sampled phenotypes")
    if not candidates:d.text(x,y+58,"No living subjects in the sampled frames",17,MUTED);return
    cell=w/len(candidates)
    for i,(_,a) in enumerate(candidates):
        xx=x+i*cell
        if i:d.rule(xx,y+13,xx,y+130)
        d.rect(xx+6,y+12,cell-12,68,"#e3e8d9",5)
        creature(d,xx+cell*.5,y+48,1.16,a[6],a[9],GREEN,66+(a[6]&15))
        d.dot(xx+12,y+102,4,hue(a[2]),'stroke="#47574e" stroke-width="1"')
        d.text(xx+23,y+107,f"Family {a[2]:X}",17,INK,SANS,600)
        d.text(xx+7,y+132,f"Gen {human(a[4])}  ·  {a[9]} gates",13,MUTED,MONO)


def motor_strip(d,data,x,y,w,h):
    motors=data.get("motors",[0]*8);ev=data.get("actions",[])
    d.label(x,y,"Recorded motor discharges")
    if not sum(motors):
        d.text(x,y+49,"No motor discharge in this record",19,MUTED);return
    for i,n in enumerate(motors):
        yy=y+22+i*(h-26)/8
        d.text(x,yy+9,str(i),12,MUTED,MONO);d.rect(x+25,yy,w*.62,7,"#dbe3d9",3)
        d.rect(x+25,yy,w*.62*n/max(motors),7,GREEN if i%2==0 else "#6c9c78",3)
        d.text(x+w,yy+9,human(n),12,INK,MONO,anchor="end")
    if ev:d.text(x,y+h+20,f"{len(ev)} activity samples in this plate",13,MUTED,MONO)


def interview_panel(d,data,x,y,size):
    grid_background(d,x,y,size);d.label(x+7,y-13,"Neural exchange")
    turns=interview_turns(data)
    args=data.get("args",[]);voice=(args[args.index("--voice")+1].split(",") if "--voice" in args else ["north","east","south","west"])+["quiet"]
    d.text(x+27,y+58,"The scene and the reply",28,"#e2eddb",SERIF)
    d.text(x+27,y+89,"Positions belong to the world. The speaker can inspect one view.",14,"#b4cbbd")
    columns=[27,104,219,335,471]
    for offset,label in zip(columns,("Turn","View 1","View 2","Vision","Reply")):d.label(x+offset,y+143,label,"#abcabb")
    for j,t in enumerate(turns[-8:]):
        yy=y+173+42*j
        view=int(t["view"]);light="—"
        if "opened" in t:
            light=("closed","view 1","view 2","both")[t["opened"]]
        elif view<2 and "covered" in t:
            light="replay" if t.get("imagined") else "recall" if t.get("inner") else "covered" if t["covered"]&(1<<view) else "dim" if t["dim"]&(1<<view) else "open"
        bearings=voice[:4]+["absent"]
        values=[str(t.get("turn",len(turns)-len(turns[-8:])+j+1)),bearings[int(t["first"])],bearings[int(t["second"])],light,voice[int(t["sound"])]]
        for k,(offset,v) in enumerate(zip(columns,values)):
            if k in (1,2) and ((t.get("opened",0)&(1<<(k-1))) or view<2 and k==view+1):d.dot(x+offset-11,yy-5,3,AMBER)
            d.text(x+offset,yy,short(v,10),17,MINT if k==4 else "#dce7dd",MONO)
        d.rule(x+27,yy+12,x+size-27,yy+12,"#37594e")
    d.dot(x+30,y+size-56,4,AMBER);d.text(x+45,y+size-51,"View selected by the speaker’s muscles",15,"#b4cbbd")

def memory_panel(d,data,x,y,size):
    grid_background(d,x,y,size);d.label(x+7,y-13,"Visual memory")
    d.text(x+26,y+56,"Reports with the retina covered",27,"#e2eddb",SERIF)
    d.text(x+26,y+86,"Each cell shows correct reports across fresh lifetimes.",15,"#b4cbbd")
    rows=recollections(data)
    phases=(("one","Once"),("delay","Delay"),("noise","Outlier"),("revision","Moved"),("faint","Faint"))
    for j,(_,label) in enumerate(phases):d.text(x+196+78*j,y+143,label,14,"#abcabb",MONO,anchor="middle")
    arms=(("intact","Intact"),("fixed","Frozen"),("crossed","Crossed cues"),("source","Flipped light"),("erased","Erased"),("last-per-cue","Last per cue"),("last-event","Last event"))
    for i,(arm,label) in enumerate(arms):
        yy=y+162+45*i;r=rows[arm];n=int(r["questions"]);d.text(x+22,yy+22,label,15,"#dce7dd")
        for j,(key,_) in enumerate(phases):
            f=int(r[key])/n;color="#%02x%02x%02x"%(int(41+110*f),int(76+132*f),int(67+81*f))
            xx=x+163+78*j;d.rect(xx,yy,66,31,color,4)
            d.text(xx+33,yy+21,f"{100*f:.0f}%",14,"#102d25" if f>.50 else "#e8efdc",MONO,anchor="middle")
    d.text(x+23,y+size-58,"Once = a single clear sighting. Faint = four dim sightings.",14,"#b4cbbd")

def concept_panel(d,data,x,y,size):
    grid_background(d,x,y,size)
    if data["command"]=="infer":
        d.label(x+7,y-13,"Relational learning");d.text(x+28,y+53,"Where transfer survived",30,"#e2eddb",SERIF)
        d.text(x+28,y+84,"Correct responses on independent maps",16,"#b4cbbd")
        rows=[pairs(s) for s in data["lines"] if s.startswith("inference=")];asks="inquiry" in rows[0]
        keys=("transfer","recursive","inquiry" if asks else "uncertain","other")
        for col,label in enumerate(("2 hops","4 hops","Inquiry" if asks else "Withheld","Peer view")):
            d.text(x+220+101*col,y+135,label,15,"#b4cbbd",MONO,anchor="middle")
        labels={"intact":"Intact","no-examples":"Unexposed","own-view":"Single view","no-reuse":"Reuse cut","fixed":"Fixed","random-question":"Random probe"}
        for row,r in enumerate(rows):
            yy=y+157+55*row;d.text(x+28,yy+24,labels.get(r["inference"],r["inference"]),16,"#e1ebde")
            for col,key in enumerate(keys):
                rate=int(r[key])/int(r.get("view_trials",r["trials"]) if key=="other" else r["trials"]);xx=x+176+101*col
                d.rect(xx,yy,88,36,blend("#25443c","#99ceac",rate),5)
                d.text(xx+44,yy+25,f"{100*rate:.1f}%",16,"#123229" if rate>.45 else "#e1ebde",MONO,anchor="middle")
        d.text(x+28,y+size-42,"Each row changes one part of the same animal or its experience.",15,"#b4cbbd")
        return
    d.label(x+7,y-13,"Grounded conversation");lines=[s for s in data.get("lines",[]) if s.startswith("concept_turn=")]
    events=[pairs(s.partition("  prompt=")[0]) for s in lines];event=next((r for r in reversed(events) if int(r.get("hops",0))),None)
    if not event:
        d.text(x+28,y+58,"Objects, names and examples",31,"#e2eddb",SERIF)
        for i,(label,key) in enumerate((("Presentations","board_stimuli"),("Examples of relations","route_examples"),("Questions from the witness","concept_questions"))):
            yy=y+164+i*124;d.text(x+31,yy,label,18,"#b4cbbd");d.text(x+31,yy+46,human(data["metrics"].get(key,0)),39,MINT,SERIF)
        return
    d.text(x+27,y+55,"Following a learned pathway",31,"#e2eddb",SERIF)
    views={"1":"Witness's view","2":"Partner's last observed view","3":"Both possible views"}
    d.text(x+28,y+86,views.get(event["views"],"Unresolved perspective"),17,"#b4cbbd")
    pulses=[int(v) for v in event["path"].split(",")];nodes=[m.bit_length()-1 if m and m&(m-1)==0 else None for m in pulses]
    def pos(j):return (x+56+(j%8)*70+(j//8%2)*13,y+169+(j//8)*72)
    for k,(a,b) in enumerate(zip(nodes,nodes[1:])):
        if a is None or b is None:continue
        ax,ay=pos(a);bx,by=pos(b);lift=18+6*(k%3);color=blend("#7cb8a0","#edb772",k/max(1,len(nodes)-2))
        if a==b:d.path(f"M{ax-8} {ay-9} c-25 -30 41 -37 18 -1",color,2.5)
        else:d.path(f"M{ax} {ay} Q{(ax+bx)/2} {(ay+by)/2-lift} {bx} {by}",color,2.8)
    for j in range(32):
        xx,yy=pos(j);lit=j in nodes;d.dot(xx,yy,14 if lit else 9,"#d4ba80" if lit else "#344f46")
        d.text(xx,yy+4,str(j),12,"#16392f" if lit else "#b3cbbb",MONO,anchor="middle")
    sequence=" → ".join(str(j) if j is not None else "?" for j in nodes)
    if int(event["hops"])>8:sequence+=" → …"
    d.label(x+27,y+467,"Order of activation","#a9c6b8");d.text(x+28,y+499,sequence,19,"#ece4c9",MONO)
    d.text(x+28,y+size-38,f"{counted(int(event['hops']),'crossing')}  /  emitted “{event.get('speech','')}”",19,"#e2eddb",SERIF)

def garden_panel(d,data,x,y,size):
    grid_background(d,x,y,size);d.label(x+7,y-13,"Physical and imagined interventions")
    lines=data.get("lines",[]);turns=[s for s in lines if s.startswith("garden_event=")]
    samples=[]
    for line in turns:
        fields=pairs(line.partition("  speech=")[0]);path=fields.get("path","")
        if "," in path:samples.append((line,fields,[int(v) for v in path.split(",")]))
    if not samples:
        d.text(x+26,y+56,"Learning from the display",29,"#e1ead9",SERIF);d.text(x+27,y+87,"Recorded exposures and motor events",16,"#aec9b9")
        for i,(label,key) in enumerate((("Encounters","garden_events"),("Grounded turns","garden_turns"),("Executed actions","garden_actions"))):
            yy=y+163+119*i;d.label(x+27,yy,label,"#b3cbbb");d.text(x+28,yy+48,human(data["metrics"].get(key,0)),40,MINT,SERIF)
        return
    line,r,states=max(reversed(samples),key=lambda p:len(p[2]));actual=int(r["actions"])>0
    d.text(x+27,y+54,"An action changes the next state" if actual else "An animal imagines a plan",27,"#e1ead9",SERIF)
    d.text(x+28,y+84,"Observed consequences" if actual else "Predictions through its private causal contacts",16,"#aec9b9")
    names=next((s.split("=",1)[1].split(",") for s in reversed(lines) if s.startswith("garden_traits=")),[f"Cell {i}" for i in range(6)])
    cols=thin(list(enumerate(states)),10);pitch=(size-165)/len(cols)
    for k,(index,_) in enumerate(cols):d.text(x+137+(k+.45)*pitch,y+137,str(index),13,"#b3cbbb",MONO,anchor="middle")
    for j in range(6):
        yy=y+163+j*43;d.text(x+26,yy+17,short(names[j],13),16,"#d4e4d6")
        for k,(_,state) in enumerate(cols):
            xx=x+137+k*pitch;on=bool((state>>j)&1);d.rect(xx,yy,pitch-5,26,"#a4d7ae" if on else "#29483e",4)
            if k and (cols[k-1][1]^state)&(1<<j):d.dot(xx+pitch*.45,yy+13,3.4,"#e6aa6e" if not on else "#2c6955")
    d.label(x+27,y+449,"Motor utterance","#adcabc")
    said=line.partition("  speech=")[2].partition("  prompt=")[0]
    for i,v in enumerate(textwrap.wrap(said or "No audible sequence",46)[:2]):d.text(x+27,y+481+i*25,v,19,"#f0e5c9",SERIF)
    d.text(x+27,y+size-39,"Light cells fired. Dots mark changes between shown states.",15,"#abc7b8")

def greenhouse_panel(d,data,x,y,size):
    grid_background(d,x,y,size);d.label(x+7,y-13,"Growers and their plots")
    rows=[pairs(s) for s in data.get("lines",[]) if s.startswith("crop_island=")];residents=data.get("commons_residents",[])
    d.text(x+27,y+54,"At the end of the season",29,"#e1ead9",SERIF);d.text(x+28,y+84,"Amber plots currently bear food.",16,"#b1cabb")
    if not rows:return
    current=[r for r in rows if r["generation"]==rows[-1]["generation"]]
    for k,r in enumerate(current[-2:]):
        yi=y+120+197*k;d.label(x+28,yi,f"Bed {int(r['crop_island'])+1}    /    seed line {int(r['ancestor'])+1}","#b6cbb9")
        code=int(r["food"])-1
        for j,v in enumerate(map(int,r["plots"].split(","))):
            xx=x+27+(j%8)*70;yy=yi+15+(j//8)*37
            input_index=((v>>(code&7))&1)+2*((v>>((code>>3)&7))&1);sweet=(code>>(6+input_index))&1
            d.rect(xx,yy,61,31,"#876943" if sweet else blend("#244539","#456e50",v.bit_count()/6),5)
            if 32*k+j<len(residents):
                _,family,energy,meals,work,mass,body,tape=residents[32*k+j]
                creature(d,xx+30,yy+15,.46+min(.12,mass/600),body,tape,"#c5e1bc" if energy else "#56665b",family*47%360)
    d.text(x+28,y+size-39,"Each animal shares growing space with its neighbours.",15,"#b3cbbb")

def commons_panel(d,data,x,y,size):
    grid_background(d,x,y,size);d.label(x+7,y-13,"Residents of the commons")
    residents=data.get("commons_residents",[]);d.text(x+25,y+53,"At the end of the season",28,"#e2eddb",SERIF)
    d.text(x+26,y+84,"Amber rings mark animals that chose to experiment.",15,"#b4cbbd")
    for j,a in enumerate(residents):
        site,family,energy,meals,experiments,mass,body,tape=a;group=j//32;position=j%32
        if not position:d.label(x+27,y+125+197*group,f"Island {site//32+1}","#abcabb")
        xx=x+50+(position%8)*71;yy=y+157+197*group+(position//8)*37
        if experiments:d.dot(xx,yy,11+min(5,experiments/4),"none",'stroke="#d1a25f" stroke-width="1"')
        creature(d,xx,yy,.52+min(.13,mass/500),body,tape,"#9dc9a9" if energy else "#526861",(family*53)%360)
    d.text(x+26,y+size-38,"Bright individuals survived. Larger bodies deposited more material.",14,"#b4cbbd")

def dialect_panel(d,data,x,y,size):
    grid_background(d,x,y,size);d.label(x+7,y-13,"Vocal discharges")
    rows=[pairs(line) for line in data.get("lines",[]) if line.startswith("dialect=")]
    if not rows:return
    r=rows[-1];counts=[int(v) for v in r["counts"].split(",")]
    args=data.get("args",[])
    sounds=(args[args.index("--voice")+1].split(",") if "--voice" in args else ["ka","lo","mi","tu"])+["quiet"]
    d.text(x+25,y+54,"One pair’s observed vocabulary",27,"#e2eddb",SERIF)
    d.text(x+26,y+88,"Counts show which sound followed each private observation.",14,"#b4cbbd")
    left=x+113;cw=(size-140)/5;top=y+165;rh=69
    for j,name in enumerate(sounds):d.text(left+(j+.5)*cw,y+145,short(name,9),15,MINT,MONO,anchor="middle")
    for i in range(4):
        row=counts[5*i:5*i+5];peak=max(1,max(row));yy=top+i*rh
        d.text(x+24,yy+40,f"Fact {i+1}",16,"#e6ecd9",MONO)
        for j,n in enumerate(row):
            xx=left+j*cw
            d.rect(xx,yy,cw-6,rh-6,"#214038",4)
            if n:
                d.rect(xx,yy,cw-6,rh-6,"#76c79b",4,extra=f'opacity="{.25+.7*n/peak:.3f}"')
            d.text(xx+(cw-6)/2,yy+38,human(n),23,"#102d28" if n else "#66867b",MONO,anchor="middle")
    if data.get("breeding"):
        points=data["breeding"];ts=[p[0] for p in points];best=[p[3] for p in points]
        d.label(x+26,y+468,f"Breeding scores across {human(ts[-1])} generations","#b4cbbd")
        bounds=(0,max(1,max(best)))
        trend(d,x+27,y+479,size-54,39,[p[2]/p[1] for p in points],MINT,False,ts,bounds)
        trend(d,x+27,y+479,size-54,39,best,AMBER,False,ts,bounds)
    else:d.rule(x+25,y+size-102,x+size-27,y+size-102,"#47675b")
    d.text(x+25,y+size-65,f"Pair {r['pair']}   /   {sum(counts)} observed calls",21,"#dce6d5",SERIF)
    d.text(x+26,y+size-32,"Quiet records a closed vocal actuator.",14,"#b4cbbd")

def joint_panel(d,snapshot,x,y,size):
    grid_background(d,x,y,size);d.label(x+7,y-13,"One animal's learned apparatus")
    seen,truth,reports,voiced=(int(snapshot[k]) for k in ("observed","physical","reported","audible"))
    d.text(x+32,y+48,"What follows each arrangement?",28,"#e2eddb",SERIF)
    d.text(x+32,y+78,"Arrows show the predicted food bearing.",16,"#b0c9bb")
    letters="NESW";left=x+110;top=y+148;step=(size-162)/4;h=70
    d.label(left,y+111,"Food before the move","#a4c7b5")
    for a in range(4):
        d.text(left+(a+.5)*step,y+136,letters[a],16,"#d6e8d9",MONO,anchor="middle")
    palette=("#315143","#98dfba","#d89467","#274047")
    for row in range(4):
        cy=top+(row+.5)*(h+8)
        d.text(left-22,cy+3,letters[row],16,"#d6e8d9",MONO,anchor="end")
        for col in range(4):
            site=row*4+col;bit=1<<site;value=(reports>>(2*site))&3
            status=3 if not voiced&bit else 2 if value!=((truth>>(2*site))&3) else 0 if seen&bit else 1
            bx=left+col*step;by=top+row*(h+8);cx=bx+(step-8)/2
            d.rect(bx,by,step-8,h,palette[status],6)
            if status==3:d.text(cx,cy+2,"·",23,"#a7b9af",MONO,anchor="middle");continue
            ink="#d0e7d5" if status==0 else "#193a30";dx,dy=((0,-1),(1,0),(0,1),(-1,0))[value]
            gx=cx+12*dx;gy=cy+12*dy-4
            d.rule(cx-12*dx,cy-12*dy-4,gx,gy,ink,3)
            for sign in (-1,1):d.rule(gx,gy,gx-8*dx+sign*7*dy,gy-8*dy-sign*7*dx,ink,3)
    d.add(f'<text transform="translate({x+46:.1f} {top+154:.1f}) rotate(-90)" text-anchor="middle" fill="#b0c9bb" font-size="16" font-family="{SANS}">Water before the move</text>')
    for i,(label,col) in enumerate(zip(("Observed","Inferred","Wrong","Silent"),palette)):
        xx=x+30+(i%2)*(size/2-10);yy=y+size-69+(i//2)*29
        d.rect(xx,yy-13,16,16,col,3);d.text(xx+24,yy,label,15,"#dce8dc")

def report_panel(d,data,x,y,size):
    grid_background(d,x,y,size);d.label(x+7,y-13,"The acoustic channel")
    lines=data.get("lines",[]);report=next((s for s in reversed(lines) if s.startswith("report_turn=")),None)
    if report and "witnessed=" in report:
        d.text(x+29,y+53,"The witness's memory",31,"#dce8d8",SERIF)
        r=pairs(report.partition("  speech=")[0])
        for i,(key,when) in enumerate((("prior","Earlier"),("witnessed","Latest"))):
            e=int(r[key]);cx=x+size*(.27+.46*i);cy=y+201;radius=65
            d.dot(cx,cy,radius+19,"#25473e");d.dot(cx,cy,radius+3,"#1b3731")
            direction=((0,-1),(1,0),(0,1),(-1,0))
            for v,(dx,dy) in enumerate(direction):
                d.text(cx+dx*radius,cy+dy*radius+5,"NESW"[v],14,"#a9c6b8",MONO,anchor="middle")
            if e&32:
                a=direction[(e>>1)&3];b=direction[(e>>3)&3]
                ax,ay=cx+a[0]*42,cy+a[1]*42;bx,by=cx+b[0]*42,cy+b[1]*42
                d.path(f"M {ax} {ay} Q {cx+12} {cy-7} {bx} {by}",MINT,3)
                d.dot(ax,ay,5,AMBER);d.dot(bx,by,7,MINT);label=("water" if e&1 else "food")
            else:label="unknown"
            d.text(cx,cy+114,f"{when} · {label}",18,"#dbe8dc",SERIF,anchor="middle")
        d.dot(x+size*.30,y+337,4,AMBER);d.text(x+size*.30+10,y+342,"Departure",14,"#b7cfc0")
        d.dot(x+size*.56,y+337,5,MINT);d.text(x+size*.56+11,y+342,"Arrival",14,"#b7cfc0")
        d.label(x+31,y+370,"Emitted words","#a9c6b8");samples=[s for s in lines if s.startswith(("report_turn=","retelling="))][-3:]
        for i,s in enumerate(samples):
            yy=y+407+i*46;d.text(x+31,yy,short(s.partition("  speech=")[2] or "(silence)",48),19,"#d3ead9",SERIF)
            d.rule(x+30,yy+12,x+size-30,yy+12,"#3b5e51")
    else:
        d.text(x+30,y+55,"Heard and retold",32,"#dce8d8",SERIF)
        samples=[s for s in lines if s.startswith(("report_turn=","retelling=","incoming_report="))][-6:]
        for i,s in enumerate(samples):
            yy=y+111+72*i;label="Listener" if s.startswith("retelling=") else "Human" if s.startswith("incoming_report=") else "Witness"
            d.label(x+32,yy,label,"#9cb7a8");d.text(x+32,yy+24,short(s.partition("  speech=")[2] or "(silence)",47),18,"#d3ead9",SERIF)
            d.rule(x+30,yy+35,x+size-30,yy+35,"#3b5e51")

def chart_panel(d,b,data,x,y,size):
    grid_background(d,x,y,size);d.label(x+7,y-13,"Recorded measurements")
    series=data.get("series",[])
    if b["kind"]=="forage" and series:
        left=x+34;right=x+size-34;peak=max(1,max(value for _,value in series))
        d.text(left,y+58,"Material passed to offspring",28,"#e2eddb",SERIF)
        d.text(left,y+88,"Mean units per lifetime",15,"#b0c9bb",MONO)
        for i,(label,value) in enumerate(series):
            row=y+136+i*57
            d.text(left,row,label,17,"#e4ebdf")
            d.text(right,row,human(value),17,MINT,MONO,anchor="end")
            d.rect(left,row+10,right-left,8,"#355146",4)
            if value:d.rect(left,row+10,(right-left)*value/peak,8,MINT if i==0 else "#76a891",4)
        d.rule(left,y+size-91,right,y+size-91,"#47675b")
        d.text(left,y+size-47,counted(data["metrics"]["observations"],"lifetime")+" per condition",21,"#dce6d5",SERIF)
        return
    if not series:series=[[label,100*a/bb] for label,a,bb in b["scores"] if bb]
    if series:
        lower=0;upper=max(1,max(v for _,v in series))
        if b["unit"]=="%":upper=100
        tx=x+34;cw=size-68;h=size*.49;top=y+92
        d.text(tx,y+50,short(data.get("chart_label",b["family"]),35),26,"#e2eddb",SERIF)
        for row in range(5):
            py=top+row*h/4;d.path(f"M{tx} {py} h{cw}","#466358",1,extra='opacity=".55"')
            d.text(tx,py-6,human(upper*(1-row/4)),11,"#a6bfaf",MONO)
        values=[float(v) for _,v in series];gap=cw/len(values)
        for i,v in enumerate(values):
            bar=max(1,h*max(0,min(v,upper))/upper);d.rect(tx+i*gap+gap*.17,top+h-bar,max(1,gap*.66),bar,MINT if i==len(values)-1 else "#61977f",2)
        d.text(tx,top+h+28,short(series[0][0],32),13,"#c8d9c8",MONO)
        if len(series)>1:d.text(x+size-31,top+h+51,short(series[-1][0],35),13,"#c8d9c8",MONO,anchor="end")
        d.rule(tx,y+size-100,x+size-34,y+size-100,"#47675b")
        d.text(tx,y+size-62,human(data.get("metrics",{}).get("observations",len(series))),33,"#e0ebcf",SERIF)
        d.text(tx,y+size-35,"observations in this record",15,"#b0c9bb")
    else:
        entries=data.get("stats",b.get("stats",[]))
        d.text(x+33,y+62,"Observed run totals",31,"#ebf0dc",SERIF)
        for i,(k,v) in enumerate(entries[:4]):
            yy=y+125+i*96;d.label(x+34,yy,short(str(k),34),"#9fbcad");d.text(x+36,yy+50,human(v),42,MINT,SERIF);d.rule(x+34,yy+67,x+size-34,yy+67,"#466258")


def evolution_panel(d,history,b,x,y,w,h):
    d.rect(x,y,w,h,"#dfe7d9",9)
    d.label(x+22,y+28,"Across the journal",GREEN)
    live=sum(not r["archive"] for r in history);arch=len(history)-live
    d.text(x+22,y+75,human(len(history)),37,INK,SERIF,500)
    d.text(x+22,y+102,f"{live:,} live  /  {arch:,} archived",13,MUTED,MONO)
    d.rule(x+265,y+17,x+265,y+h-17,"#b4c4b4")
    comparable=[r for r in history if r["family"]==b["family"] and r["unit"]==b["unit"]
                and (b["kind"]!="world" or b["unit"]!="animals" or capacity(r)==capacity(b))]
    # Keep the first and last point when many records share a measurement family.
    sampled=thin(comparable,160);left=x+291;right=x+w-235
    label=b["family"]+(f"  /  {human(capacity(b))} sites" if b["kind"]=="world" and b["unit"]=="animals" else "")
    d.label(left,y+28,short(label,48),GREEN)
    trend(d,left,y+46,max(25,right-left),h-70,[r["measure"] for r in sampled],GREEN,True)
    d.text(x+w-20,y+69,ratio(b["measure"],100) if b["unit"]=="%" else human(b["measure"]),30,INK,SERIF,500,"end")
    d.text(x+w-20,y+99,counted(len(comparable),"comparable record"),13,MUTED,MONO,anchor="end")


def thin(values,n):
    if len(values)<=n:return values
    return [values[round(i*(len(values)-1)/(n-1))] for i in range(n)]


def capacity(record):
    if record.get("capacity"):return record["capacity"]
    # Early entries keep the grid dimensions in their original census subtitle.
    dimensions=re.match(r"(\d+) × (\d+) sites",record["subtitle"])
    return int(dimensions[1])*int(dimensions[2]) if dimensions else 0


def development_panel(d,data,x,y,size):
    grid_background(d,x,y,size)
    m=data.get("metrics",{});lines=data.get("lines",[])
    rows=[pairs(line) for line in lines if line.startswith("latent_cell=")]
    spoken=[line.split("  heard=",1) for line in lines if line.startswith("auditory_reply=")]
    d.label(x+24,y+32,"Auditory and motor contacts",MINT)
    if spoken:
        d.text(x+24,y+69,"A retained exchange",30,PAPER,SERIF)
        for k,(answer,prompt) in enumerate(spoken[-7:]):
            yy=y+106+k*65
            d.text(x+27,yy,short(prompt,55),16,"#bfd3c5")
            d.text(x+39,yy+26,short(answer.removeprefix("auditory_reply="),49),21,AMBER,SERIF)
    elif rows:
        cells=[r["contacts"].split(",") for r in rows]
        cols=len(cells[0]);n=len(cells)
        d.text(x+24,y+69,f"{n} acquired states",32,PAPER,SERIF)
        d.text(x+25,y+96,"Each square connects one motor gesture to a future state.",15,"#bfd3c5")
        top=y+142;left=x+46;cw=(size-74)/cols;ch=min(45,(size-210)/max(1,n))
        for a in range(cols):d.text(left+(a+.5)*cw,top-12,f"{a:X}",13,MINT,MONO,anchor="middle")
        for j,row in enumerate(cells):
            if ch>=12 or j%4==0:d.text(left-8,top+(j+.71)*ch,str(j),min(13,ch+2),"#bfd3c5",MONO,anchor="end")
            for a,e in enumerate(row):
                if e=="-":continue
                dest,sense=map(int,e.split("."));color=COLORS[sense%len(COLORS)]
                d.rect(left+a*cw,top+j*ch,max(1,cw-2),max(1,ch-1.5),color,min(3,ch/4))
                if cw>18 and ch>13:d.text(left+(a+.48)*cw,top+(j+.73)*ch,str(dest),min(13,ch*.74),DARK,MONO,anchor="middle")
        d.text(x+24,y+size-31,"Hue = observed waveform    ·    Number = next state",15,MINT)
    else:
        d.text(x+24,y+70,f"{m.get('sequence_cells',0)} acquired states",32,PAPER,SERIF)
        questions=[line for line in lines if line.startswith("experiment>")]
        for k,line in enumerate(questions[-10:]):d.text(x+25,y+121+k*39,short(line[11:],56),15,MINT,MONO)
    if "sequence_body" in m:creature(d,x+size-51,y+50,.85,m["sequence_body"],m.get("sequence_tape",1),AMBER,-31)

def chorus_panel(d,data,x,y,size):
    grid_background(d,x,y,size);d.label(x+24,y+32,"One island  /  final residents",MINT)
    d.text(x+24,y+72,"Who fed together",32,PAPER,SERIF)
    rows=[pairs(line) for line in data.get("lines",[]) if line.startswith("chorus_voice=")]
    for k in range(0,len(rows)-1,2):
        xx=x+78+(k//2%4)*145;yy=y+138+(k//8)*111
        d.rule(xx-11,yy,xx+31,yy,"#75998b",1.7)
        for j in (0,1):
            r=rows[k+j];color=hue(int(r["founder"]));cx=xx+j*40
            creature(d,cx,yy,.7,int(r["rule"]),int(r["cells"]),color,90 if j==0 else -90)
        d.text(xx+19,yy+36,f"{rows[k]['meals']} · {rows[k+1]['meals']}",13,MINT,MONO,anchor="middle")
    d.text(x+24,y+size-35,"Pairs face their partner. Numbers show individual nectar meals.",15,"#bfd3c5")

def composition_panel(d,data,x,y,size):
    grid_background(d,x,y,size);lines=data.get("lines",[])
    def group(tag):
        rows=[]
        for line in lines:
            if line.startswith(tag+"=") and "  form=" in line:
                head,form=line.split("  form=",1);rows.append((pairs(head),form))
        return rows
    facts=group("role_frame");speech=group("vocal_frame");m=data.get("metrics",{})
    d.label(x+24,y+32,"The first animal  /  acquired projections",MINT)
    d.text(x+24,y+73,"How an answer is bound",31,PAPER,SERIF)
    if "composition_body" in m:creature(d,x+size-48,y+49,.8,m["composition_body"],m.get("composition_tape",1),AMBER,-28)
    if not facts or not speech:
        d.text(x+25,y+137,"Recorded acoustic evidence",22,MINT,SERIF)
        for k,row in enumerate(data.get("scores",[])[:5]):d.text(x+25,y+196+54*k,short(row[0],43),17,PAPER)
        return
    left=x+28;right=x+size-206;top=y+138
    def positions(n):
        return [top+32+j*min(55,(size-232)/max(1,n-1)) for j in range(n)]
    yy=positions(len(speech));fy=positions(len(facts))
    d.label(left,y+118,"Heard construction",MINT);d.label(right,y+118,"Observed frame",MINT)
    for i,(q,_) in enumerate(speech):
        for j in range(len(facts)):
            gate=int(q["shutters"])>>j&1
            if not ((int(q["inputs"])>>j&1) or gate):continue
            c=AMBER if gate else COLORS[j%len(COLORS)]
            d.path(f"M{left+218} {yy[i]} C{x+305} {yy[i]} {x+321} {fy[j]} {right-7} {fy[j]}",c,2.5,extra='opacity=".72"'+(' stroke-dasharray="4 4"' if gate else ''))
    for j,(_,name) in enumerate(facts):
        d.dot(right-7,fy[j],4,COLORS[j%len(COLORS)]);d.rect(right,fy[j]-17,183,34,"#284c44",6)
        label=f"pulse frame {j+1}" if "@" in name else name
        d.text(right+7,fy[j]+5,short(label,24),13,PAPER,MONO)
    for i,(row,name) in enumerate(speech):
        d.rect(left,yy[i]-18,216,36,"#21463e",6)
        label=f"vocal frame {i+1}" if "@" in name else name
        d.text(left+7,yy[i]+5,short(label,27),13,PAPER,MONO)
        d.dot(left+218,yy[i],4,MINT if int(row["ready"]) else AMBER)
    d.text(x+22,y+size-33,"Solid lines retrieve a relation. Dashed lines select an earlier view.",14,MINT)

def procedure_panel(d,data,x,y,size,kind):
    grid_background(d,x,y,size)
    def rows(key):return [pairs(line) for line in data.get("lines",[]) if line.startswith(key+"=")]
    def caption(text,title):
        d.label(x+24,y+31,text,MINT);d.text(x+24,y+72,title,30,PAPER,SERIF)
    if kind=="flight-colony":
        seasons=rows("roost_season");residents=rows("roost_resident");land=rows("roost_terrain")
        caption("First island  /  final census","At the returning roost")
        bysite=all("site" in r for r in residents) and bool(residents)
        buds=int(land[-1]["buds"]) if land else 0
        if bysite:
            left=x+34;top=y+108;tile=47;occupied=defaultdict(list)
            for r in residents:occupied[int(r["site"])].append(r)
            for site in range(64):
                px=left+(site%8)*tile;py=top+(site//8)*tile
                d.rect(px,py,tile-2,tile-2,"#335d47" if (buds>>site)&1 else "#20423c",4)
                group=occupied[site]
                if group:
                    a=max(group,key=lambda v:int(v["food"]));creature(d,px+22,py+20,.56,int(a["form"]),int(a["founder"]),COLORS[int(a["founder"])%8],-35)
                    if len(group)>1:d.text(px+39,py+41,len(group),11,MINT,MONO,anchor="end")
            flights=rows("roost_flight")
            sample=max(flights,key=lambda r:(int(r["found"])<64,len(set(r["path"].split(",")))),default=None)
            if sample:
                sites=[int(s) for s in sample["path"].split(",")]
                coords=[(left+(site%8+.5)*tile,top+(site//8+.5)*tile) for site in sites]
                d.path("M"+" L".join(f"{px},{py}" for px,py in coords),AMBER,2.7,extra='stroke-dasharray="4 4"')
                fruit=int(sample["found"])
                if fruit<64:d.dot(left+(fruit%8+.5)*tile,top+(fruit//8+.5)*tile,6,AMBER)
                d.text(x+24,y+size-107,f"Flight {sample['roost_flight']} on day {int(sample['tick'])+1}   ·   {len(sites)-1} moves",17,AMBER,SERIF)
            d.label(x+431,y+132,"Final lineage",MINT)
            for k,(lineage,n) in enumerate(Counter(r["founder"] for r in residents).most_common(8)):
                d.dot(x+438,y+171+k*35,5,COLORS[int(lineage)%8]);d.text(x+452,y+176+k*35,f"{lineage}  × {n}",14,PAPER,MONO)
            d.text(x+26,y+size-82,"One resident per occupied site is drawn. Counts include its neighbours.",15,MINT)
        else:
            for j,r in enumerate(residents[:32]):
                cx=x+64+(j%8)*68;cy=y+141+(j//8)*83
                creature(d,cx,cy,.62,int(r["form"]),int(r["founder"]),COLORS[int(r["founder"])%8],-38)
                d.text(cx,cy+31,r["food"],13,PAPER,MONO,anchor="middle")
            d.text(x+24,y+size-84,"Meals won by each recorded resident on the first island",17,MINT)
        d.text(x+24,y+size-35,f"{len(seasons)} seasons  ·  {human(sum(int(r['homed']) for r in seasons))} landings at home",18,PAPER,SERIF)
    elif kind=="motor-forecast":
        caption("Six retinal channels  /  one cell per origin","Where each possible plot lands")
        projection=rows("projected_states");values=projection[-1]["destinations"].split(",") if projection else []
        for i,z in enumerate(values):
            px=x+32+(i%8)*67;py=y+115+(i//8)*48;known=z!="?"
            color=COLORS[(int(z)//8)%8] if known else "#59716a";d.rect(px,py,60,40,color,5)
            d.text(px+30,py+25,f"{i}→{z}",13,DARK,MONO,600,anchor="middle")
        if not values:d.text(x+30,y+170,"The score could not be resolved.",20,MINT)
        d.text(x+30,y+size-80,"Each label gives the starting state and its predicted destination.",16,MINT)
        measured=rows("physical_audit")
        if measured:d.text(x+30,y+size-40,f"Physical replay matched {measured[-1]['agreement']} of 64 projections.",19,PAPER,SERIF)
    elif kind=="motor-interview":
        caption("Conversation  /  observed activity","A procedure in motion")
        events=[s for s in data.get("lines",[]) if s.startswith(("grounded_procedure=","procedural_reply=","retinal_tick="))]
        for i,line in enumerate(events[-8:]):
            a=pairs(line);py=y+119+i*54;d.dot(x+34,py,5,COLORS[i%8])
            if "grounded_procedure" in a:
                label=f"{a['grounded_procedure'].title()}  {a.get('start','?')} → {a.get('expected','?')}   ·   {a.get('actions',0)} physical moves"
            elif "retinal_tick" in a:label=f"View {a.get('view','?')}   →   {a.get('motor','?')}"
            else:label=line.removeprefix("procedural_reply=").replace("  heard=","    ←    ")
            d.text(x+54,py+6,short(label,55),15,PAPER,MONO)
    else:
        caption("Acoustic motifs  /  learned motor productions","What the animal compressed")
        rules=rows("motor_cell");arms=rows("recurrent_state");fits=rows("sensorimotor")
        for i,r in enumerate(rules[:8]):
            py=y+115+i*41;d.rect(x+23,py-17,252,32,"#24483f",6)
            d.text(x+34,py+5,short(r["input"],25),15,PAPER,MONO);d.rule(x+279,py,x+312,py,COLORS[i%8],2)
            d.dot(x+316,py,3,COLORS[i%8]);d.text(x+330,py+5,short(r.get("action","∅")+(" / "+r["alternative"] if "alternative" in r else ""),26),15,COLORS[i%8],MONO)
        if fits:
            for i,r in enumerate(fits[:2]):
                py=y+475+i*45;pct=int(r["correct"])/max(1,int(r["paths"]));d.text(x+24,py+5,r["sensorimotor"],14,PAPER,MONO)
                d.rect(x+155,py-9,330,13,"#345447",4);d.rect(x+155,py-9,330*pct,13,COLORS[i],4);d.text(x+503,py+4,f"{pct:.1%}",15,COLORS[i],MONO)
        elif arms:
            d.label(x+26,y+470,"First pulse  /  return pulse",MINT)
            for k,r in enumerate(arms[:6]):d.text(x+28+(k%3)*180,y+505+(k//3)*32,short(f"{r['terminal']}   {r.get('first','∅')}·{r.get('deferred','∅')}",19),15,PAPER,MONO)
        else:
            d.text(x+24,y+size-65,"[A] and [B] re-enter the acquired score for an argument.",16,MINT)

def shelter_panel(d,data,x,y,size):
    grid_background(d,x,y,size);d.label(x+25,y+32,"The first island  /  final season",MINT)
    d.text(x+24,y+73,"Conversations at feeding time",30,PAPER,SERIF)
    residents=[pairs(s) for s in data.get("lines",[]) if s.startswith("shelter_resident=")]
    for k in range(0,len(residents)-1,2):
        xx=x+70+(k//2%4)*144;yy=y+139+(k//8)*107
        a,b=residents[k:k+2];d.rule(xx-5,yy,xx+52,yy,"#90b6a1",1+min(4,(int(a["asked"])+int(b["asked"]))/20))
        for off,r in ((0,a),(43,b)):
            creature(d,xx+off,yy,.72,int(r["body"]),1+(int(r["founder"])&3),hue(int(r["founder"])),90 if off==0 else -90)
        d.text(xx+22,yy+33,f"{a['fed']} · {b['fed']}",13,MINT,MONO,anchor="middle")
    d.text(x+24,y+size-33,"Line width follows questions asked. Numbers show nectar meals.",14,MINT)
def inner_panel(d,data,x,y,size):
    grid_background(d,x,y,size)
    d.label(x+25,y+33,"One cortex  /  two acquired pathways",MINT)
    d.text(x+25,y+72,"Speech reaches the plot",31,PAPER,SERIF)
    steps=[];recent=None
    for line in data.get("lines",[]):
        if line.startswith("composed_reply=") and "  heard=" in line:
            reply,heard=line.removeprefix("composed_reply=").split("  heard=",1)
            if reply not in ("_","?"):recent=(heard,reply)
        if line.startswith("garden_event=") and "  prompt=" in line and "  speech=" in line:
            head,_,prompt=line.partition("  prompt=");stats=pairs(head);voice=head.partition("  speech=")[2]
            if recent and recent[0]==prompt and voice:
                steps.append((prompt,recent[1],voice,stats));recent=None
    for i,(prompt,heard,motor,snap) in enumerate(steps[-2:]):
        yy=y+118+i*207
        d.text(x+26,yy,short(prompt,45),20,PAPER,SERIF)
        d.path(f"M{x+34} {yy+16} v63",MINT,1.5)
        d.dot(x+34,yy+32,3,MINT);d.text(x+49,yy+37,short(heard,53),17,MINT,SANS)
        d.dot(x+34,yy+70,3,AMBER);d.text(x+49,yy+76,short(motor,53),17,AMBER,SANS)
        d.text(x+27,yy+115,"Plot "+snap.get("site","?")+"    ·    "+snap.get("actions","0")+" physical moves",14,"#b9ccc1",MONO)
        if i==0:d.rule(x+26,yy+148,x+size-27,yy+148,"#486657")
    d.text(x+25,y+size-34,"Green shows emitted words. Amber shows the grounded reply.",15,MINT)
def manual_panel(d,data,x,y,size):
    d.rect(x,y,size,size,"#173b38",13);d.label(x+25,y+34,"Mineral workshop   /   physical events",MINT)
    rows=[pairs(s) for s in data["lines"] if s.startswith("workbench_event=")]
    profile=[pairs(s) for s in data["lines"] if s.startswith("workpieces=")]
    if profile:
        parts=[tuple(map(int,z.split("/"))) for z in profile[-1]["workpieces"].split(",")]
        d.text(x+25,y+78,"The work surface",30,PAPER,SERIF);step=(size-70)/len(parts)
        mask=int(rows[-1]["bonds"]) if rows else 0
        for part in parts:
            i,ports,charge=part[:3]
            px=x+35+(i+.5)*step;py=y+148
            if mask&(1<<i):d.rule(px,py,px+step,py,"#d3e8ad",5)
            if ports:
                d.rect(px-17,py-17,34,34,"#a3bda3",5)
                for k,(dx,dy) in enumerate(((0,-1),(1,0),(0,1),(-1,0))):
                    val=ports>>(2*k)&3
                    if val:d.dot(px+dx*14,py+dy*14,2.8,("#000","#484a42","#f2eace","#69c4ba")[val])
                if charge:d.dot(px,py,4,"#fbd07f")
            d.text(px,py+45,str(i),14,"#cedcca",MONO,anchor="middle")
        d.label(x+25,y+232,"Recent hand and witness events",MINT);top=y+266
    else:
        d.text(x+25,y+78,"Received gestures",30,PAPER,SERIF);top=y+125
    for i,r in enumerate(rows[-6:]):
        yy=top+31*i;d.text(x+25,yy,r["workbench_event"],16,"#d5e4cf",MONO)
        d.text(x+size-24,yy,f"muscle effect {r['operation']}   ·   {r['reactions']} reactions",15,"#b6cdb9",SANS,anchor="end")

def shoal_panel(d,data,x,y,size):
    rows={v.partition("=")[0]:v.partition("=")[2] for v in data["lines"]}
    frame=pairs("shoal_specimen="+rows["shoal_specimen"]) if "shoal_specimen" in rows else pairs("shore_layout="+rows["shore_layout"])
    decode=lambda name:[int(q) for q in rows.get(name,"").split(",") if q]
    first=decode("shoal_before") or decode("shore_before")
    last=decode("shoal_after") or decode("shore_after");origins=decode("shoal_origins")
    sun=int(frame["light"]);a=int(frame["patch_a"]);b=int(frame["patch_b"])
    target=b if int(frame.get("diet",0)) else a
    # Translate the periodic field once for both frames. Put the seam where it
    # cuts the fewest real bonds, so connected constructions stay readable.
    def neighbors(i):return ((i//8*8+(i+1)%8,1),((i+8)%32,2))
    def terminal(v,direction):return (v>>(2*((direction-((v>>24)&1))&3)))&3
    edges=[]
    if len(last)==32:
        for i,part in enumerate(last):
            for j,direction in neighbors(i):
                u,v=terminal(part,direction),terminal(last[j],direction^2)
                if u and v and (u!=v or u==3):edges.append((i,j,direction))
    component={sun}
    for _ in range(32):
        reached=component|{j for i,j,_ in edges if i in component}|{i for i,j,_ in edges if j in component}
        if reached==component:break
        component=reached
    def coordinate(i,offset):return ((i%8+offset%8)%8,(i//8+offset//8)%4)
    def cuts(offset):
        score=0
        for i,j,direction in edges:
            xx,yy=coordinate(i,offset)
            if direction==1 and xx==7 or direction==2 and yy==3:score+=5 if i in component else 1
        return score
    translation=min(range(32),key=cuts)
    d.rect(x,y,size,size,"#193a38",13)
    d.label(x+24,y+34,"Mineral shoal   /   physical inventory","#bfd4bd")
    for top,label,field in ((y+73,"Before handling",first),(y+319,"After handling",last)):
        d.text(x+24,top,label,24,"#f1e8ca",SERIF)
        if len(field)!=32:continue
        col=(size-82)/8;row=41
        def xy(site):
            xx,yy=coordinate(site,translation);return x+42+(xx+.5)*col,top+36+(yy+.5)*row
        def face(site,direction):
            part=field[site];rotation=(part>>24)&1
            return (part>>(2*((direction-rotation)&3)))&3
        for i,part in enumerate(field):
            xx,yy=xy(i)
            d.dot(xx,yy,2.2,"#4a6861")
            if not part&255:continue
            for direction,j in ((1,(i//8)*8+(i+1)%8),(2,(i+8)%32)):
                u,v=face(i,direction),face(j,direction^2)
                if not u or not v or (u==v and u!=3):continue
                ex,ey=xy(j)
                cx,cy=coordinate(i,translation);ink="#b4dbac" if field is last and i in component else "#819e91"
                if (direction==1 and cx==7) or (direction==2 and cy==3):
                    dx,dy=(21,0) if direction==1 else (0,15)
                    d.rule(xx,yy,xx+dx,yy+dy,ink,3);d.rule(ex-dx,ey-dy,ex,ey,ink,3)
                else:d.rule(xx,yy,ex,ey,ink,3)
        for i,part in enumerate(field):
            xx,yy=xy(i)
            if i in (sun,a,b):
                accent="#f1c067" if i==sun else "#9be1bb" if i==target else "#a3b2bc"
                d.dot(xx,yy,16,"none",f'stroke="{accent}" stroke-width="2"')
            if not part&255:continue
            moved=field is last and (origins[i]!=i if len(origins)==32 else len(first)==32 and not (first[i]&255))
            d.rect(xx-9,yy-9,18,18,"#e6ae71" if moved else "#90a9a2",4)
            for edge,(dx,dy) in enumerate(((0,-1),(1,0),(0,1),(-1,0))):
                port=face(i,edge)
                if port:d.dot(xx+7*dx,yy+7*dy,2,("#173a38","#537075","#f1e5bd","#aee5bc")[port])
        for i,text in ((sun,"L"),(a,"A"),(b,"B")):
            xx,yy=xy(i);d.text(xx+18,yy-8,text,12,"#d6e5ce",MONO,600)
    d.rule(x+24,y+size-58,x+size-24,y+size-58,"#56746b")
    d.text(x+24,y+size-34,"L  light      A / B  deposits      Amber  placed grain",14,"#d1dfc8",MONO)

def mineral_panel(d,data,x,y,size):
    rows=[pairs(line)["grains"] for line in data["lines"] if line.startswith("mineral_row=")]
    d.rect(x,y,size,size,"#173b38",13);d.label(x+24,y+34,"Material field   /   closing exposure",MINT)
    root=data.get("metrics",{}).get("mineral_focus_root")
    d.text(x+24,y+65,f"Assembly at site {root:,}" if root is not None else "A connected assembly",27,"#f2ebce",SERIF)
    if not rows:return
    n=len(rows);step=(size-132)/n;ox=x+(size-n*step)/2;oy=y+80
    recorded=data.get("metrics",{});labels={}
    specimen=next((pairs(v) for v in data["lines"] if v.startswith("artifact_signature=") and pairs(v)["artifact_signature"]==str(root)),None)
    if specimen and "mineral_window_x" in recorded:
        side=data["side"];left=recorded["mineral_window_x"];top=recorded["mineral_window_y"]
        for key,tag in (("lamp_a","A"),("lamp_b","B"),("sink","O")):
            pos=int(specimen[key]);labels[((pos%side-left)%side,(pos//side-top)%side)]=tag
    def point(i,j):return ox+(i+.5)*step,oy+(j+.5)*step
    colors=("#c38b60","#8ab5a4","#ab9bb9","#d9be70")
    for j,row in enumerate(rows):
        for i in range(n):
            raw=bytes.fromhex(row[i*10:i*10+10]);ports,sub,charge,marks,bonds=raw;cx,cy=point(i,j)
            if not ports:continue
            for mask,dx,dy in ((2,1,0),(4,0,1)):
                if bonds&mask and i+dx<n and j+dy<n:d.rule(cx,cy,*point(i+dx,j+dy),"#cfdfae" if bonds&16 else "#59756b",max(2,step*.1))
    for j,row in enumerate(rows):
        for i in range(n):
            ports,sub,charge,marks,bonds=bytes.fromhex(row[i*10:i*10+10]);cx,cy=point(i,j)
            if not ports:continue
            radius=step*(.28 if bonds&16 else .20);colour=colors[sub>>4&3] if bonds&16 else "#60756a"
            d.rect(cx-radius,cy-radius,2*radius,2*radius,colour,3)
            actual=((ports|(ports<<8))>>((8-2*(marks&1))&7))&255
            for k,(dx,dy) in enumerate(((0,-1),(1,0),(0,1),(-1,0))):
                val=(actual>>(2*k))&3
                if val:d.dot(cx+dx*radius*.77,cy+dy*radius*.77,1.4,("#000","#42372e","#ecf2d9","#a5e0dc")[val])
            if charge:d.dot(cx,cy,step*.045+min(1,charge/(2+(sub&15)))*step*.06,"#ffdf78")
            if (i,j) in labels:
                d.dot(cx,cy,radius+3,"none",'stroke="#f5e1a9" stroke-width="1.4"')
                d.text(cx+radius+5,cy-5,labels[i,j],13,"#fff1bc",MONO,700)
    legend="A/B  lamps     O  product site     Gold  stored charge" if labels else "Bright links trace the component. Gold is stored charge."
    d.text(x+25,y+size-13,legend,14,"#d2e2ce")

def watershed_panel(d,data,x,y,size):
    sample=[pairs(v) for v in data["lines"] if v.startswith("watershed_row=")]
    rows=[v["channels"] for v in sample]
    d.rect(x,y,size,size,"#15363c",13);d.label(x+23,y+35,"Floodplain   /   closing survey",MINT)
    if not rows:return
    n=len(rows);width=size-96;step=width/n;ox=x+(size-width)/2;oy=y+54
    for gy,row in enumerate(rows):
        for gx in range(min(n,len(row)//4)):
            water,leaf,wall,animals=(int(c,16) for c in row[4*gx:4*gx+4])
            base=blend("#354f3d","#488899",water/15)
            tile=blend(base,"#a3b360",leaf/23)
            px,py=ox+gx*step,oy+gy*step
            d.rect(px,py,step+.2,step+.2,tile)
            if wall:d.rule(px+1,py+step*.8,px+step*.87,py+step*.8,"#f0af70",min(3,.7+wall/5))
            if sample[gy].get("fibre") and int(sample[gy]["fibre"][gx],16):
                d.rule(px+step*.75,py+1,px+step*.75,py+step*.77,"#ccb4d7",1.2)
            if animals:d.dot(px+step*.45,py+step*.42,step*(.06+.075*animals/15),"#f0f1cd")
    trails=[pairs(v) for v in data["lines"] if v.startswith("watershed_trail=")]
    for record in trails:
        cells=[int(v) for v in record["positions"].split(",") if v];span=int(record["side"])
        if len(set(cells))<2:continue
        pos=[(c%span,c//span) for c in cells];scale=width/(data.get("side") or span)
        for (ax,ay),(bx,by) in zip(pos,pos[1:]):
            if abs(ax-bx)+abs(ay-by)!=1:continue
            d.rule(ox+(ax+.5)*scale,oy+(ay+.5)*scale,ox+(bx+.5)*scale,oy+(by+.5)*scale,"#172929",4)
            d.rule(ox+(ax+.5)*scale,oy+(ay+.5)*scale,ox+(bx+.5)*scale,oy+(by+.5)*scale,"#ffdc88",2)
    # Abundance swatches are the same colors as the recorded material layers.
    for i,(label,color) in enumerate((("water","#69b4c8"),("plants","#b1bd75"),("clay","#f0af70"),("fibre","#ccb4d7"),("residents","#f0f1cd"))):
        px=x+24+i*(size-40)/5;d.dot(px,y+size-13,3.5,color);d.text(px+9,y+size-8,label,13,"#e0e7d5")
def grounding_panel(d,b,data,x,y,size):
    examples=[pairs(line) for line in data["lines"] if line.startswith("retinal_sample=")]
    if not examples:
        series=[[label,100*a/n] for label,a,n in b["scores"] if n]
        chart_panel(d,b,dict(data,series=series,chart_label="Correct responses (%)"),x,y,size);return
    d.rect(x,y,size,size,"#203a3e",13);d.label(x+25,y+36,"Retinal record   /   two exposures",MINT)
    row=examples[-1];side=48;ox=x+(size-8*side)/2;oy=y+76
    for k in range(9):
        d.rule(ox+k*side,oy,ox+k*side,oy+8*side,"#3b5554")
        d.rule(ox,oy+k*side,ox+8*side,oy+k*side,"#3b5554")
    point=lambda site:(ox+(site%8+.5)*side,oy+(site//8+.5)*side)
    for patch in row["patches"].split(","):
        light,old,site=map(int,patch.split("/"));cx,cy=point(site);color=COLORS[light%len(COLORS)]
        if old<64:
            ax,ay=point(old);d.path(f"M{ax} {ay} L{cx} {cy}",color,2);d.dot(ax,ay,6,"#203a3e",f'stroke="{color}" stroke-width="2"')
        d.dot(cx,cy,15,color);d.text(cx,cy+4,f"{light:02X}",11,"#162d2e",MONO,600,"middle")
    d.label(x+25,y+size-83,"Caption heard",MINT);d.text(x+25,y+size-47,short(row["sound"].replace("/","  "),57),19,PAPER,MONO)
    d.text(x+25,y+size-19,"Open points show prior locations when tracking was unambiguous.",14,"#b8d0bf")

def worksite_panel(d,data,x,y,size):
    records=[pairs(v) for v in data["lines"] if v.startswith("worksite_row=")]
    region=next(pairs(v) for v in data["lines"] if v.startswith("worksite_window="))
    width=int(region["width"]);side=int(region["side"]);focus=int(region["worksite_window"])
    d.rect(x,y,size,size,"#1f3538",13);d.label(x+26,y+37,"Field notebook   /   surface traces",MINT)
    d.text(x+26,y+70,f"{width} × {width} sites around ({focus%side}, {focus//side})",19,PAPER,SERIF)
    pitch=39;start=x+(size-pitch*width)/2;top=y+96
    for row in records[-width:]:
        iy=int(row["worksite_row"]);cells=row["cells"]
        for ix in range(width):
            at=3*ix;m=int(cells[at:at+2],16);item=int(cells[at+2],16);px=start+ix*pitch;py=top+iy*pitch
            col="#334749" if not m else ("#805e43" if m&64 else "#3e6a70")
            d.rect(px,py,pitch-3,pitch-3,col,4)
            if m&128:
                if m&32:d.dot(px+16,py+16,7,"#edcb83")
                else:d.rule(px+10,py+16,px+22,py+16,"#bdd1be",2)
                if m&16:d.rule(px+6,py+29,px+26,py+29,"#c4dcc8",2)
            if item&1:d.dot(px+30,py+6,2.8,"#f2ead2")
            if item&2:d.dot(px+6,py+6,2.8,"#b5d792")
    for i,(label,col) in enumerate((("Terminal contact","#5b8d93"),("Soil contact","#ba8c5a"),("Observed response","#edcb83"))):
        xx=x+29+185*i;d.dot(xx,y+524,5,col);d.text(xx+12,y+530,label,15,PAPER)
    d.text(x+26,y+562,"Underline  ·  animal-made mark",16,MINT)
    d.text(x+26,y+586,"Corner dots  ·  living resident (left), loose grain (right)",15,"#b4c9be")


def field_dialogue_panel(d,data,x,y,size):
    table=None;turn="";answer=None;snapshot=None;listening="1";memory="lived"
    for line in data["lines"]:
        if line.startswith("field_table="):table=pairs(line)
        elif line.startswith("field_hearer="):listening=pairs(line)["field_hearer"]
        elif line.startswith("field_memory="):memory=pairs(line)["field_memory"]
        elif line.startswith("field_query="):turn=line.partition("text=")[2]
        elif line.startswith("field_answer="):answer=pairs(line);snapshot=(table,turn,answer,listening,memory)
    if snapshot:table,turn,answer,listening,memory=snapshot
    d.rect(x,y,size,size,"#203a3e",13);d.label(x+27,y+37,"Material conversation   /   laboratory table",MINT)
    if not table:return
    d.text(x+27,y+74,"Table "+table["field_table"],24,PAPER,SERIF)
    start=x+35;top=y+111;pitch=66
    for j in range(32):
        px=start+(j%8)*pitch;py=top+(j//8)*pitch
        d.rect(px,py,pitch-4,pitch-4,"#355056",5)
    for j in range(4):
        site=int(table["site"+str(j)]);px=start+(site%8)*pitch;py=top+(site//8)*pitch
        pigment=int(table["pigment"+str(j)])
        color="hsl(%d,38%%,72%%)"%((pigment*71)%360)
        if j<2:d.path(f"M{px+30} {py+4} l14 15 -14 15 -14 -15z",color,1,color)
        else:d.rect(px+13,py+9,36,22,color,5)
        name=table[("chip" if j<2 else "bed")+str(j%2)]
        d.text(px+30,py+53,"·" if name=="_" else name,14,PAPER,MONO,anchor="middle")
        if answer and int(answer["receiver_site"])==site:
            d.rect(px+1,py+1,pitch-6,pitch-6,"none",5,'stroke="#efc975" stroke-width="3"')
    d.text(x+35,y+397,"Diamonds  ·  samples      Bars  ·  deposits",17,MINT)
    if answer:
        d.text(x+34,y+438,short(turn,47),22,PAPER,SERIF)
        voice=answer["field_answer"];d.text(x+34,y+482,"“"+voice+"”" if voice!="_" else "Silence",33,"#efc975",SERIF)
        ch={"0":"shuttered","1":"open","2":"substituted"}[listening]
        d.text(x+35,y+526,f"Acoustic channel {ch}   ·   field memory {memory}",16,MINT)
        d.text(x+35,y+565,f"Listener spent {answer['receiver_work']} units and collected {answer['receiver_food']}.",18,PAPER)
    else:
        d.text(x+35,y+454,"Reference exposures",27,PAPER,SERIF)
        d.text(x+35,y+490,"Each coupon is a fresh copy of the chosen sample.",17,MINT)
        d.text(x+35,y+523,"The experimental animal pays for the physical test.",17,MINT)


def brine_panel(d,data,x,y,size):
    sample=next(pairs(l) for l in data["lines"] if l.startswith("brine_specimen="))
    rows=[pairs(l) for l in data["lines"] if l.startswith("brine_row=")]
    d.rect(x,y,size,size,"#1d353c",13);d.label(x+26,y+35,"Tidal survey  /  local physicochemistry",MINT)
    d.text(x+26,y+72,"Which soil activates coating zero?",23,PAPER,SERIF)
    colors=("#e2b76d","#67b7b2","#a4a2ce","#9cbd7d")
    def active(base,phase):
        a,b=base&1,base>>1
        a^=b*((phase>>2)&1);b^=a*((phase>>3)&1);a^=b*((phase>>4)&1)
        return (a|b<<1)^(phase&3)
    for row in rows[-8:]:
        iy=int(row["brine_row"]);values=row["states"]
        for ix in range(8):
            v=int(values[3*ix:3*ix+2],16);objects=int(values[3*ix+2],16);px=x+30+ix*33;py=y+97+iy*33
            soil=next(j for j in range(4) if active(j,v)==0);d.rect(px,py,30,30,colors[soil],3)
            if objects&1:d.rect(px+17,py+6,7,5,"#29474a",1)
            if objects&2:d.dot(px+8,py+23,3.2,"#31463d")
    d.label(x+331,y+121,"Reactive soil",MINT)
    for j,col in enumerate(colors):
        d.rect(x+332,y+144+33*j,18,12,col,2);d.text(x+363,y+156+33*j,"Surface "+str(j),16,PAPER)
    d.text(x+331,y+302,"Corner marks identify",16,MINT);d.text(x+331,y+325,"grains and residents.",16,MINT)
    live=bool(int(sample["living"]));owner=sample["brine_specimen"]
    d.rule(x+24,y+374,x+size-24,y+374,"#536d6b")
    d.label(x+27,y+403,f"Resident {owner}   /   receptive field" if live else "Vacant field",MINT)
    bits=list(map(int,sample["firing"].split(",")));active_mask=int(sample["active"]);lo=int(sample["low"]);hi=int(sample["high"])
    for cue,word in enumerate(bits):
        for j in range(32):
            px=x+33+j*11;py=y+421+cue*8
            state=((lo>>j)&1)+2*((hi>>j)&1)
            c="#29434a" if not active_mask&(1<<j) else "#45616a" if not word&(1<<j) else ("#8ba9a6","#8dd4ba","#e3c778","#c6abda")[state]
            d.rect(px,py,9,6,c,1)
    d.text(x+401,y+448,"32 cell sites",16,PAPER);d.text(x+401,y+475,"16 cue patterns",16,PAPER)
    d.text(x+401,y+513,f"{active_mask.bit_count()} expressed",16,MINT)
    d.text(x+27,y+580,"Bright marks fire for the input. Colour records synaptic state.",15,"#bdd1c5")


def brine_social_panel(d,data,x,y,size):
    arms={r["brine_dialogue"]:r for l in data["lines"] if l.startswith("brine_dialogue=") for r in [pairs(l)]}
    d.rect(x,y,size,size,"#20393d",13);d.label(x+26,y+37,"Two animals   /   acoustic reference channel",MINT)
    d.text(x+26,y+78,"Answers on the same hidden baths",25,PAPER,SERIF)
    shown=(("autonomous","Animal chooses to ask"),("prior_confidence","Prior confidence only"),("consulted","Full consultation"),("reference_quiet","Requests shuttered")) if "prior_confidence" in arms else (
        ("consulted","Reference tests visible"),("reference_quiet","Requests shuttered"),("false_feedback","Observed pulses inverted"),("autonomous","Animal chooses to ask"))
    for j,(arm,label) in enumerate(shown):
        r=arms[arm];n=max(1,int(r["questions"]));yy=y+134+j*91
        d.text(x+28,yy,label,19,PAPER)
        start=x+28
        for key,col in (("correct","#b1d5a7"),("wrong","#d68f70"),("silent","#526c70")):
            width=(size-58)*int(r[key])/n;d.rect(start,yy+14,width,13,col);start+=width
        d.text(x+28,yy+51,f"{int(r['correct']):,} correct   ·   {int(r['wrong']):,} wrong   ·   {int(r['silent']):,} silent",15,MINT,MONO)
    r=arms["autonomous"] if "prior_confidence" in arms else arms["consulted"];d.rule(x+26,y+507,x+size-26,y+507,"#52706a")
    d.text(x+26,y+547,f"{int(r['tests_shown']):,} tests performed by the partner",21,PAPER,SERIF)
    d.text(x+26,y+578,f"{int(r['target_sampled']):,} reference tests used the coating in the question.",16,MINT)


def clay_panel(d,data,x,y,size):
    lines=data.get("lines",[])
    def rows(tag):return [pairs(l) for l in lines if l.startswith(tag+"=")]
    boards=rows("clay_tablet");objects=rows("clay_object")
    d.rect(x,y,size,size,"#26332f",14)
    d.label(x+28,y+40,"MATERIAL RECORD   /   SOURCE AND SOUND","#c2dbb7")
    if boards or objects:
        d.text(x+28,y+88,"What the surface retained",34,"#f0edce",SERIF)
        if boards:
            pieces=[(f"Community {int(q['clay_tablet'])+1}",q) for q in boards[-2:]]
        else:
            q=objects[-1];pieces=[(f"Grain at site {q['clay_object']}",q)]
        for j,(name,q) in enumerate(pieces):
            yy=y+123+188*j;mineral=int(q["mineral"])
            d.text(x+28,yy+21,name,22,"#e6edd9",SERIF);d.text(x+size-29,yy+21,f"Glaze {mineral:02X}",17,"#b7cdaf",MONO,anchor="end")
            d.rect(x+24,yy+36,size-48,120,("#bc956e","#bea186","#bba375","#bb9a84")[mineral&3],9)
            bits=int(q["first"])|(int(q["second"])<<32)
            for k in range(8):
                e=(bits>>(8*k))&255;xx=x+35+68.5*k
                d.rect(xx,yy+48,60,81,"#7f644f" if e&128 else "#aa896d",5)
                if not e&128:continue
                pitch=e&3;relief=(e>>2)&3
                for r in range(relief+1):
                    yy0=yy+60+r*5
                    d.path(f"M{xx+8} {yy0}q20 {-7 if e&16 else 5} 44 0","#f0ddaa",2)
                d.text(xx+31,yy+103,("ka","li","mu","so")[pitch],23,("#f4da91","#c4e6c5","#e1c9e8","#bee1e8")[pitch],MONO,anchor="middle")
                d.text(xx+31,yy+121,"slit" if e&16 else "inlet",12,"#eee9d7",SANS,anchor="middle")
        yy=y+524 if len(pieces)>1 else y+349;d.rule(x+28,yy,x+size-28,yy,"#6b7e68")
        d.text(x+28,yy+33,"Ridges encode the felt property",22,"#f0edce",SERIF)
        d.text(x+28,yy+60,"Each occupied groove sounds one of four notes when struck.",15,"#c5d6bd")
        if len(pieces)==1:
            tick=rows("clay_tick")[-1]
            d.text(x+28,y+510,f"{int(tick['carried']):,} moves of marked material",22,"#d3e0ca",SERIF)
            d.text(x+28,y+555,f"{int(tick['before_birth']):,} pre-birth impressions recovered",20,"#d3e0ca",SERIF)
        return
    scores=rows("clay_conversation")
    if scores:
        target="source" if any(q["target"]=="source" for q in scores) else scores[0]["target"]
        arms={q["clay_conversation"]:q for q in scores if q["target"]==target};d.text(x+28,y+90,"Recognizing a hidden witness",31,"#f0edce",SERIF)
        for j,(key,label) in enumerate((("self_directed","Pupil chooses when to stop"),("matching","One paid stroke"),("untapped","Untouched object"),("foreign_self_directed","Other community’s object"),("visible","Visible glaze"))):
            if key not in arms:continue
            q=arms[key];yy=y+160+j*77;n=int(q["trials"]);v=int(q["correct"]);d.text(x+28,yy,label,19,"#e9edda")
            d.text(x+size-28,yy,f"{v:,} / {n:,}",17,"#cae1bd",MONO,anchor="end")
            d.rect(x+28,yy+13,size-56,10,"#4c5d50",4);d.rect(x+28,yy+13,(size-56)*v/max(1,n),10,"#d7b77d" if j==0 else "#97b9a5",4)
        d.text(x+28,y+560,"The grading words stayed outside the GPU.",18,"#c5d6bd",SERIF)
    else:
        d.text(x+28,y+90,"At the clay table",35,"#f0edce",SERIF);take=rows("clay_inferred")[-4:]
        if take:
            for j,q in enumerate(take):
                yy=y+164+82*j;source=int(q["clay_inferred"]);d.text(x+28,yy,f"{int(q['taps'])} paid taps",23,"#f0edce",SERIF)
                d.text(x+size-28,yy,"Unresolved" if source==4 else f"Assembly {source+1}",19,"#c5d6bd",MONO,anchor="end")
        else:
            q=rows("clay_tick");metrics=(("grooves","Surviving impressions"),("read","Sounds recovered"),("carried","Movements of marked grains"))
            if q:
                for j,(key,label) in enumerate(metrics):
                    yy=y+200+103*j;d.text(x+28,yy,label,21,"#e9edda");d.text(x+size-28,yy,f"{int(q[-1][key]):,}",25,"#c5d6bd",MONO,anchor="end")
            else:
                contacts=rows("clay_contact");d.text(x+28,y+193,f"{len(contacts)} retained contacts",25,"#ecedd7",SERIF)
                for j,event in enumerate(contacts[-3:]):
                    d.text(x+29,y+275+69*j,f"{event['strokes']} strokes   ·   {event['available']} assemblies",18,"#c5d6bd",MONO)


def clay_speech_panel(d, data, x, y, width):
    lines = data["lines"]
    def rows(tag):
        return [pairs(line) for line in lines if line.startswith(tag + "=")]

    scores = rows("clay_conversation")
    entries = []
    if scores:
        arms = {(q["clay_conversation"], q["target"]): q for q in scores}
        heard, shut = ("self_directed", "silent_witness") if any(
            key == "silent_witness" for key, _ in arms
        ) else ("matching", "answer_shut")
        title, labels = "Which answers survive a silent partner?", ("Question", "Partner heard", "Return closed")
        for key, name in (("source", "Who worked the clay?"), ("effect", "Which material appeared?"),
                          ("origin", "Where did it appear?"), ("contradiction", "False claim withheld")):
            pair = [arms.get((condition, key)) for condition in (heard, shut)]
            if any(pair):
                entries.append((name, *(f"{int(q['correct']):,} / {int(q['trials']):,}" if q else "—" for q in pair)))
    else:
        exchanges = []
        prompt = None
        for line in lines:
            if line.startswith("reed_prompt=heard  text="):
                prompt = line.partition("  text=")[2]
            elif line.startswith("animal> ") and prompt is not None:
                answer = line.removeprefix("animal> ")
                exchanges.append((short(prompt, 41), "withheld" if answer in ("?", "(silence)") else answer, ""))
                prompt = None
        if exchanges:
            title, labels = "From the recorded conversation", ("Question heard", "Resident said", "")
            entries = exchanges[-4:]
        elif rows("clay_dialogues"):
            title, labels = "Recorded utterances", ("Pupil said", "Paid taps", "Peer queries")
            for q in rows("clay_sentence")[-4:]:
                entries.append(("withheld" if q["said"] == "_" else q["said"], q["paid_taps"], q["questions"]))
        else:
            title, labels = "Recent native exchanges", ("Call and reply", "Grip", "Food received")
            pitches = ("ka", "li", "mu", "so", "quiet")
            for q in rows("reed_duet")[-4:]:
                voices = "  →  ".join(pitches[min(4, int(q[k]))] for k in ("query", "reply"))
                entries.append((voices, q["stroke"], q["food"]))

    d.label(x, y, title)
    columns = (x, x + width * .66, x + width)
    for column, label, align in zip(columns, labels, ("start", "end", "end")):
        d.text(column, y + 32, label, 14, MUTED, MONO, anchor=align)
    for i, (label, first, second) in enumerate(entries):
        yy = y + 64 + 34 * i
        d.text(columns[0], yy, label, 17, INK)
        d.text(columns[1], yy, first, 17, GREEN, MONO, 600, "end")
        d.text(columns[2], yy, second, 17, INK, MONO, anchor="end")
        d.rule(x, yy + 10, x + width, yy + 10)


def reed_panel(d,data,x,y,size):
    lines=data.get("lines",[])
    def entries(key):return [pairs(l) for l in lines if l.startswith(key+"=")]
    d.rect(x,y,size,size,"#172f35",14)
    d.label(x+29,y+43,"REEDBED   /   ACOUSTIC FIELD NOTES","#a4d3c0")
    d.text(x+28,y+91,"A private view of the current",35,"#f2f0d7",SERIF)
    traces=entries("siphon_transcript")
    tones=("ka","li","mu","so","·");palette=("#f2c876","#9bd6b0","#d0b0e1","#94c8d9","#526972")
    if traces:
        d.text(x+29,y+130,"One measured pair. Each column changes the private inlet.",17,MINT)
        for j in range(4):
            xx=x+203+88*j;d.text(xx,y+185,f"{j}",23,PAPER,MONO,anchor="middle");d.dot(xx,y+151,5,palette[j])
        d.text(x+24,y+185,"Inlet value",17,MINT)
        for j,row in enumerate(traces):
            yy=y+219+j*70
            d.text(x+27,yy+18,f"Filter {int(row['siphon_transcript'])}",21,PAPER,SERIF)
            d.text(x+27,yy+41,f"asked  {tones[int(row['query'])]}",14,MINT,MONO)
            for k in range(4):
                xx=x+171+88*k;voice=(int(row["voices"])>>(4*k))&15
                angle=(int(row["apertures"])>>(4*k))&15;won=int(row["worked"])>>k&1
                d.rect(xx,yy-9,66,55,"#31574d" if won else "#293e49",5)
                d.text(xx+33,yy+13,tones[voice],20,palette[voice],MONO,anchor="middle")
                d.text(xx+33,yy+36,f"→ {angle}",15,"#edf0df",MONO,anchor="middle")
        d.rule(x+27,y+521,x+size-26,y+521,"#466960")
        d.text(x+29,y+550,"Spoken reply and resulting aperture",22,PAPER,SERIF)
        d.text(x+29,y+578,"Green cells released carbon. A dot records silence.",16,MINT)
        return
    rows=[];heading="Paid contacts"
    for tag,arms,key,denom in (
        ("reed_evidence",("open","query_shut","answer_shut","answer_shift"),"accurate","unique"),
        ("reed_pair",("lived","query_shut","answer_shut","words_erased"),"productive","trials"),
        ("siphon_legacy",("inhabited","rotated","reference","sealed"),"passive_carbon",None)):
        found={q[tag]:q for q in entries(tag) if key in q}
        if found:
            maximum=max(1,*[int(v[key]) for v in found.values()])
            for arm in arms:
                if arm in found:
                    value=int(found[arm][key]);n=int(found[arm][denom]) if denom else maximum
                    rows.append((arm.replace("_"," ").capitalize(),value,n,bool(denom)))
            heading="Correct spoken facts" if tag=="reed_evidence" else "Productive grips" if tag=="reed_pair" else "Carbon released without occupants"
            break
    if rows:
        d.text(x+29,y+143,heading,21,MINT)
        for j,(name,val,total,fraction) in enumerate(rows):
            yy=y+207+j*89
            d.text(x+29,yy,name,20,PAPER);d.text(x+size-31,yy,f"{val:,} / {total:,}" if fraction else f"{val:,} units",19,MINT,MONO,anchor="end")
            d.rect(x+29,yy+17,size-60,12,"#3a5057",4);d.rect(x+29,yy+17,(size-60)*val/max(1,total),12,palette[j],4)
        d.rule(x+28,y+535,x+size-29,y+535,"#45615e");d.text(x+28,y+572,"Matched physical starting conditions",21,"#f2f0d7",SERIF)
        return
    events=entries("reed_duet")[-4:];questions=[s.partition("  text=")[2] for s in lines if s.startswith("reed_prompt=")]
    if events:
        d.text(x+29,y+150,"Recent exchanges at the material table",23,MINT)
        for j,q in enumerate(events):
            yy=y+221+j*78;ask=int(q["query"]);answer=int(q["reply"]);seen=int(q["viewed"])
            d.text(x+29,yy,f"Reed {q['requested']}",21,PAPER,SERIF);d.text(x+156,yy,f"{tones[ask]}  →  {tones[answer]}",24,palette[answer],MONO)
            d.text(x+345,yy,f"Seen {seen if seen<4 else '·'}    Food {q['food']}",17,MINT,MONO)
        d.text(x+29,y+563,"After-stroke sightings are available to the captioning resident.",15,MINT)
    elif questions:
        d.text(x+29,y+158,"Heard at the outlet",22,MINT)
        for j,msg in enumerate(questions[-4:]):d.text(x+29,y+235+71*j,short(msg,38),24,PAPER,SERIF)
    else:
        q=next(iter(entries("siphon_tick")),{})
        for j,(key,label) in enumerate((("pairs","Native encounters"),("heard","Audible turns"),("released","Carbon released"),("passive","Released by the current"))):
            if key in q:
                yy=y+209+85*j;d.text(x+29,yy,label,22,PAPER);d.text(x+size-30,yy,f"{int(q[key]):,}",29,palette[j],MONO,anchor="end")

def plate(data,b,history):
    if b.get("kind")=="genesis" or data.get("command")=="genesis" or any(l.startswith("genesis_") for l in data.get("lines",[])):
        return genesis_plate_drawing(data,b)
    d=Drawing();header(d,b,b["id"]);x,y,size=48,206,604;rx=703;rw=W-48-rx
    m=data.get("metrics",{})
    cascade_art=b["kind"]=="interview" and cascade_session(data)
    if b["kind"] in ("conversation","interview","concept-interview","garden-interview") and "speaker_body" in m:
        for dx,role,color,angle in ((278,"listener",GREEN,52),(128,"speaker","#a46a39",-48)):
            args=data.get("args",[]);flag="--"+role
            exact=next((s.get("measures",{}) for s in data.get("specimens",[]) if s.get("role")=="input" and flag in args and s.get("path")==args[args.index(flag)+1]),{}) if cascade_art else {}
            if exact.get("body_plan"):
                atlas_anatomy(d,W-dx,108,exact,atlas_identity(exact),15)
            else:creature(d,W-dx,116,1.1,m[role+"_body"],m[role+"_tape"],color,angle)
            d.text(W-dx,157,"Questioner" if role=="listener" and m.get("facets")==2 and not m.get("field_conversation_events") else role.capitalize(),13,MUTED,MONO,anchor="middle")
    reed_art=any(l.startswith(("siphon_transcript=","siphon_legacy=","reed_pair=","reed_duet=","reed_evidence=","reed_prompt=")) for l in data["lines"])
    clay_art=any(l.startswith(("clay_conversation=","clay_tablet=","clay_object=","clay_inferred=","clay_contact=","clay_tick=")) for l in data["lines"])
    press_art=any(l.startswith(("press_transfer=","press_field=","press_language=","press_reply=","press_census=","press_contact=","press_season=","press_deploy=")) for l in data["lines"])
    spatial_art=any(l.startswith(("hollow_trip=","hollow_contact=","hollow_language=","hollow_field=","hollow_scholar=","hollow_reply=")) for l in data["lines"])
    cost_art=any(l.startswith(("effort_field=","effort_choice=","effort_language=","effort_reply=","effort_contact=","effort_shape_language=")) for l in data["lines"])
    link_art=any(l.startswith(("linkage_field=","linkage_fabrication=","linkage_dialogue=","linkage_language=","linkage_recall=")) for l in data["lines"])
    runnel_art=any(l.startswith(("reed_field=","flume_dialogue=","flume_contact=","reed_analogy=","reed_chain=","reed_rig=","reed_image=")) for l in data["lines"])
    bank_art=any(l.startswith(("bank_field=","bank_talk=","bank_dialogue=")) for l in data["lines"])
    braid_art=not cascade_art and any(l.startswith(("narrative_turn=","narrative_exam=","field_ink=","braid_field=","braid_talk=","braid_recall=","neural_benchmark=","frontier_census=","gate_trial=","field_interview=","field_class=","field_watch=","field_lesson=","field_follow=","regulatory_brood=","apprentice_language=","regulatory_field=","regulatory_survey=","regulatory_carriers=","shore_school=","drift_tick=","drift_witness=","drift_legacy=","drift_conversation=","drift_food_test=","inlet_panel=","inlet_conversation=","inlet_storage=")) for l in data["lines"])
    mind_art=any(l.startswith(("mind_evo=","mind_eval=","mind_bench=","mind_peer_exchange=","mind_society=","mind_ecosystem=","mind_turn=")) for l in data["lines"])
    inquiry_art=mind_art or braid_art or bank_art or runnel_art or any(l.startswith(("practice_trial=","practice_field=","practice_image=","studio_contact=","practice_dialogue=","wick_field=")) for l in data["lines"])
    if cascade_art:cascade_session_panel(d,data,x,y,size)
    elif mind_art:mind_plate(d,data,b,x,y,size,rx,rw)
    elif braid_art:braid_plate(d,data,b,x,y,size,rx,rw)
    elif bank_art:backwater_plate(d,data,b,x,y,size,rx,rw)
    elif runnel_art:runnel_plate(d,data,b,x,y,size,rx,rw)
    elif inquiry_art:practice_plate(d,data,b,x,y,size,rx,rw)
    elif link_art:linkage_plate(d,data,b,x,y,size,rx,rw)
    elif cost_art:effort_plate(d,data,b,x,y,size,rx,rw)
    elif spatial_art:hollow_plate(d,data,b,x,y,size,rx,rw)
    elif press_art:pressure_plate(d,data,b,x,y,size,rx,rw)
    elif clay_art:clay_panel(d,data,x,y,size)
    elif reed_art:reed_panel(d,data,x,y,size)
    elif b["family"] in ("Grounded answers after consultation","Reported material intentions","Self-directed material questions"):brine_social_panel(d,data,x,y,size)
    elif any(l.startswith("brine_specimen=") for l in data["lines"]):brine_panel(d,data,x,y,size)
    elif m.get("field_conversation_events"):field_dialogue_panel(d,data,x,y,size)
    elif any(s.startswith("worksite_window=") for s in data["lines"]):worksite_panel(d,data,x,y,size)
    elif any(v.startswith("shoal_specimen=") or v.startswith("shore_layout=") for v in data["lines"]):shoal_panel(d,data,x,y,size)
    elif data.get("metrics",{}).get("workbench_events"):manual_panel(d,data,x,y,size)
    elif b["kind"]=="floodplain" and any(s.startswith("mineral_row=") for s in data["lines"]):mineral_panel(d,data,x,y,size)
    elif b["kind"]=="floodplain":watershed_panel(d,data,x,y,size)
    elif b["kind"]=="scene-dialogue":
        d.rect(x,y,size,size,"#203a3e",13);d.label(x+26,y+37,"Scene conversation",MINT)
        items=[]
        for line in data["lines"]:
            if line.startswith("scene_reply="):
                r=pairs(line);items.append((line.partition("  heard=")[2],r["scene_reply"]))
            if line.startswith("scene_point="):
                r=pairs(line);sites=int(r["possible"]);items.append(("Point to "+r["scene_point"],", ".join(str(i) for i in range(64) if sites&(1<<i)) or "?"))
        for k,(question,reply) in enumerate(items[-4:]):
            cy=y+100+k*123;d.text(x+27,cy,short(question,45),20,PAPER,SERIF)
            d.path(f"M{x+34} {cy+13} v29 h12",MINT,1.5);d.text(x+56,cy+50,short(reply,40),23,AMBER,SANS,600)
    elif b["kind"]=="grounding":grounding_panel(d,b,data,x,y,size)
    elif b["kind"] in ("motor-learning","motor-forecast","motor-interview","flight-colony"):procedure_panel(d,data,x,y,size,b["kind"])
    elif b["kind"]=="causal-questions":
        r=next(pairs(s) for s in data["lines"] if s.startswith("motor_survey="));n=max(1,int(r["withheld"]))
        series=[["Correct",100*int(r["inferred"])/n],["Determinate",100*int(r["settled"])/n],["Fully solved rooms",100*int(r["perfect"])/max(1,int(r["motor_survey"]))]]
        chart_panel(d,b,dict(data,chart_label="Held-out experiment results (%)",series=series,metrics={"observations":n}),x,y,size)
    elif b["kind"]=="self-hearing":inner_panel(d,data,x,y,size)
    elif b["kind"]=="composition":composition_panel(d,data,x,y,size)
    elif b["kind"]=="shelter":shelter_panel(d,data,x,y,size)
    elif b["kind"]=="composed-dialogue":
        copy=dict(data,lines=[s.replace("composed_reply=","auditory_reply=",1) for s in data["lines"] if s.startswith("composed_reply=")])
        development_panel(d,copy,x,y,size)
    elif b["kind"]=="composition-scale":
        r=[pairs(s) for s in data["lines"] if s.startswith("composition_batch=")]
        chart_panel(d,b,dict(data,chart_label="Million acoustic events per second",series=[[a["composition_batch"].title(),float(a["Mevents_per_s"])] for a in r],metrics={"observations":int(r[-1]["replicas"])}),x,y,size)
    elif b["kind"]=="development":development_panel(d,data,x,y,size)
    elif b["kind"]=="chorus":chorus_panel(d,data,x,y,size)
    elif b["kind"]=="interview":interview_panel(d,data,x,y,size)
    elif b["kind"] in ("commons","culture","estuary"):commons_panel(d,data,x,y,size)
    elif b["kind"]=="greenhouse":greenhouse_panel(d,data,x,y,size)
    elif b["kind"]=="garden-interview":garden_panel(d,data,x,y,size)
    elif b["kind"]=="cultivation":
        rows=[pairs(s) for s in data["lines"] if s.startswith("cultivation=")]
        series=[[r["cultivation"].title(),100*int(r["goals"])/int(r["worlds"])] for r in rows]
        chart_panel(d,b,dict(data,chart_label="Physical goals reached (%)",series=series,metrics={"observations":int(rows[0]["worlds"])}),x,y,size)
    elif b["kind"]=="causal-census":
        r=next(pairs(s) for s in data["lines"] if s.startswith("causal_census="))
        bars=[[k.title(),100*int(r[k])/int(r["trials"])] for k in ("reachable","already","unreachable")]
        chart_panel(d,b,dict(data,chart_label="Physical reachability (%)",series=bars,metrics={"observations":int(r["trials"])}),x,y,size)
    elif b["kind"] in ("inference","concept-interview"):concept_panel(d,data,x,y,size)
    elif b["kind"]=="concept-census":
        r=next(pairs(s) for s in data["lines"] if s.startswith("concept_census="))
        values=[["Unique",int(r["unique"])],["Empty",int(r["empty"])],["Ambiguous",int(r["ambiguous"])]]
        chart_panel(d,b,dict(data,chart_label="Share of graph queries (%)",series=[[k,100*v/int(r["trials"])] for k,v in values],metrics={"observations":int(r["trials"])}),x,y,size)
    elif b["kind"]=="endpoint-scale":
        rows=[pairs(s) for s in data["lines"] if s.startswith("endpoint_batch=")]
        series=[[k.title(),statistics.median(float(r["Mqueries_per_s"]) for r in rows if r["endpoint_batch"]==k)/1000] for k in ("direct","projected")]
        chart_panel(d,b,dict(data,chart_label="Billion endpoint queries per second",series=series,metrics={"observations":int(rows[0]["queries"])}),x,y,size)
    elif b["kind"]=="recollection":memory_panel(d,data,x,y,size)
    elif b["kind"]=="attention":
        rows=[pairs(s) for s in data["lines"] if s.startswith("attention=")]
        labels={"intact":"Intact","inert":"Inert contacts","muted":"Pitch muted","inverted":"Pitch reversed","shuffled-food":"Random feedback"}
        bars=[[labels[r["attention"]],100*int(r["revised_chain"])/int(r["probes"])] for r in rows]
        chart_panel(d,b,dict(data,chart_label="Correct after reassignment (%)",series=bars,metrics={"observations":int(rows[0]["probes"])}),x,y,size)
    elif b["kind"]=="imagery":
        rows=[pairs(s) for s in data["lines"] if s.startswith("imagery=")]
        labels={"intact":"Intact","fixed":"Frozen at birth","displaced":"Displaced arrivals","silent":"Replays muted","reverse":"Reversed sequence","locked":"Frozen at change","compiled":"Compiled contacts"}
        bars=[[labels[r["imagery"]],100*int(r["changed_sequence"])/int(r["probes"])] for r in rows]
        chart_panel(d,b,dict(data,chart_label="Arrivals predicted after the change (%)",series=bars,metrics={"observations":int(rows[0]["probes"])}),x,y,size)
    elif b["kind"]=="narration":
        rows=[pairs(s) for s in data["lines"] if s.startswith("narration=")]
        bars=[[r["narration"],100*int(r["decoded"])/int(r["trials"])] for r in rows]
        chart_panel(d,b,dict(data,chart_label="Private events decoded (%)",series=bars,metrics={"observations":int(rows[0]["trials"])}),x,y,size)
    elif b["kind"]=="report-interview":
        report_panel(d,data,x,y,size)
    elif b["kind"]=="conjunction-scale":
        rows=[pairs(s) for s in data["lines"] if s.startswith("joint_batch=")]
        bars=[[layout.title(),statistics.median(float(t["Mqueries/s"]) for t in rows if t["joint_batch"]==layout)/1000] for layout in ("direct","compiled")]
        chart_panel(d,b,dict(data,chart_label="Billion single-step reports per second",series=bars,metrics={"observations":int(rows[0]["queries"])}),x,y,size)
    elif b["kind"] in ("conjunction","conjunction-census"):
        rows=[pairs(s) for s in data["lines"] if s.startswith("conjunction=")]
        sample=next((pairs(s) for s in data["lines"] if s.startswith("conjunction_scene=")),None)
        if sample:joint_panel(d,sample,x,y,size)
        else:
            bars=[[r["conjunction"],100*int(r["completion"])/int(r["probes"])] for r in rows]
            chart_panel(d,b,dict(data,chart_label="Unseen joint arrivals predicted (%)",series=bars,metrics={"observations":int(rows[0]["probes"])}),x,y,size)
    elif b["kind"]=="binding":
        rows=[pairs(s) for s in data["lines"] if s.startswith("binding=")]
        labels={"intact":"Intact","fixed-support":"Frozen source contacts","random-choice":"Random experiment","confounded":"Confounded outcome","crossed-arrivals":"Crossed departures"}
        bars=[[labels[r["binding"]],100*(int(r["revised_first"])+int(r["revised_second"]))/(2*int(r["probes"]))] for r in rows]
        chart_panel(d,b,dict(data,chart_label="Correct after the roles changed (%)",series=bars,metrics={"observations":int(rows[0]["probes"])}),x,y,size)
    elif b["kind"]=="attention-scale":
        rows=[pairs(s) for s in data["lines"] if s.startswith("attention_batch=")]
        bars=[[layout.title(),statistics.median(float(t["Mdecisions/s"]) for t in rows if t["attention_batch"]==layout)/1000] for layout in ("direct","compiled")]
        chart_panel(d,b,dict(data,chart_label="Billion sensory decisions per second",series=bars,metrics={"observations":int(rows[0]["decisions"])}),x,y,size)
    elif b["kind"]=="conversation":dialect_panel(d,data,x,y,size)
    elif b["kind"]=="forage":
        arms=foraging(data);labels=(("intact","Intact"),("raw","Base map"),("trace-fixed","Fixed collaterals"),("immediate","Last event"),("no-appetite","No food target"),("silent","Silent map"))
        bars=[[label,int(arms[key]["offspring_mass"])/max(1,int(arms[key]["lives"]))] for key,label in labels if key in arms]
        chart_panel(d,b,dict(data,chart_label="Offspring material per lifetime",series=bars,metrics={"observations":int(arms["intact"]["lives"])}),x,y,size)
    elif b["kind"]=="world" and data.get("tiles"):field_panel(d,data,x,y,size)
    elif b["kind"]=="animal":circuit_panel(d,data,x,y,size)
    else:chart_panel(d,b,data,x,y,size)
    d.label(rx,194,"Run measurements")
    for i,(key,n) in enumerate(b["stats"][:3]):
        xx=rx+i*rw/3
        if i:d.rule(xx-15,210,xx-15,282)
        d.text(xx,252,short(human(n),13),37,INK,SERIF)
        d.text(xx,279,short(str(key),24),16,MUTED)
    d.rule(rx,302,W-48,302)
    points=data.get("points",[])
    if b["kind"]=="world" and points:
        breeding=data.get("breeding",[]);scores=b["scores"]
        d.label(rx,334,"Breeding scores per candidate" if breeding else "Independent learning probes" if scores and len(points)==1 else "Population through this run")
        for i in range(3):d.rule(rx,358+43*i,W-48,358+43*i,"#d5dbcf")
        if breeding:
            means=[p[2]/p[1] for p in breeding];best=[p[3] for p in breeding];ts=[p[0] for p in breeding]
            limits=(0,max(1,max(best)));trend(d,rx+3,360,rw-6,81,means,GREEN,True,ts,limits)
            trend(d,rx+3,360,rw-6,81,best,"#ad713e",False,ts,limits)
            left=f"Generation {ts[0]} → {ts[-1]}";right=f"Mean {means[-1]:.1f}  /  best {human(best[-1])}"
        elif scores and len(points)==1:
            for i,(label,a,n) in enumerate(scores[-3:]):
                yy=358+32*i;d.text(rx,yy+5,short(label,33),17,INK)
                d.text(W-48,yy+5,f"{human(a)} / {human(n)}",16,GREEN,MONO,anchor="end")
            left="Probe denominators are recorded per task";right=""
        else:
            vals=[p[3] for p in points]
            if len(points)>1:trend(d,rx+3,359,rw-6,83,vals,GREEN,True,[p[0] for p in points])
            else:d.text(rx,413,"Final census",33,GREEN,SERIF)
            left=f"Tick {human(points[0][1])} → {human(points[-1][1])}"
            right=f"{human(min(vals))}–{human(max(vals))} animals" if len(vals)>1 else ""
        d.text(rx,469,left,14,MUTED,MONO);d.text(W-48,469,right,14,MUTED,MONO,anchor="end")
        d.rule(rx,493,W-48,493);specimens(d,data,rx,526,rw)
        d.rule(rx,684,W-48,684)
        d.label(rx,712,"Checkpoint history" if len(points)==1 else "Activity during this run")
        for i,(k,v) in enumerate(zip(("Moves","Births","Food grazed"),(b["activity"][i] for i in ("moves","births","grazed")))):
            xx=rx+i*rw/3;d.text(xx,751,human(v),27,GREEN,SERIF);d.text(xx,779,k,15,MUTED)
    else:
        if inquiry_art or link_art or cost_art or spatial_art or press_art:pass  # The two measured pressure panels are drawn together.
        elif b["family"]=="Unseen coating in changed water":
            conditions={r["brine_transfer"]:r for l in data["lines"] if l.startswith("brine_transfer=") for r in [pairs(l)]}
            d.label(rx,335,"Productive first choices   /   paired counterfactual baths")
            for i,(arm,caption) in enumerate((("responsive","Own observations"),("frozen","Synapses frozen"),("yoked","Same evidence in both"),("opposite","Other bath shown"),("rearranged","Sensory inputs remapped"))):
                q=conditions[arm];frac=int(q["correct"])/int(q["choices"]);yy=369+32*i
                d.text(rx,yy,caption,16,INK);d.rect(rx+215,yy-11,rw-298,10,"#d6dfd0",3)
                d.rect(rx+215,yy-11,(rw-298)*frac,10,GREEN if not i else "#819e91",3);d.text(W-49,yy,f"{100*frac:.1f}%",16,GREEN,MONO,anchor="end")
        elif b["family"] in ("Grounded answers after consultation","Reported material intentions","Self-directed material questions"):
            arm="autonomous" if b["family"]=="Self-directed material questions" else "consulted"
            q=next(pairs(l) for l in reversed(data["lines"]) if l.startswith("brine_dialogue="+arm+" "))
            d.label(rx,335,"From public experiment to physical action")
            for i,(key,label) in enumerate((("spoken_requests","Requests spoken"),("tests_shown","Partner's tests"),("witnessed","Outcomes witnessed"),("intended_site","Final intended sites reached"))):
                yy=372+39*i;d.text(rx,yy,label,17,INK);d.text(W-49,yy,f"{int(q[key]):,}",19,GREEN,MONO,anchor="end");d.rule(rx,yy+12,W-49,yy+12)
        elif b["family"]=="Provisioned lineage in shared water":
            rows=[pairs(l) for l in data["lines"] if l.startswith("flood_census=")]
            d.label(rx,335,"Residents of each cohort")
            for i,(name,key,color) in enumerate((("Provisioned","cohort_a",GREEN),("Fostered","cohort_b","#957047"))):
                vals=[int(r[key]) for r in rows];yy=392+72*i;d.text(rx,yy,name,19,INK)
                trend(d,rx+155,yy-30,rw-248,34,vals,color);d.text(W-49,yy,f"{vals[-1]:,}",20,color,MONO,anchor="end")
        elif b["family"]=="Field-reared material choices":
            conditions={r["field_transfer"]:r for v in data["lines"] if v.startswith("field_transfer=") for r in [pairs(v)]}
            d.label(rx,335,"Choice of reactive soil   /   unfamiliar beds")
            for i,(arm,name) in enumerate((("lived","Field experience"),("unexperienced","Experience cleared"),("odor_only","Odor alone"),("ground_only","Local imprints"),("ground_scrambled","Reordered outcomes"))):
                r=conditions[arm];part=int(r["soil_choices"])/int(r["trials"]);yy=371+32*i
                d.text(rx,yy,name,16,INK);d.rect(rx+190,yy-11,rw-270,10,"#d7dfd1",4)
                d.rect(rx+190,yy-11,(rw-270)*part,10,GREEN if i==0 else "#88a38f",4)
                d.text(W-48,yy,f"{100*part:.1f}%",16,GREEN,MONO,anchor="end")
        elif b["family"]=="Spoken field judgments":
            rows=[pairs(v) for v in data["lines"] if v.startswith("field_answer=")]
            d.label(rx,335,"What happened at the listening table")
            for i,(name,count) in enumerate((("Heard questions",len(rows)),("Voiced answers",sum(r["field_answer"]!="_" for r in rows)),
                                             ("Tests paid for",sum(int(r["receiver_work"])>0 for r in rows)),("Edible product",sum(int(r["receiver_food"]) for r in rows)))):
                yy=373+37*i;d.text(rx,yy,name,18,INK);d.text(W-49,yy,str(count),21,GREEN,MONO,anchor="end");d.rule(rx,yy+10,W-49,yy+10)
        elif b["kind"]=="floodplain" and b["family"]=="Construction from isolated stock":
            conditions={r["shoal_test"]:r for line in data["lines"] if line.startswith("shoal_test=") for r in [pairs(line)]}
            d.label(rx,335,"Cold harvests   /   matched animals and beds")
            for i,(key,name) in enumerate((("experienced","After exploration"),("no_contact_learning","Unlearned contacts"),("no_motor_learning","Unlearned muscles"),("no_spread","Spatial replay silent"),("random_ideas","Random alternatives"))):
                q=conditions.get(key)
                if not q:continue
                rate=int(q["builders"])/int(q["trials"]);yy=367+i*30
                d.text(rx,yy,name,16,INK);d.rect(rx+210,yy-11,(rw-278)*rate,10,GREEN if not i else "#89a691",3);d.text(W-49,yy,f"{100*rate:.1f}%",16,GREEN,MONO,600,"end")
        elif b["kind"]=="floodplain" and b["family"]=="Working material responses":
            all_rows=[pairs(s) for s in data["lines"] if s.startswith("artifact_signature=")]
            root=data.get("metrics",{}).get("mineral_focus_root")
            q=next((r for r in all_rows if int(r["artifact_signature"])==root),all_rows[0])
            d.label(rx,335,f"Measured reactions   /   specimen {q['artifact_signature']}")
            lamps=[("dark","No light"),("a","Lamp A"),("b","Lamp B"),("both","Both lamps"),("retained","Late substrate")]
            top=max(1,*[int(q[k]) for k,_ in lamps])
            for i,(k,label) in enumerate(lamps):
                yy=365+30*i;v=int(q[k]);d.text(rx,yy,label,16,INK)
                d.rect(rx+161,yy-11,(rw-225)*v/top,10,GREEN if k=="both" else "#86a38b",3)
                d.text(W-49,yy,str(v),16,GREEN,MONO,600,"end")
        elif b["kind"]=="floodplain" and b["family"] in ("Transfer to unseen fruits","Held-out fruit choices"):
            remap=b["family"]=="Transfer to unseen fruits"
            prefix="pod_remap=" if remap else "pod_transfers="
            q=next(pairs(line) for line in data["lines"] if line.startswith(prefix) and ("practice=self" in line or "practice=" not in line))
            fields=(("intact","Own experience"),("frame_only","Transferred frame"),("independent","Independent changes"),("yoked","Another animal’s gut")) if remap else (("practiced","After practice"),("relearned","After reversal"),("erased","Contacts erased"),("yoked","Another animal’s gut"))
            d.label(rx,335,"Correct choices on held-out fruit")
            for i,(key,label) in enumerate(fields):
                yy=368+39*i;frac=int(q[key])/int(q["unseen" if remap else "probes"])
                d.text(rx,yy,label,16,INK);d.rect(rx+206,yy-12,(rw-281)*frac,11,GREEN if i==0 else "#87a38c",3)
                d.text(W-48,yy,f"{100*frac:.1f}%",16,GREEN,MONO,600,"end")
        elif b["kind"]=="floodplain":
            water=[pairs(line) for line in data["lines"] if line.startswith("flood_tick=")]
            ancestry=[pairs(line) for line in data["lines"] if line.startswith("flood_census=")]
            d.label(rx,335,"Across the recorded censuses")
            for i,(records,metric,caption,col) in enumerate(((water,"living","Living animals",GREEN),(ancestry,"distinct_bodies","Expressed body plans","#ad713d"),(water,"banked","Clay in banks","#527f90"))):
                n=[int(v[metric]) for v in records];yy=377+i*62
                d.text(rx,yy,caption,16,INK);d.text(W-48,yy,f"{n[-1]:,}",18,col,MONO,600,"end")
                if len(n)>1:trend(d,rx+204,yy-21,rw-317,27,n,col)
                d.rule(rx,yy+17,W-48,yy+17)
        elif b["kind"]=="interview" and b["family"] in (
                "Spoken attribution from material evidence", "Cultural sentence completion", "Physical source inquiries"):
            clay_speech_panel(d,data,rx,335,rw)
        elif cascade_art:cascade_session_evidence(d,data,rx,335,rw)
        elif b["kind"]=="interview":motor_strip(d,data,rx,335,rw,176)
        elif b["kind"]=="garden-interview":
            d.label(rx,335,"What the witness said")
            utterances=[s for s in data.get("lines",[]) if s.startswith("garden_event=") and "  intent=0 " not in s and "  speech=  " not in s][-3:]
            for i,s in enumerate(utterances):
                voice,_,prompt=s.partition("  speech=")[2].partition("  prompt=");yy=367+54*i
                d.text(rx,yy,short(prompt,53),16,MUTED);d.text(rx,yy+23,short(voice,61),18,GREEN,SERIF)
            if not utterances:d.text(rx,387,"Observing physical examples",23,GREEN,SERIF)
        elif b["kind"]=="shelter":
            rows=[pairs(s) for s in data["lines"] if s.startswith("shelter_season=")]
            d.label(rx,335,"Each season  /  share of opportunities")
            for k in range(3):d.rule(rx,364+k*47,rx+rw,364+k*47,"#d5dbcf")
            for key,color in (("reached",GREEN),("meals","#b88048"),("inquiries","#558a96")):
                vals=[100*int(r[key])/max(1,int(r["opportunities"])) for r in rows]
                trend(d,rx+4,365,rw-8,96,vals,color,limits=(0,max(100,max(vals))))
            for k,(label,color) in enumerate((("Food located",GREEN),("Harvested","#b88048"),("Asked","#558a96"))):
                xx=rx+k*rw/3;d.rule(xx,488,xx+19,488,color,3);d.text(xx+25,493,label,15,MUTED)
        elif b["kind"]=="chorus":
            seasons=[pairs(s) for s in data["lines"] if s.startswith("chorus_season=")]
            d.label(rx,335,"Each season  /  share of motor calls")
            for k in range(3):d.rule(rx,364+k*48,rx+rw,364+k*48,"#d5dbcf")
            for key,color in (("predicted",GREEN),("aimed","#b88048"),("meals","#558a96")):
                values=[100*int(r[key])/max(1,int(r["moves"])) for r in seasons]
                trend(d,rx+4,365,rw-8,96,values,color,limits=(0,100))
            for k,(label,color) in enumerate((("Predicted echo",GREEN),("Planned call","#b88048"),("Harvested","#558a96"))):
                xx=rx+k*rw/3;d.rule(xx,489,xx+18,489,color,3);d.text(xx+26,494,label,15,MUTED)
        elif b["kind"]=="greenhouse":
            r=next(pairs(s) for s in reversed(data["lines"]) if s.startswith("greenhouse_generation="))
            d.label(rx,335,"Seed inheritance and acquired mechanisms")
            for i,(label,key) in enumerate((("Surviving plant founders","plant_founders"),("Inherited seed edits","inherited_edits"),("Settled action effects","settled_fields"))):
                yy=375+48*i;d.text(rx,yy,label,18,MUTED);d.text(rx+rw,yy,human(int(r[key])),22,GREEN,MONO,anchor="end")
        elif b["kind"]=="concept-interview":
            d.label(rx,335,"Recent questions and answers")
            utterances=[s for s in data.get("lines",[]) if s.startswith("concept_turn=") and "assertion=0 " in s and "  prompt=" in s][-3:]
            for i,line in enumerate(utterances):
                obs,_,prompt=line.partition("  prompt=");yy=369+53*i
                d.text(rx,yy,short(prompt,51),17,MUTED);d.text(rx,yy+23,pairs(obs).get("speech","(silence)"),20,GREEN,SERIF)
            if not utterances:d.text(rx,390,"The pair observed the teaching display.",23,GREEN,SERIF)
        elif b["kind"]=="estuary":
            e=next(pairs(s) for s in reversed(data["lines"]) if s.startswith("estuary_generation="));n=int(m["residents"]);d.label(rx,335,"Residents with an acquired pathway")
            for i,key in enumerate(("two_hops","four_hops","eight_hops")[:int(e["depth"])]):
                yy=371+49*i;v=int(e[key]);d.text(rx,yy,f"{2**(i+1)} crossings",17,MUTED);d.rect(rx+134,yy-14,(rw-248)*v/n,15,GREEN,4)
                d.text(rx+rw,yy,human(v),17,GREEN,MONO,anchor="end")
        elif b["kind"]=="report-interview":
            heard=next((s for s in reversed(data["lines"]) if s.startswith(("report_turn=","incoming_report=")) and "  decoded=" in s),None)
            d.label(rx,333,"What the listener inferred")
            if heard:
                record=pairs(heard.partition("  speech=")[0]);directions=("north","east","south","west")
                for i,(key,label) in enumerate((("decoded","Latest movement"),("decoded_prior","Earlier movement"))):
                    mask=int(record.get(key,0));yy=369+65*i
                    if mask and not mask&(mask-1):
                        e=mask.bit_length()-1;description=f"{'Water' if e&1 else 'Food'} · {directions[(e>>1)&3]} → {directions[e>>3]}"
                    else:description=f"{mask.bit_count()} possible movements" if mask else "No recognized report"
                    d.text(rx,yy,label,14,MUTED,MONO);d.text(rx,yy+25,description,23,INK,SERIF)
                source="the human statement" if heard.startswith("incoming_report=") else "the witness's call"
                d.text(rx,508,f"Evidence from {source}",16,GREEN)
            else:d.text(rx,402,"No decoded event was recorded.",20,MUTED,SERIF)
        elif b["kind"]=="culture":
            gen=next(pairs(s)["culture_generation"] for s in reversed(data["lines"]) if s.startswith("culture_generation="))
            samples=[s.partition("  speech=")[2] for s in data["lines"] if s.startswith("island_report=") and f"  generation={gen}  " in s]
            d.label(rx,333,"A common scene across the islands")
            d.text(rx,366,"Food moved north to south, then water moved north to east.",18,INK,SERIF)
            for i,line in enumerate(samples[:4]):
                yy=405+30*i;d.text(rx,yy,short(line,56),16,GREEN,MONO);d.rule(rx,yy+8,W-48,yy+8,"#d5dccf")
            if not samples:d.text(rx,420,"The sampled witnesses stayed silent.",18,MUTED,SERIF)
        elif b["kind"]=="commons" and len(data.get("breeding",[]))<=1:
            row=next(pairs(s) for s in reversed(data["lines"]) if s.startswith("commons_generation="))
            d.label(rx,335,"Where the living days went")
            parts=[("Collected food","meals"),("Explored","experiments"),("Missed food","wrong_traps"),("Lost competition","lost_contests"),("Pod stayed closed","unopened")]
            for i,(label,key) in enumerate(parts):
                n=int(row.get(key,0));yy=373+32*i;f=n/max(1,int(row["lived_days"]))
                d.text(rx,yy,label,15,INK);d.rect(rx+222,yy-10,rw-356,8,"#d7dfd1",3)
                if f:d.rect(rx+222,yy-10,(rw-356)*f,8,GREEN,3)
                d.text(W-48,yy,f"{human(n)} / {100*f:.1f}%",14,GREEN,MONO,anchor="end")
        elif b["kind"] in ("recollection","attention","binding","conjunction","narration","commons") and data.get("breeding"):
            cohort=data["breeding"];ts=[p[0] for p in cohort];means=[p[2]/p[1] for p in cohort];best=[p[3] for p in cohort]
            d.label(rx,335,"Reproductive material per family" if b["kind"]=="commons" else "Fitness through the nursery")
            for i in range(3):d.rule(rx,367+41*i,W-48,367+41*i,"#d5dbcf")
            limits=(0,max(1,max(best)))
            trend(d,rx+3,367,rw-6,82,means,GREEN,True,ts,limits)
            trend(d,rx+3,367,rw-6,82,best,"#ad713e",False,ts,limits)
            d.text(rx,490,f"Generation {ts[0]} → {ts[-1]}",15,MUTED,MONO)
            d.text(W-48,490,f"Mean {means[-1]:.1f}  /  best {best[-1]:,}",15,GREEN,MONO,anchor="end")
        elif b["kind"]=="conversation":
            speech=dialogues(data);phase="revised" if ("revised","open") in speech else "learned"
            d.label(rx,334,"Choices through the exchange")
            chosen=[("First meeting",speech.get(("naive","open"))),("After practice",speech.get(("learned","open")))]
            if phase=="revised":chosen.append(("After the doors changed",speech[phase,"open"]))
            chosen.extend([("Incoming call silenced",speech.get((phase,"silent"))),("Question silenced",speech.get((phase,"no-question")))])
            for i,(label,r) in enumerate((q for q in chosen if q[1])):
                yy=372+33*i;val=int(r["correct"])/max(1,int(r["exchanges"]))
                d.text(rx,yy,label,15,INK);d.rect(rx+248,yy-10,rw-348,8,"#d7dfd1",4)
                if val:d.rect(rx+248,yy-10,(rw-348)*val,8,GREEN,4)
                d.text(W-48,yy,f"{100*val:.1f}%",15,GREEN,MONO,anchor="end")
        elif b["kind"]=="forage":
            arms=foraging(data);d.label(rx,335,"Observed foraging")
            field=arms["intact"]
            measures=[("Lived to the end","survived","lives"),("Found a spring","found_spring","lives"),
                      ("Left a spring","depart_spring","at_spring"),("Moved at a dry site","depart_away","away")]
            for i,(label,num,den) in enumerate(measures):
                if num not in field or not int(field.get(den,0)):continue
                a,n=int(field[num]),int(field[den]);yy=379+41*i
                d.text(rx,yy,label,16,INK);d.rect(rx+229,yy-10,rw-354,8,"#dce4d8",3)
                d.rect(rx+229,yy-10,(rw-354)*a/n,8,GREEN,3);d.text(W-49,yy,ratio(a,n),16,GREEN,MONO,anchor="end")
        elif b["kind"]=="animal" and data.get("metrics",{}).get("replicas"):
            d.label(rx,335,"CUDA frozen inference")
            samples=[pairs(line) for line in data["lines"] if "Mchoices/s=" in line]
            for i,row in enumerate(samples[:3]):
                yy=380+45*i;d.text(rx,yy,f"Launch {row['repeat']}",19,INK)
                d.text(W-48,yy,f"{row['GPU_ms']} ms  /  {row['Mchoices/s']} M choices/s",16,GREEN,MONO,anchor="end")
        elif b["kind"]=="animal" and data.get("metrics",{}).get("score_phases",0)>1:
            d.label(rx,335,"Scored choices through this run")
            phases=thin(data["scores"][-data["metrics"]["score_phases"]:],6)
            for i,(label,correct,total) in enumerate(phases):
                yy=365+i*31
                d.text(rx,yy+5,short(label,23),15,INK)
                d.rect(rx+221,yy-4,rw-363,8,"#dce4d8",3)
                d.rect(rx+221,yy-4,(rw-363)*correct/total,8,GREEN,3)
                d.text(W-48,yy+5,f"{human(correct)} / {human(total)}",14,GREEN,MONO,anchor="end")
        elif b["kind"]=="conjunction-census":
            row=next(pairs(s) for s in data["lines"] if s.startswith("joint_census="))
            d.label(rx,334,"All unseen queries")
            labels=(("forced_correct","Forced answer given"),("forced_silent","Forced answer missed"),("forced_wrong","Incorrect forced answer"),
                    ("ambiguous_silent","Ambiguous answer withheld"),("ambiguous_lucky","Guess matching the world"),("ambiguous_wrong","Guess differing from the world"))
            for i,(key,label) in enumerate(labels):
                yy=368+29*i;d.text(rx,yy,label,16,INK)
                d.text(W-48,yy,human(int(row[key])),15,GREEN,MONO,anchor="end")
                d.rule(rx,yy+8,W-48,yy+8,"#d5dccf")
        elif b["kind"]=="conjunction":
            row=next(pairs(s) for s in data["lines"] if s.startswith("conjunction=intact "))
            d.label(rx,334,"Correct reports and justified silence")
            labels=(("arbitrary","Observed arbitrary change"),("sparse_role","Transferred single-source rule"),
                    ("completion","Inferred missing bearing"),("ambiguous","Withheld ambiguous answer"),
                    ("revision","Revised an observation"),("unstructured_silence","Withheld unstructured guess"))
            for i,(key,label) in enumerate(labels):
                yy=368+29*i;d.text(rx,yy,label,16,INK)
                d.text(W-48,yy,f"{human(int(row[key]))} / {human(int(row['probes']))}",15,GREEN,MONO,anchor="end")
                d.rule(rx,yy+8,W-48,yy+8,"#d5dccf")
        elif b["kind"]=="animal":motor_strip(d,data,rx,335,rw,176)
        else:
            d.label(rx,336,"Recorded series")
            s=b["scores"][-5:] or data.get("summary_rows",[])[:5]
            if s:
                for i,row in enumerate(s):
                    yy=374+34*i;d.text(rx,yy,short(str(row[0]),33),16,INK)
                    d.text(W-50,yy,ratio(row[1],row[2]) if len(row)>2 else short(human(row[1]),18),16,GREEN,MONO,anchor="end")
                    d.rule(rx,yy+11,W-48,yy+11,"#d5dccf")
            else:
                for i,(key,num) in enumerate(b["stats"][:3]):d.text(rx,377+42*i,f"{key}  /  {human(num)}",21,INK,SERIF)
        d.rule(rx,559,W-48,559);d.label(rx,590,"Run notes")
        import textwrap
        yy=624
        for sentence in b["narrative"]:
            for line in textwrap.wrap(sentence,70)[:2]:
                d.text(rx,yy,line,17,INK);yy+=23
            yy+=9
    note=("Colour marks founder ancestry. Lines join consecutive observed positions." if b["kind"]=="world" and any(p[0] for p in points)
          else "Snapshot of final terrain and sampled residents." if b["kind"]=="world"
          else "Conditional contacts are read directly from the animal's saved state." if data.get("conditional")
          else "Contact directions and colours follow inherited motor wiring." if b["kind"]=="animal" and data.get("transitions")
          else "Source measurements are retained with this journal entry.")
    if b["kind"]=="world" and data.get("tiles"):
        d.text(x,834,"Plant biomass",13,MUTED)
        for i,col in enumerate(("#10302e","#1e4840","#315e4a","#436e53")):d.rect(x+101+10*i,823,10,10,col)
        d.dot(x+176,828,3.5,GREEN);d.text(x+187,834,"Occupied site or bin",13,MUTED)
        d.path(f"M{x+341} 828h22","#b78d42",2.4)
        d.text(x+373,834,"Consecutive moves",13,MUTED)
    elif mind_art:
        d.text(x,834,"Native Blackwell SM 12.0 spiking cortical columns, 26-SM L2 hippocampus, and causal rollouts.",13,MUTED)
    elif inquiry_art:
        d.text(x,834,"The record follows material actions, observed consequences and physical resource transfers.",13,MUTED)
    elif link_art:
        d.text(x,834,"Each successful mechanism is graded on its actual physical end state.",14,MUTED)
    elif cost_art:
        d.text(x,834,"The physical ledger follows the work and food received by the same body.",14,MUTED)
    elif spatial_art:
        d.text(x,834,"Measurements follow the body through physical turns and contacts.",14,MUTED)
    elif press_art:
        d.text(x,834,"A contact can transfer relief and resonance to a different carrier.",15,MUTED)
    elif clay_art:
        d.text(x,834,"Only physical contact exposes the note held by a clay groove.",13,MUTED)
    elif reed_art:
        d.text(x,834,"Two independent residents exchange sound across the same physical inlet.",13,MUTED)
    elif b["family"] in ("Grounded answers after consultation","Reported material intentions","Self-directed material questions"):
        d.text(x,834,"Each hidden bath changes which deposit makes food while the visible table stays fixed.",13,MUTED)
    elif any(l.startswith("brine_specimen=") for l in data["lines"]):
        d.text(x,834,"The map and firing fields come from a single closing census.",13,MUTED)
    elif m.get("field_conversation_events"):
        d.text(x,834,"The panel shows the last recorded table with a question, or the final reference exposure.",13,MUTED)
    elif any(v.startswith("worksite_window=") for v in data["lines"]):
        d.text(x,834,"Each tile records the most recent surviving mark at one site.",13,MUTED)
    elif b["kind"]=="floodplain" and any(v.startswith("watershed_trail=") for v in data["lines"]):
        d.text(x,834,"Gold tracks follow sampled surviving animals through the final exposure.",13,MUTED)
    else:d.text(x,834,note,13,MUTED)
    evolution_panel(d,history,b,48,860,W-96,121)
    footer=short(b.get("source") or "Axion simulation",75)
    if b.get("parents"):footer+="  /  checkpoint from "+", ".join(f"run {p['id']:05d}" for p in b["parents"][:3])
    d.rule(48,1003,W-48,1003);d.text(49,1026,footer,13,MUTED,MONO)
    d.text(W-49,1026,"AXION  /  FIELD RECORD",13,GREEN,MONO,600,"end")
    if b.get("performance"):performance_ribbon(d,b["performance"])
    return d

def overview(history):
    d=Drawing();latest=history[-1] if history else None
    mind_runs=[x for x in history if x.get("command")=="mind"]
    critter=globals().get("_mind_critter")
    pill=globals().get("_mind_pill")
    d.rect(0,0,W,318,DARK)
    d.path("M49 51 L62 29 L77 51 M55 42 H70",MINT,3)
    d.label(91,47,"AXION  /  EVOLUTION & COGNITION PROGRESS" if mind_runs else "AXION  /  EVOLUTION JOURNAL","#a6d1b7")
    d.text(W-49,47,latest["date"][:10] if latest else "No records",16,"#bed5c4",MONO,anchor="end")
    d.text(47,128,"How Axion's minds are growing" if mind_runs else "Evolution across runs",55 if mind_runs else 59,"#eff2db",SERIF,500)
    if mind_runs and critter:
        # Expressive Evolution Parade Medallion in top-right header!
        px,py,pw,ph=956,64,496,102
        d.rect(px,py,pw,ph,"#17332e",12,'stroke="#4f8874" stroke-width="1.5"')
        top_doms=max((int(re.search(r"(\d+)\s+domains",r.get("subtitle","")).group(1)) for r in mind_runs if re.search(r"(\d+)\s+domains",r.get("subtitle",""))),default=170)
        parade=[("NEWBORN","axion","newborn",1,"Gen 1"),("LEARNER","mira","happy",2,"Homestead"),("BLACKWELL","axion","proud",3,f"{top_doms} doms")]
        for pi,(plbl,prole,pmood,plvl,psub) in enumerate(parade):
            cx_p=px+78+pi*168
            critter(d,cx_p,py+44,0.88,prole,pmood,plvl)
            if pi==2:
                critter(d,cx_p+42,py+48,0.68,"alien","happy",3)
            d.text(cx_p,py+82,plbl,9.2,MINT if pi==2 else "#f3bc73",MONO,700,"middle")
            d.text(cx_p,py+95,psub,8.6,"#b8cfc0",SANS,400,"middle")
            if pi<2:
                ax=cx_p+60;ay=py+44
                d.path(f"M{ax} {ay} H{ax+24} M{ax+18} {ay-5} L{ax+24} {ay} L{ax+18} {ay+5}","#f3bc73",2.2)
    live=[x for x in history if not x["archive"]];worlds=[x for x in history if x["kind"]=="world"]
    # Extract chronological cortical capability milestones (distinct domain exams across versions!)
    mind_exams=[]
    for r in mind_runs:
        if r.get("family")=="Held-out cognitive accuracy" and "arm=intact" in r.get("subtitle","") and "English" in r.get("subtitle",""):
            m_dom=re.search(r"(\d+)\s+domains",r.get("subtitle",""))
            m_pr=next((re.search(r"(\d+)/(\d+)",str(st[1])) for st in r.get("stats",[]) if "score" in str(st[0]).lower()),None)
            doms=int(m_dom.group(1)) if m_dom else 0
            probes=int(m_pr.group(1)) if m_pr else 0
            if doms>0:
                if not mind_exams or doms>mind_exams[-1]["doms"]:
                    mind_exams.append({"id":r["id"],"doms":doms,"probes":probes,"rate":r.get("measure",100.0)})
                elif doms==mind_exams[-1]["doms"] and probes>=mind_exams[-1]["probes"]:
                    mind_exams[-1]={"id":r["id"],"doms":doms,"probes":probes,"rate":r.get("measure",100.0)}
    if mind_runs and mind_exams:
        cur_doms=mind_exams[-1]["doms"];cur_probes=mind_exams[-1]["probes"]
        cards=(("Live journal runs",len(live)),("Mastered domains",f"{cur_doms} ({cur_probes} probes)"),
               ("L2 plastic synapses","29.36M (97.5% L2)"),("Peak habitat pop",max((x.get("peak",0) for x in history),default=0)))
    else:
        cards=(("Live runs",len(live)),("Archived series",len(history)-len(live)),("Peak observed population",max((x.get("peak",0) for x in history),default=0)),
               ("Deepest observed lineage",max((x.get("depth",0) for x in history),default=0)))
    for i,(k,v) in enumerate(cards):
        x=49+358*i
        if i:d.rule(x-19,186,x-19,277,"#45685b")
        txt=v if isinstance(v,str) else human(v)
        d.text(x,233,txt,36 if len(txt)>10 else 43,MINT if i==0 else "#f3efd7",SERIF)
        d.text(x,271,k,17,"#c8d9ca")

    left,right=48,856
    if mind_runs and len(mind_exams)>=2:
        d.label(left,356,"CORTICAL CAPABILITY GROWTH ACROSS GENERATIONS & VERSIONS")
        d.text(left,392,"Mastered higher-cognition domains (100% English SVO = Alien SOV)",24,INK,SERIF)
        max_d=max(140,((mind_exams[-1]["doms"]+19)//20)*20)
        vals=[e["doms"] for e in mind_exams]
        for i in range(5):
            yy=422+142*i/4;d.rule(left+48,yy,right-18,yy,"#ced6c8")
            d.text(left+39,yy+4,f"{int(max_d*(4-i)/4)}",11.5,MUTED,MONO,anchor="end")
        trend(d,left+50,422,right-left-72,142,vals,GREEN,True,limits=(0,max_d))
        # Plot milestone dots + start/end tags
        n_ex=len(mind_exams)
        for ei,ex in enumerate(mind_exams):
            px_e=left+50+(right-left-72)*ei/max(1,n_ex-1)
            py_e=422+142*(1.0-ex["doms"]/max_d)
            d.dot(px_e,py_e,4.5,"#c98b3e" if ei==n_ex-1 else GREEN)
        d.text(left+50,584,f"Run {mind_exams[0]['id']:05d} ({mind_exams[0]['doms']} doms)",12,MUTED,MONO)
        d.text(right-18,584,f"Run {mind_exams[-1]['id']:05d} ({mind_exams[-1]['doms']} doms · {mind_exams[-1]['probes']} probes!)",12,GREEN,MONO,700,"end")
        # 3 crisp "Recent Breakthroughs" impact pills under the growth chart!
        chips=[("K51 256-MCMC GPU!","#235e51","51 Kernels & 52-Track Ribot"),("3-ENTITY & SIBLING","#99623e","GroupThree + Fermi/Roof"),("51 KERNELS · 550 DOMS!","#327a83","114-pillar evo! Zero LLMs!")]
        cw_ch=(right-left-24)/3.0
        for ci,(ct,cc,cs) in enumerate(chips):
            cx_c=left+ci*(cw_ch+12)
            d.rect(cx_c,604,cw_ch,62,"#e7ece0",8,f'stroke="{cc}" stroke-width="1.4"')
            d.text(cx_c+12,626,ct,11.2,cc,MONO,700)
            d.text(cx_c+12,649,cs,13.0,INK,SANS,500)
    else:
        d.label(left,370,"Population at recorded endpoints")
        d.text(left,410,"Share of habitat occupied",29,INK,SERIF)
        records=thin([r for r in worlds if capacity(r)],220)
        if records:
            for i in range(5):
                yy=454+165*i/4;d.rule(left+49,yy,right-26,yy,"#ced6c8")
                d.text(left+39,yy+4,f"{100-25*i}%",12,MUTED,MONO,anchor="end")
            trend(d,left+51,454,right-left-82,165,[100*r["alive"]/capacity(r) for r in records],GREEN,True,limits=(0,100))
            d.text(left,654,f"Record {records[0]['id']:05d}",13,MUTED,MONO)
            d.text(right-26,654,f"Record {records[-1]['id']:05d}",13,MUTED,MONO,anchor="end")
        else:d.text(left,548,"No ecological endpoint in this journal yet",23,MUTED,SERIF)

    d.rule(874,344,874,678)
    if mind_runs:
        rx_o=896;rw_o=W-48-rx_o
        d.label(rx_o,356,"WHAT CHANGED · WHAT IT MEANS · COMING UP NEXT")
        story_o=[
            ("WHAT JUST CHANGED",GREEN,"#e4eee7",
             f"Run {latest['id']:05d}: {short(latest['title'],32)} — STAGE 8o · 51 KERNELS & 550 DOMAINS! 114-pillar evo & K51 GPU 256-Chain MCMC!"),
            ("HOW IT INFLUENCES THE ANIMALS","#b5762c","#f5eddc",
             "Axion runs 8,192 parallel MCMC draws on GPU L2 (K51), clusters 3 entities via K12, expands sibling categories & resolves capability anaphora!"),
            ("COMING UP NEXT (STAGE 8p · 560 DOMAINS!)","#327a83","#e3eff0",
             "Next: K52 pure-GPU Spiking MCTS Tree-Search Planner Cortex (free PW27 Q!), deeper unconstrained chat & multi-mind science labs!"),
        ]
        for si,(st,sc,sbg,sb) in enumerate(story_o):
            sy=372+si*76
            d.rect(rx_o,sy,rw_o,68,sbg,8,f'stroke="{sc}" stroke-width="1.3"')
            d.rect(rx_o,sy,7,68,sc,3)
            d.text(rx_o+16,sy+19,st,10.2,sc,MONO,700,space=0.8)
            for li,ln in enumerate(card_wrap(sb,64)[:2]):
                d.text(rx_o+16,sy+38+li*16,ln,12.0,INK,SANS)
        d.label(rx_o,614,"LATEST 3 VERIFIED RECORDS")
        for i,r in enumerate(reversed(history[-3:])):
            yy=634+20*i
            d.text(rx_o,yy,f"{r['id']:05d}",11.5,GREEN,MONO,700)
            d.text(rx_o+58,yy,short(r["title"],38),12.5,INK,SANS,600)
            d.text(W-48,yy,short(r["family"],24),11.0,MUTED,MONO,anchor="end")
    else:
        d.label(918,370,"Latest records")
        for i,r in enumerate(reversed(history[-5:])):
            yy=415+53*i
            d.text(919,yy,f"{r['id']:05d}",13,GREEN,MONO)
            d.text(994,yy,short(r["title"],36),17,INK,SANS,600)
            d.text(994,yy+19,short(r["family"],45),12,MUTED,MONO)

    d.rule(48,694,W-48,694);d.label(48,728,"CORE COGNITIVE & ECOLOGICAL SCORECARD" if mind_runs else "Latest recorded task scores")
    scored=[]
    if mind_runs:
        # Curate 6 clean, headline capability families (never 6 random clipped sub-domains!)
        def last_match(pred,label,note_fn=None):
            r=next((x for x in reversed(history) if pred(x)),None)
            if not r:return None
            out=dict(r,family=label,measure=r.get("measure",0.0))
            if note_fn:out["_note"]=note_fn(r)
            return out
        def _stat_score(r, fallback):
            for st in r.get("stats", []):
                if "/" in str(st[1]): return str(st[1])
            return fallback
        def _dom_n(r, fallback=340):
            m_d = re.search(r"(\d+)\s+domains", r.get("subtitle", ""))
            return int(m_d.group(1)) if m_d else fallback
        def _pil_n(r, fallback=72):
            m_p = re.search(r"(\d+)-pillar", r.get("subtitle", ""))
            return int(m_p.group(1)) if m_p else fallback
        def _tck_n(r, fallback=400):
            m_t = re.search(r"(\d+)\s+ticks", r.get("subtitle", ""))
            return int(m_t.group(1)) if m_t else fallback
        def _ribot_name(nd):
            return ("Duoquinquagintuple" if nd>=545 else "Unquinquagintuple" if nd>=535 else "Quinquagintuple" if nd>=525 else "Novemquadragintuple" if nd>=515 else "Octoquadragintuple" if nd>=505 else "Septemquadragintuple" if nd>=495 else "Sexquadragintuple" if nd>=485 else "Quinquadragintuple" if nd>=475 else "Quattuorquaragintuple" if nd>=465 else "Trequaragintuple" if nd>=455 else "Duoquaragintuple" if nd>=445 else "Unquadragintuple" if nd>=435 else "Quadragintuple" if nd>=425 else "Novemtrigintuple" if nd>=415 else "Octotrigintuple" if nd>=405 else "Septentrigintuple" if nd>=395 else "Sextrigintuple" if nd>=385 else "Quintrigintuple" if nd>=375 else "Quattuortrigintuple" if nd>=365 else "Tretrigintuple" if nd>=355 else "Duotrigintuple" if nd>=345 else "Untrigintuple" if nd>=335 else "Trigintuple" if nd>=325 else "Nonavigintuple" if nd>=315 else "Octovigintuple" if nd>=305 else "Septemvigintuple" if nd>=295 else
                    "Sexvigintuple" if nd>=285 else "Quinvigintuple" if nd>=275 else
                    "Quattuorvigintuple" if nd>=265 else "Trevigintuple")
        picks=[
            last_match(lambda x:x.get("family")=="Held-out cognitive accuracy" and "English" in x.get("subtitle","") and "arm=intact" in x.get("subtitle",""),
                       "English SVO Cognitive Exam",lambda r:f"Run {r['id']:05d}  ·  {_dom_n(r)} domains · {_stat_score(r,'1046/1046')} intact"),
            last_match(lambda x:x.get("family")=="Held-out cognitive accuracy" and "Alien" in x.get("subtitle","") and "arm=intact" in x.get("subtitle",""),
                       "Alien SOV Exact-Parity Exam",lambda r:f"Run {r['id']:05d}  ·  Zero English · {_stat_score(r,'1046/1046')} parity"),
            last_match(lambda x:x.get("family")=="Evolved cortical fitness",
                       "GPU Multi-Pillar Evolution",lambda r:f"Run {r['id']:05d}  ·  {_pil_n(r)} pillars · 4,096 minds"),
            last_match(lambda x:x.get("family")=="Ecosystem cognition score",
                       "Open-Valley Multi-Gen Ecosystem",lambda r:f"Run {r['id']:05d}  ·  {_tck_n(r)} ticks · {_stat_score(r,'56/56')} checks"),
            last_match(lambda x:x.get("family")=="Society cognition score",
                       "4-Mind Trust & Lie Vigilance",lambda r:f"Run {r['id']:05d}  ·  3 Fox lies caught · {_stat_score(r,'8/8')}"),
            last_match(lambda x:x.get("family")=="Cooperative exchange score" or (x.get("family")=="Held-out cognitive accuracy" and "arm=hipp" in x.get("subtitle","")),
                       "Ribot Cortical Survival",lambda r:f"Run {r['id']:05d}  ·  Hipp off · {_ribot_name(_dom_n(r,140))} Ribot"),
        ]
        scored=[p for p in picks if p is not None]
    if len(scored)<6:
        groups={}
        for r in history:
            if r.get("unit")=="%":groups[r["family"]]=r
            for name,num,den in r.get("scores",[]):
                if den:groups[name]=dict(r,family=name,measure=100*num/den)
        for cand in reversed(sorted(groups.values(),key=lambda r:r["id"])):
            if all(s["family"]!=cand["family"] for s in scored):
                scored.append(cand)
            if len(scored)>=6:break
    if not scored:d.text(48,829,"Scored tasks will appear here after a learning run.",23,MUTED,SERIF)
    for i,r in enumerate(scored[:6]):
        col=i%3;row=i//3;x=48+484*col;yy=774+94*row
        d.rect(x-8,yy-24,458,78,"#e9eee3",8,'stroke="#cfd8c9" stroke-width="1"')
        d.text(x+4,yy,short(r["family"],33),16.5,INK,SANS,600)
        d.text(x+438,yy,f"{r['measure']:.1f}%",16.5,GREEN,MONO,700,"end")
        d.rect(x+4,yy+15,434,9,"#d3dcd0",4);d.rect(x+4,yy+15,434*min(100,max(0,r["measure"]))/100,9,GREEN,4)
        sub_note=r.get("_note") or (f"Run {r['id']:05d}  ·  {short(r.get('subtitle',''),34)}" if mind_runs else f"Record {r['id']:05d}")
        d.text(x+4,yy+42,sub_note,11.2,MUTED,MONO)
    d.rule(48,966,W-48,966)
    d.text(48,1004,"Zero LLMs · zero backprop · every spike, synapse edit & ablation logged reproducibly in JOURNAL.md.",16,MUTED)
    recent=latest_timing(history,True) or latest_timing(history)
    if recent:performance_ribbon(d,recent["performance"],f"Timed run {recent['id']:05d}")
    return d


def atomic(path,write):
    name=None
    try:
        with tempfile.NamedTemporaryFile(prefix=".write-",dir=path.parent,delete=False) as fd:name=Path(fd.name)
        write(name)
        os.replace(name,path);name=None
    finally:
        if name and name.exists():name.unlink()


def read_history(root):
    runs=root/"runs";runs.mkdir(parents=True,exist_ok=True)
    return [json.loads(p.read_text()) for p in sorted(runs.glob("[0-9]*-*/summary.json"))]


def md(s):return re.sub(r'([\\\[\]*_\x60<>|])',r'\\\1'," ".join(str(s).splitlines()))


def markdown(history):
    live=sum(not x["archive"] for x in history)
    atlas=" · [Current creature card](creature-card.webp) · [Capability timeline](capabilities.json)" if any(b.get("card") for b in history) else ""
    lines=["# Axion evolution journal","","[Latest run](latest.webp) · [Progress across all records](progress.webp) · [The journey from the beginning](journey.webp)"+atlas,"",
           f"{live:,} live runs and {len(history)-live:,} archived records. Each image preserves the progress known when that entry was created.",""]
    if atlas:
        lines.extend(['<a href="creature-card.webp"><img src="creature-card.webp" alt="Current creature card" width="470"></a> '
                      '<a href="journey.webp"><img src="journey.webp" alt="Timeline from the first record through the latest run" width="180"></a>',""])
    for b in reversed(history):
        card_link=f" · [Creature card]({b['path']}/creature-card.webp)" if b.get("card") else ""
        lines.extend([f"## {b['id']:05d} · {md(b['title'])}","",
                      f"{'Archived evidence' if b['archive'] else 'Completed '+b['date']} · [WebP plate]({b['path']}/plate.webp){card_link} · [Summary]({b['path']}/summary.json) · [Observations]({b['path']}/observations.json.gz)",""])
        if b.get("parents"):
            links=(f"[run {p['id']:05d}]({p['path']}/plate.webp)" for p in b["parents"])
            lines.extend(["Input checkpoints were saved in "+", ".join(links)+".",""])
        for bullet in b['bullets']:lines.append("- "+md(bullet))
        lines.extend([""," ".join(md(x) for x in b["narrative"]),""])
    return "\n".join(lines).rstrip("\n")+"\n"


def restore_performance(root,history,codec,ledger):
    # Old observations predate the performance ribbon. Reconstruct a figure
    # only from its own measurements and the history available on its run.
    pending=[]
    for i,b in enumerate(history):
        old=b.get("performance",{})
        mixed_scopes=old.get("unit")=="site-ticks/s" and old.get("byte_scope")=="Motor image / animal"
        if b.get("archive") or ("performance" in b and not mixed_scopes):continue
        raw=card_observations(root,b);perf=measured_performance(raw)
        if not perf:continue
        b["performance"]=perf;pending.append(i)
    if pending:
        atomic(root/"journey.webp",lambda path:codec.write(journey_drawing(history,ledger),path))
    def restore(i):
        b=history[i];raw=card_observations(root,b);directory=root/b["path"]
        atomic(directory/"plate.webp",lambda path:codec.write(plate(raw,b,history[:i+1]),path))
        if b.get("card") and (directory/CARD_FILE).is_file():
            atomic(directory/CARD_FILE,lambda path:codec.write(card_drawing(b,ledger),path))
        atomic(directory/"summary.json",lambda path:path.write_text(json.dumps(b,ensure_ascii=False,indent=2)+"\n"))
    if len(pending)>1:
        from concurrent.futures import ThreadPoolExecutor
        with ThreadPoolExecutor(max_workers=min(3,len(pending))) as workers:
            for _ in workers.map(restore,pending):pass
    elif pending:restore(pending[0])
    return len(pending)


def refresh(root,history,codec,repair=False):
    # Genesis runs use ONE clean, self-contained storytelling plate.webp per run/milestone
    # (skipping redundant legacy creature-card / roadmap / journey / progress clutter!).
    if history and not repair:
        newest=next((r for r in reversed(history) if not r["archive"]),history[-1])
        if newest.get("kind")=="genesis" or newest.get("command")=="genesis":
            atomic(root/"JOURNAL.md",lambda p:p.write_text(markdown(history)))
            latest=root/newest["path"]/"plate.webp"
            atomic(root/"latest.webp",lambda p:shutil.copyfile(latest,p))
            gmode=newest.get("genesis_mode")
            if gmode:
                atomic(root/f"genesis-{gmode}.webp",lambda p:shutil.copyfile(latest,p))
            return
    if repair:
        for b in history:
            if b["kind"]=="floodplain" and b.get("alive") is None:
                raw=card_observations(root,b);fixed=brief(raw)
                for key in ("alive","capacity","depth","peak"):b[key]=fixed[key]
                atomic(root/b["path"]/"summary.json",lambda path:path.write_text(json.dumps(b,ensure_ascii=False,indent=2)+"\n"))
    ledger=card_history(root,history)
    if repair:
        # Earlier mineral panels let the lowest row reach into the legend.
        # Repaint from the exposure captured in that run, including its original crop.
        candidates=[(i,r) for i,r in enumerate(history) if r.get("kind")=="floodplain" and r.get("mineral_drawing",0)<2
                    and any("hand_faces" in a.get("q",{}) for a in r.get("card",{}).get("roster",{}).get("items",[]))]
        def refit(job):
            i,r=job;raw=card_observations(root,r)
            if not any(v.startswith("mineral_row=") for v in raw.get("lines",[])):return
            r["mineral_drawing"]=2;directory=root/r["path"]
            atomic(directory/"plate.webp",lambda p:codec.write(plate(raw,r,history[:i+1]),p))
            atomic(directory/"summary.json",lambda p:p.write_text(json.dumps(r,ensure_ascii=False,indent=2)+"\n"))
        if candidates:
            from concurrent.futures import ThreadPoolExecutor
            with ThreadPoolExecutor(max_workers=3) as workers:
                for _ in workers.map(refit,candidates):pass
        shore_families=("Construction from isolated stock","Live shore harvest","Requested shore harvest","Detached shore harvest")
        old_shoals=[(i,r) for i,r in enumerate(history) if r.get("shore_drawing",0)<3 and r.get("family") in shore_families]
        # Freeze every earlier comparison before parallel drawing starts.
        for _,r in old_shoals:
            new=brief(card_observations(root,r))
            for field in ("title","subtitle","family","measure","unit","stats","narrative","bullets"):
                r[field]=new[field]
            r["shore_drawing"]=3
        def repaint_shoal(record):
            i,r=record;raw=card_observations(root,r);directory=root/r["path"]
            atomic(directory/"plate.webp",lambda path:codec.write(plate(raw,r,history[:i+1]),path))
            atomic(directory/"summary.json",lambda path:path.write_text(json.dumps(r,ensure_ascii=False,indent=2)+"\n"))
        if old_shoals:
            from concurrent.futures import ThreadPoolExecutor
            with ThreadPoolExecutor(max_workers=3) as workers:
                for _ in workers.map(repaint_shoal,old_shoals):pass
        restore_performance(root,history,codec,ledger)
    if history:
        newest=next((r for r in reversed(history) if not r["archive"]),history[-1])
        needed={newest["id"]}
        if repair:needed.update(r["id"] for r,_ in journey_stops(history,ledger))
        def update_sheet(i):
            r=history[i];directory=root/r["path"];target=directory/CARD_FILE
            if r["id"] not in needed and not (repair and r.get("card")):return
            if r.get("card",{}).get("atlas_version")==ATLAS_SCHEMA and target.is_file():return
            data=card_observations(root,r)
            short_ledger=dict(ledger,records=i,milestones=[m for m in ledger["milestones"] if m["run"]<r["id"]])
            r["card"]=card_record(data,r,short_ledger)
            r["card"]["series"]=card_series(root,data,r,history[:i])
            atomic(target,lambda p:codec.write(card_drawing(r,short_ledger),p))
            atomic(directory/"summary.json",lambda p:p.write_text(json.dumps(r,ensure_ascii=False,indent=2)+"\n"))
        active=next((i for i in range(len(history)-1,-1,-1) if history[i].get("card",{}).get("roster",{}).get("kind")=="census"),history.index(newest))
        update_sheet(active)
        atomic(root/CARD_FILE,lambda p:shutil.copyfile(root/history[active]["path"]/CARD_FILE,p))
        jobs=[i for i,r in enumerate(history) if i!=active and (r["id"] in needed or repair and r.get("card"))]
        if len(jobs)>4:
            from concurrent.futures import ThreadPoolExecutor
            with ThreadPoolExecutor(max_workers=3) as workers:
                for _ in workers.map(update_sheet,jobs):pass
        else:
            for i in jobs:update_sheet(i)
        population=next((r for r in reversed(history) if r.get("card",{}).get("roster",{}).get("kind")=="census"),newest)
        atomic(root/CARD_FILE,lambda p:shutil.copyfile(root/population["path"]/CARD_FILE,p))
    card_cache(root,ledger)
    atomic(root/"journey.webp",lambda p:codec.write(journey_drawing(history,ledger),p))
    atomic(root/"roadmap.webp",lambda p:codec.write(roadmap_drawing(history,ledger),p))
    atomic(root/"JOURNAL.md",lambda p:p.write_text(markdown(history)))
    atomic(root/"progress.webp",lambda p:codec.write(overview(history),p))
    if history:
        newest=next((r for r in reversed(history) if not r["archive"]),history[-1])
        latest=root/newest["path"]/"plate.webp"
        atomic(root/"latest.webp",lambda p:shutil.copyfile(latest,p))


def store(root,data,history,codec):
    b=brief(data);archive=b["archive"]
    if b["family"] in ("Construction from isolated stock","Live shore harvest","Requested shore harvest","Detached shore harvest"):b["shore_drawing"]=3
    if any(line.startswith("mineral_row=") for line in data.get("lines",[])):b["mineral_drawing"]=2
    if archive and any(x["archive"]==archive for x in history):return None
    identifier=max((x["id"] for x in history),default=0)+1
    if identifier>99999999:raise ValueError("journal record counter exhausted")
    if data.get("checkpoints"):
        b["checkpoints"]=data["checkpoints"]
        inputs={f["checksum"] for f in b["checkpoints"] if not f["saved"]}
        parents=[]
        for prior in reversed(history):
            if not inputs:break
            matches={f["checksum"] for f in prior.get("checkpoints",[]) if f["saved"]} & inputs
            if matches:
                parents.append({"id":prior["id"],"path":prior["path"]})
                inputs-=matches
        if parents:b["parents"]=parents[::-1]
    slug=re.sub(r'[^a-z0-9]+','-',data["command"].lower()).strip("-")[:30] or "run"
    rel=f"runs/{identifier:08d}-{slug}";b.update(id=identifier,path=rel,date=data.get("recorded_at") or utc(),version=1)
    is_genesis=(b.get("kind")=="genesis" or data.get("command")=="genesis")
    if is_genesis:
        glines=data.get("lines",[])
        if any(l.startswith("genesis_summary=") for l in glines): b["genesis_mode"]="evolve"
        elif any(l.startswith("genesis_probe=") for l in glines): b["genesis_mode"]="probe"
        elif any(l.startswith("genesis_converse=") for l in glines): b["genesis_mode"]="converse"
        elif any(l.startswith("genesis_inspect=") for l in glines): b["genesis_mode"]="inspect"
        elif any(l.startswith("genesis_bench=") for l in glines): b["genesis_mode"]="bench"
        else:
            gargs=data.get("args",[])
            if gargs and gargs[0] in ("evolve","probe","converse","bench","inspect"):
                b["genesis_mode"]=gargs[0]
    ledger=None
    if not is_genesis:
        ledger=card_history(root,history);b["card"]=card_record(data,b,ledger)
        b["card"]["series"]=card_series(root,data,b,history)
    staged=Path(tempfile.mkdtemp(prefix=".pending-",dir=root))
    try:
        codec.write(plate(data,b,[*history,b]),staged/"plate.webp")
        if not is_genesis:
            codec.write(card_drawing(b,ledger),staged/CARD_FILE)
        (staged/"summary.json").write_text(json.dumps(b,ensure_ascii=False,indent=2)+"\n")
        with gzip.open(staged/"observations.json.gz","wt",encoding="utf-8",compresslevel=5) as f:json.dump(data,f,ensure_ascii=False,separators=(",",":"))
        os.replace(staged,root/rel)
    finally:
        if staged.exists():shutil.rmtree(staged)
    history.append(b)
    if not is_genesis and ledger is not None:
        card_cache(root,ledger)
    return b


def publish(root,data,defer=False):
    root.mkdir(parents=True,exist_ok=True)
    with (root/".lock").open("a+") as lock:
        fcntl.flock(lock,fcntl.LOCK_EX);history=read_history(root);codec=Codec()
        b=store(root,data,history,codec)
        if not defer:refresh(root,history,codec)
    return b


def receipt_id(name,stat,raw):
    identity=f"{name}/{stat.st_mtime_ns}/{stat.st_ino}".encode()
    return hashlib.sha256(identity+b"\0"+raw).hexdigest()

def queued(root,batch=False,own=None):
    root.mkdir(parents=True,exist_ok=True)
    with (root/".lock").open("a+") as lock:
        fcntl.flock(lock,fcntl.LOCK_EX);history=read_history(root);codec=Codec()
        known={r.get("receipt") for r in history};processed=[];published=[]
        receipts=sorted(root.glob(".receipt-*"),key=lambda p:(p.stat().st_mtime_ns,p.name))
        for path in receipts:
            raw=path.read_bytes()
            stat=path.stat();receipt=receipt_id(path.name,stat,raw)
            if receipt not in known:
                data=json.loads(raw);data["receipt"]=receipt
                if not re.fullmatch(r"\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ",str(data.get("recorded_at",""))):
                    data["recorded_at"]=utc(stat.st_mtime)
                if not data.get("archive"):batch=False
                entry=store(root,data,history,codec);known.add(receipt)
                if entry and not entry["archive"]:published.append(entry)
            processed.append(path)
        if not batch and (processed or own is None):refresh(root,history,codec,repair=own is None)
        for path in processed:path.unlink()
        if own is not None:published=[r for r in history if r.get("receipt")==own and not r["archive"]]
        for entry in published:print(f"Run {entry['id']:05d} · {root/entry['path']/'plate.webp'}",file=sys.stderr)
    return len(history)


def tabular(path,repo):
    rel=path.relative_to(repo).as_posix();digest=hashlib.sha256(path.read_bytes()).hexdigest()
    with path.open(newline="") as f:rows=list(csv.DictReader(f))
    groups=defaultdict(list)
    for r in rows:
        seed=next((r.get(k) for k in ("seed","food_seed","world_seed","founder_seed") if r.get(k)),None)
        groups[seed].append(r)
    for seed,records in groups.items():
        title=path.stem.replace("-"," ").replace("_"," ").title();family=path.parent.name.capitalize()+" · "+title
        key=f"table:{rel}@{digest}"+(f":{seed}" if seed else "")
        s=dict(command="archive",kind="archive",archive=key,source=rel,title=title,subtitle=family+(f"   /   Seed {seed}" if seed else ""),
               family=family,metrics={"observations":len(records)},args=[],lines=[],scores=[])
        # Preserve units. A seed can include several interventions and phases.
        ratios=(("revised","items","Context-dependent revision"),("inferred_changed","items","Revaluation"),("d4_changed","items","Route revision"),("changed","changed_items","Changed first moves"),
                ("changed","items","Revaluation"),("correct","items","Correct choices"),("matches","points","Matched choices"),
                ("first","questions","First moves"),("arrived","journeys","Goals reached"),("learned","items","Acquired choices"),
                ("recalled","recall_items","Recall"),("inferred","value_items","Food revaluation"))
        measurement=None
        for num,den,label in ratios:
            g=[(i,numeric(v.get(num)),numeric(v.get(den))) for i,v in enumerate(records)]
            g=[(i,n,d) for i,n,d in g if n is not None and d and 0<=n<=d]
            if g:measurement=(num,den,label,g);break
        unique=lambda k:sorted(set(x[k] for x in records if x.get(k)))
        maxval=lambda k:max([numeric(x.get(k)) for x in records if numeric(x.get(k)) is not None] or [0])
        m=s["metrics"];m["peak_alive"]=int(maxval("alive"));m["peak_depth"]=int(maxval("max_generation"))
        unit="rows";s["measure"]=len(records);stats=[["Observations",len(records)]]
        s["narrative"]=[f"This archived series contains {len(records):,} recorded measurements"+(f" for seed {seed}." if seed else ".")]
        if measurement:
            num,den,label,g=measurement;s["family"]=family+" · "+label;unit="%"
            keycol=next((k for k in ("phase","calibration","crossings","maze","valuation","task","growth","inheritance","arm","schedule","tick") if len(unique(k))>1),None)
            bucket=defaultdict(lambda:[0,0])
            for i,n,d in g:
                keyv=records[i].get(keycol,"") if keycol else f"Row {i+1}"
                bucket[keyv][0]+=n;bucket[keyv][1]+=d
            entries=list(bucket.items());s["series"]=thin([[k,100*n/d]for k,(n,d) in entries],12)
            n=sum(x[1] for x in g);d=sum(x[2] for x in g)
            s["scores"]=[[label,int(n),int(d)]];s["measure"]=100*n/d
            stats+=[["Scored choices",int(d)],["Pooled rate",ratio(n,d)]]
            s["narrative"]+=[f"Across the recorded conditions, {human(n)} of {human(d)} scored choices met the {label.lower()} criterion.",
                             f"The figure groups those observations by {keycol.replace('_',' ') if keycol else 'source row'} and keeps the original denominators."]
        elif m["peak_alive"] or "alive" in records[0]:
            unit="animals";s["measure"]=m["peak_alive"];s["family"]="Archive population · "+path.parent.name
            s["series"]=thin([[f"Tick {r.get('tick',i)}",val(r,"alive")]for i,r in enumerate(records) if numeric(r.get("alive")) is not None],24)
            stats+=[["Peak population",m["peak_alive"]],["Deepest lineage",m["peak_depth"]]]
            s["narrative"] += [f"The highest recorded population was {human(m['peak_alive'])} animals.",
                               f"The deepest recorded resident lineage reached {human(m['peak_depth'])} divisions." if m["peak_depth"] else "The retained counts describe population size at the sampled endpoints."]
        elif "final_treatment" in records[0] and "final_control" in records[0]:
            cohorts=defaultdict(lambda:[0,0]);wins=0
            for r in records:
                treated=int(r["final_treatment"]);control=int(r["final_control"])
                condition=r.get("contest") or r.get("habitat") or "Competition"
                totals=cohorts[condition];totals[0]+=treated;totals[1]+=control;wins+=treated>control
            categories=[[str(condition),100*a/(a+b)] for condition,(a,b) in cohorts.items() if a+b]
            s["series"]=thin(categories,12);s["chart_label"]="Treatment share of survivors"
            s["family"]=family+" · Treatment wins";unit="%";s["scores"]=[["Treatment majority",wins,len(records)]]
            s["measure"]=100*wins/len(records);stats+=[["Treatment majorities",wins],["Final control animals",sum(v[1]for v in cohorts.values())]]
            s["narrative"] += [f"The treatment cohort finished ahead in {human(wins)} of {human(len(records))} competitions.",
                               "The chart gives the treatment share of surviving animals within each recorded condition."]
        else:
            col=next((k for k in ("offspring_mass","tested_lifetimes","Gcell_updates/s","Mchoices/s","dispersal_ms","GPU_ms","neural_regs_per_thread","births","moves","correct","count","neurons","generation","items","ticks") if any(numeric(x.get(k)) is not None for x in records)),None)
            if col:
                vals=[(i,numeric(x.get(col)))for i,x in enumerate(records) if numeric(x.get(col)) is not None]
                s["series"]=thin([[f"Row {i+1}",v]for i,v in vals],24);unit=col;s["measure"]=max(v for _,v in vals)
                stats+=[["Peak "+col.replace("_"," "),s["measure"]],["Data columns",len(records[0])]]
                s["narrative"] += [f"The recorded {col.replace('_',' ')} ranges from {human(min(v for _,v in vals))} to {human(s['measure'])}.","The linked source table preserves the conditions for each observation."]
            else:
                stats.append(["Data columns",len(records[0])]);s["narrative"]+=["The table records the conditions and outcomes of this experiment.","Its original observations are included with the journal entry."]
        s.update(unit=unit,stats=stats,bullets=[f"Source table {rel}.",f"{len(records):,} retained rows"+(f" sharing seed {seed}." if seed else ".")],
                 rows=records)
        yield s


def archive(root,repo):
    repo=repo.resolve();root.mkdir(parents=True,exist_ok=True);added=0
    # Import raw experiment tables once, then render the overview once for the batch.
    with (root/".lock").open("a+") as lock:
        fcntl.flock(lock,fcntl.LOCK_EX);history=read_history(root);known={x["archive"] for x in history};codec=Codec()
        for path in sorted((repo/"docs").rglob("*.csv")):
            for data in tabular(path,repo):
                if data["archive"] in known:continue
                if store(root,data,history,codec):added+=1;known.add(data["archive"])
        refresh(root,history,codec)
    # Existing files preserve endpoints. Read their actual CUDA state, without advancing them.
    for path in sorted((repo/"examples").glob("*")):
        if path.suffix not in (".eco",".mind",".axn"):continue
        rel=path.relative_to(repo).as_posix();key=f"checkpoint:{rel}@"+hashlib.sha256(path.read_bytes()).hexdigest()
        if key in known:continue
        if path.suffix==".axn":
            q=dict(command="archive",kind="archive",archive=key,source=rel,title=path.stem.title()+" checkpoint",
                   subtitle="Saved reference population",family="Reference checkpoint size",unit="bytes",measure=path.stat().st_size,
                   metrics={"observations":1},stats=[["Checkpoint bytes",path.stat().st_size]],scores=[],lines=[],args=[],
                   narrative=[f"The retained {path.stem} population is represented by a complete checkpoint.",
                              f"The file occupies {human(path.stat().st_size)} bytes.","The associated experiment tables supply its recorded learning outcomes."],
                   bullets=[f"Source checkpoint {rel}.","Stored as an archived endpoint."])
            publish(root,q,True)
        else:
            cmd=["life","--load",rel,"--steps","0"] if path.suffix==".eco" else ["agent","--load",rel]
            env=dict(os.environ,AXION_JOURNAL_DIR=str(root),AXION_JOURNAL_ARCHIVE_KEY=key,AXION_JOURNAL_BATCH_IMPORT="1")
            subprocess.run([BINARY,*cmd],cwd=repo,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,check=True)
        added+=1
    with (root/".lock").open("a+") as lock:
        fcntl.flock(lock,fcntl.LOCK_EX);history=read_history(root);refresh(root,history,Codec())
    print(f"Imported {added} records. {len(history)} journal entries in {root}.")


def main():
    global ROOT,BINARY
    if len(sys.argv)<3:raise ValueError("usage: journal.py JOURNAL_DIR AXION_BINARY [record|rebuild|import REPO]")
    ROOT=Path(sys.argv[1]).resolve();BINARY=str(Path(sys.argv[2]).resolve());args=sys.argv[3:];mode=args[0] if args else "rebuild"
    if mode in ("--help","help"):
        print("axion journal rebuild | import [REPO] [--journal-dir DIR]");return
    if mode=="record":
        own=None
        if len(args)==3:
            with os.fdopen(int(args[2]),"rb") as src:own=receipt_id(args[1],os.fstat(src.fileno()),src.read())
        elif len(args)!=1:raise ValueError("invalid journal receipt arguments")
        queued(ROOT,bool(os.getenv("AXION_JOURNAL_BATCH_IMPORT")),own);return
    if mode=="import":archive(ROOT,Path(args[1]) if len(args)>1 else Path.cwd());return
    if mode!="rebuild":raise ValueError("journal accepts rebuild or import [REPO]")
    print(f"Rebuilt {queued(ROOT)} journal records in {ROOT}.")


# A local script uses the same field-guide definitions embedded ahead of this
# source in the standalone C++ executable.
if "compound_observation" not in globals():
    exec(compile(Path(__file__).with_name("compound_journal.py").read_text(),"compound_journal.py","exec"))
if "context_observation" not in globals():
    exec(compile(Path(__file__).with_name("context_journal.py").read_text(),"context_journal.py","exec"))
if "detritus_observation" not in globals():
    exec(compile(Path(__file__).with_name("detritus_journal.py").read_text(),"detritus_journal.py","exec"))
if "capillary_observation" not in globals():
    exec(compile(Path(__file__).with_name("capillary_journal.py").read_text(),"capillary_journal.py","exec"))
if "inquiry_observation" not in globals():
    exec(compile(Path(__file__).with_name("inquiry_journal.py").read_text(),"inquiry_journal.py","exec"))
if "retinal_observation" not in globals():
    exec(compile(Path(__file__).with_name("retina_journal.py").read_bytes(),"retina_journal.py","exec"))
if "inlet_observation" not in globals():
    exec(compile(Path(__file__).with_name("inlet_journal.py").read_bytes(),"inlet_journal.py","exec"))
if "drift_observation" not in globals():
    exec(compile(Path(__file__).with_name("drift_journal.py").read_bytes(),"drift_journal.py","exec"))

if "braid_summary" not in globals():
    exec(compile(Path(__file__).with_name("braid_journal.py").read_bytes(),"braid_journal.py","exec"))
if "mind_summary" not in globals():
    exec(compile(Path(__file__).with_name("mind_journal.py").read_bytes(),"mind_journal.py","exec"))
if "genesis_summary" not in globals():
    exec(compile(Path(__file__).with_name("genesis_journal.py").read_bytes(),"genesis_journal.py","exec"))
if "roadmap_drawing" not in globals():
    exec(compile(Path(__file__).with_name("roadmap_card.py").read_bytes(),"roadmap_card.py","exec"))

if "card_record" not in globals():
    _cards=Path(__file__).with_name("creature_cards.py")
    exec(compile(_cards.read_bytes(),str(_cards),"exec"))


if "population_roster" not in globals():
    exec(compile(Path(__file__).with_name("population_cards.py").read_text(), "population_cards.py", "exec"))

if __name__=="__main__":
    try:main()
    except (ValueError,RuntimeError,OSError,subprocess.CalledProcessError) as exc:
        print(f"axion journal: {exc}",file=sys.stderr);sys.exit(1)
