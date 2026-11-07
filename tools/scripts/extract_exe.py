#!/usr/bin/env python3
"""Extract a file (by default SLUS_006.64) from the root directory of a raw
MODE2/2352 Xenogears disc image, without mounting it.

Usage: python3 tools/scripts/extract_exe.py <disc1.bin> [--name SLUS_006.64] [--extract-to disc/]
"""

import argparse
import os
import sys

from cdrom.cdxa import CdromXa
from cdrom.fs import DirectoryRecord, VolumeDescriptor

PVD_SECTOR = 16


def find_disc_dir():
    start = os.path.abspath(os.path.dirname(__file__))
    while start != '/':
        candidate = os.path.join(start, 'disc')
        if os.path.isdir(candidate):
            return candidate
        start = os.path.dirname(start)
    return os.getcwd()


def iter_directory(disc: CdromXa, record: DirectoryRecord):
    sectors = (record.data_size + CdromXa.SECTOR_DATA_SIZE - 1) // CdromXa.SECTOR_DATA_SIZE
    disc.seek_sector(record.data_logical_block_num)
    for _ in range(sectors):
        data = disc.read_next_sector()
        pos = 0
        while pos < len(data):
            length = data[pos]
            if length == 0:
                break  # records never straddle sectors; rest of sector is padding
            yield DirectoryRecord(data[pos:pos + length])
            pos += length


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("bin_file", help="Path to the Xenogears disc 1 .bin (MODE2/2352)")
    parser.add_argument("--name", default="SLUS_006.64", help="File in the disc root to extract")
    parser.add_argument("--extract-to", default=find_disc_dir())
    args = parser.parse_args()

    with CdromXa(args.bin_file) as disc:
        pvd = VolumeDescriptor(disc.read_sector(PVD_SECTOR))
        assert pvd.standard_identifier == b'CD001', "not an ISO9660 image"
        wanted = args.name.upper().encode()
        for rec in iter_directory(disc, pvd.root_dir_record):
            name = rec.name.split(b';')[0]
            if name == wanted and rec.file_flags & 0x2 == 0:
                out = os.path.join(args.extract_to, args.name)
                with open(out, "wb") as fh:
                    disc.extract_sectors(rec.data_logical_block_num, rec.data_size, fh)
                    fh.truncate(rec.data_size)
                print(f"Extracted {args.name} ({rec.data_size} bytes) to {out}")
                return 0
    print(f"{args.name} not found in disc root", file=sys.stderr)
    return 1


if __name__ == '__main__':
    sys.exit(main())
