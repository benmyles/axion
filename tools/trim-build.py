#!/usr/bin/env python3
"""Share immutable native kernel pages with the executable on supporting Linux filesystems."""
import errno
import fcntl
from pathlib import Path
import struct
import subprocess
import sys


def trim(executable, directory):
    if not executable.is_file() or not directory.is_dir():
        return
    with executable.open("rb") as binary:
        header = binary.read(64)
        if header[:6] != b"\x7fELF\x02\x01":
            return
        offset, = struct.unpack_from("<Q",header,32)
        width, count = struct.unpack_from("<HH",header,54)
        segments = []
        for j in range(count):
            binary.seek(offset+j*width)
            kind,_,start,virtual,_,length,_,_ = struct.unpack("<IIQQQQQQ",binary.read(56))
            if kind==1:
                segments.append((virtual,length,start))
        try:
            names = subprocess.run(["nm","-n","--defined-only",executable],capture_output=True,text=True,check=True)
        except (FileNotFoundError,subprocess.CalledProcessError):
            return
        symbols = {}
        for line in names.stdout.splitlines():
            columns = line.split()
            if len(columns)==3 and columns[2].startswith("axion_"):
                symbols[columns[2][6:]] = int(columns[0],16)
        for file in directory.glob("sm_*/*.cubin"):
            if file.is_symlink() or file.stem not in symbols:
                continue
            address = symbols[file.stem]
            size = file.stat().st_size//4096*4096
            for virtual,length,start in segments:
                if not size or not (virtual<=address and address+size<=virtual+length):
                    continue
                begin = start+address-virtual
                if begin%4096:
                    break
                with file.open("r+b") as kernel:
                    # FIDEDUPERANGE verifies that the bytes already agree.
                    request = bytearray(struct.pack("=QQHHIqQQiI",begin,size,1,0,0,kernel.fileno(),0,0,0,0))
                    try:
                        fcntl.ioctl(binary,0xc0189436,request)
                    except OSError as error:
                        if error.errno in (errno.ENOTTY,errno.EOPNOTSUPP,errno.EINVAL,errno.EXDEV,errno.EPERM):
                            break
                        raise
                break


if __name__=="__main__":
    trim(Path(sys.argv[1]),Path(sys.argv[2]))
