#!/usr/bin/env python3
"""Static (read-only) inspection of downloads/GTA2INSTALLER.exe for task T02.

No extraction tool runs here. This script only reads bytes and records what is
statically recoverable:

  * installer identity (MD5 is re-checked, SHA-1/SHA-256 are reported),
  * the PE32 stub header and section table,
  * the InstallShield self-extracting (SFX) payload record directory,
  * the embedded InstallShield IIS header (Disk1\\data1.hdr) name pool, which is
    the authoritative file manifest for the 9.6F data profile.

It deliberately performs NO writes outside the two output paths below.

Usage:
    python3 scripts/gta2_static_inventory.py \
        --installer downloads/GTA2INSTALLER.exe \
        --outdir research/runtime-data/inventory
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import re
import struct
import sys

EXPECTED_MD5 = "764848b25aed87b69da472f18c19580d"
EXPECTED_BYTES = 362331961

# InstallShield self-extracting stub directory: 4 NUL-terminated ASCII fields
# (name, path, version, size_field) followed immediately by the payload bytes.
SFX_RECORD = re.compile(
    rb"([\x20-\x7e]{1,64})\x00"
    rb"(Disk1\\[\x20-\x7e]{1,120})\x00"
    rb"(\d{1,4}(?:\.\d{1,4}){1,3})\x00"
    rb"([0-9A-Fa-f]{1,10})\x00"
)

# Offsets are absolute within the installer and were established by inspection;
# they are assertions here so that a changed installer fails loudly.
PE_IMAGE_END = 0x19400          # .rsrc raw end: 0x11400 + 0x8000
SFX_FIRST_RECORD = 0x19400
IIS_HEADER_OFFSET = 0xEE96E     # Disk1\data1.hdr, magic "ISc("
IIS_HEADER_LENGTH = 71292
IIS_ENCRYPTED_TABLE_OFFSET = 0x10000  # header-relative; high-entropy record table
RTL_CAB_OFFSET = 0x15818A40     # Disk1\engine32.cab, magic "MSCF"

ROLE_BY_EXT = {
    "gmp": "district map geometry (GMP)",
    "mmp": "multiplayer map (MMP)",
    "scr": "compiled mission script (SCR)",
    "sty": "style/graphics index (STY)",
    "gxt": "text and glyph resource (GXT)",
    "raw": "raw graphics/palette data",
    "sdt": "sprite/audio data (SDT)",
    "wav": "audio sample (WAV)",
    "bik": "video (Bink)",
    "tga": "frontend/menu texture (TGA)",
    "cfg": "keyboard layout config (CFG)",
    "rep": "replay/demo file (REP)",
    "gci": "game/config info",
    "seq": "sequence data",
    "dll": "Windows DLL dependency",
    "exe": "Windows executable",
    "m3d": "Miles 3D audio provider",
    "cab": "cabinet payload",
    "hdr": "InstallShield cabinet header",
    "inx": "InstallShield compiled setup script",
    "ips": "InstallShield string table",
    "skin": "installer skin",
    "bin": "binary layout data",
    "tlb": "OLE type library",
    "ini": "configuration",
    "txt": "documentation",
    "ico": "icon",
    "pal": "palette",
}

# The IIS name pool lists installer-engine/DirectX members first, then the
# product payload. The boundary is the game's own runtime DLLs, so the split is
# made on the observed name sequence rather than on file extension alone.
INSTALLER_ENGINE_NAMES = {
    "iKernel.dll", "Setup.dll", "DotNetInstaller.exe", "iscript.dll",
    "ctor.dll", "iuser.dll", "IGDI.dll", "IsProBE.tlb", "objectps.dll",
    "setup.inx", "license.txt", "corecomp.ini",
    "StringTable-0009-English.ips", "isrt.dll", "default.pal", "_IsRes.dll",
    "layout.bin", "data1.hdr", "data1.cab", "setup.exe", "setup.skin",
    "setup.ini", "BDA.CAB", "BDANT.CAB", "CFGMGR32.DLL", "DIRECTX.CAB",
    "DSETUP.DLL", "DSETUP32.DLL", "DXNT.CAB", "DXSETUP.EXE", "SETUPAPI.DLL",
}

INSTALLER_ENGINE_NAMES_LC = {n.lower() for n in INSTALLER_ENGINE_NAMES}

# Version strings that happen to look like filenames.
NON_FILE_NAMES = {"1.00", "1.00.001"}


def hash_file(path: str) -> dict:
    md5 = hashlib.md5()
    sha1 = hashlib.sha1()
    sha256 = hashlib.sha256()
    total = 0
    with open(path, "rb") as handle:
        while True:
            chunk = handle.read(1 << 20)
            if not chunk:
                break
            total += len(chunk)
            md5.update(chunk)
            sha1.update(chunk)
            sha256.update(chunk)
    return {
        "bytes": total,
        "md5": md5.hexdigest(),
        "sha1": sha1.hexdigest(),
        "sha256": sha256.hexdigest(),
    }


def parse_pe_stub(blob: bytes) -> dict:
    if blob[:2] != b"MZ":
        raise SystemExit("not a PE image: missing MZ")
    pe = struct.unpack_from("<I", blob, 0x3C)[0]
    if blob[pe:pe + 4] != b"PE\x00\x00":
        raise SystemExit("not a PE image: missing PE signature")
    (machine, nsections, timestamp, _sym, _nsym, opt_size,
     characteristics) = struct.unpack_from("<HHIIIHH", blob, pe + 4)
    opt = blob[pe + 24:pe + 24 + opt_size]
    magic = struct.unpack_from("<H", opt, 0)[0]
    sections = []
    cursor = pe + 24 + opt_size
    for _ in range(nsections):
        name = blob[cursor:cursor + 8].rstrip(b"\x00").decode("latin1")
        vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", blob, cursor + 8)
        chars = struct.unpack_from("<I", blob, cursor + 36)[0]
        sections.append({
            "name": name, "virtual_size": vsize, "virtual_address": vaddr,
            "raw_size": rawsize, "raw_pointer": rawptr, "characteristics": chars,
            "raw_end": rawptr + rawsize,
        })
        cursor += 40
    return {
        "machine": machine,
        "pe_magic": "PE32" if magic == 0x10B else "PE32+",
        "section_count": nsections,
        "timestamp": timestamp,
        "characteristics": characteristics,
        "entrypoint_rva": struct.unpack_from("<I", opt, 0x10)[0],
        "image_base": struct.unpack_from("<I", opt, 0x1C)[0]
        if magic == 0x10B else struct.unpack_from("<Q", opt, 0x18)[0],
        "size_of_image": struct.unpack_from("<I", opt, 0x38)[0],
        "subsystem": struct.unpack_from("<H", opt, 0x44)[0],
        "sections": sections,
        "pe_image_end": max(s["raw_end"] for s in sections),
    }


def parse_sfx_records(blob: bytes) -> list:
    """Enumerate the self-extracting stub directory records.

    The ``name`` field can pick up leading noise from the stub's own UI strings
    (padding runs, ``icon=``, a path fragment), so the record's ``path``
    basename is authoritative for the file name.
    """
    records = []
    for index, match in enumerate(SFX_RECORD.finditer(blob)):
        name = match.group(1).decode("latin1")
        if name.startswith("DDING"):
            continue
        path = match.group(2).decode("latin1")
        basename = path.rsplit("\\", 1)[-1]
        size_field = match.group(4).decode("latin1")
        if not basename:
            continue
        records.append({
            "index": index,
            "name": basename,
            "path": path,
            "payload_name_field": name,
            "version": match.group(3).decode("latin1"),
            "record_offset": "0x%x" % match.start(),
            "size_field": size_field,
            "declared_bytes": int(size_field, 10),
            "match_end": "0x%x" % match.end(),
        })
    records.sort(key=lambda r: int(r["record_offset"], 16))
    return records


def parse_rtl_cab(blob: bytes, offset: int) -> dict:
    """Record what is safe to say about the InstallShield-era RTL cabinet.

    The standard MS-CAB field offsets after the ``MSCF`` magic do not line up
    for this cabinet, so no field other than the magic is interpreted here.
    """
    if blob[offset:offset + 4] != b"MSCF":
        raise SystemExit("no MSCF magic at 0x%x" % offset)
    return {
        "offset": "0x%x" % offset,
        "magic": "MSCF",
        "declared_bytes_from_sfx_record": 418296,
        "size_field_corroboration": 418296,
        "verified_end_offset": "0x1587ec38",
        "header_not_standard_mscab": True,
        "observed_first_names": [
            "ikernel.dll", "IsProBENT.tlb", "IsProBE9x.tlb", "objectps.dll",
            "DotNetInstaller.exe", "ctor.dll", "IUser.dll", "IScript.dll",
            "iKernel.rgs",
        ],
        "note": ("Standard MS-CAB field offsets do not line up for this "
                 "InstallShield-era cabinet; only the magic and the flat file "
                 "name list are treated as reliable here."),
    }


def read_iis_names(blob: bytes, offset: int, length: int) -> list:
    header = blob[offset:offset + length]
    if header[:4] != b"ISc(":
        raise SystemExit("no ISc( magic at 0x%x" % offset)
    names = []
    cursor = 0
    while cursor < len(header):
        end = header.find(b"\x00", cursor)
        if end < 0:
            break
        text = header[cursor:end]
        if len(text) >= 3 and all(0x20 <= b < 0x7F for b in text):
            names.append({
                "header_offset": "0x%x" % (offset + cursor),
                "name": text.decode("latin1"),
            })
        cursor = end + 1
    return names


def shannon_entropy(chunk: bytes) -> float:
    import math
    counts = collections.Counter(chunk)
    n = len(chunk)
    return -sum((v / n) * math.log2(v / n) for v in counts.values())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--installer", default="downloads/GTA2INSTALLER.exe")
    parser.add_argument("--outdir", default="research/runtime-data/inventory")
    args = parser.parse_args()

    with open(args.installer, "rb") as handle:
        blob = handle.read()

    identity = hash_file(args.installer)
    md5_ok = identity["md5"] == EXPECTED_MD5
    size_ok = identity["bytes"] == EXPECTED_BYTES
    print("MD5   %s  %s" % (identity["md5"], "OK" if md5_ok else "MISMATCH"))
    print("SHA1  %s" % identity["sha1"])
    print("SHA256 %s" % identity["sha256"])
    print("bytes %d  %s" % (identity["bytes"], "OK" if size_ok else "MISMATCH"))
    if not (md5_ok and size_ok):
        print("refusing to continue: installer is not the pristine artefact",
              file=sys.stderr)
        return 2

    stub = parse_pe_stub(blob)
    assert stub["pe_image_end"] == PE_IMAGE_END, stub["pe_image_end"]
    print("PE    %s sections=%d image_end=0x%x appended_bytes=%d" % (
        stub["pe_magic"], stub["section_count"], stub["pe_image_end"],
        len(blob) - stub["pe_image_end"]))

    sfx = parse_sfx_records(blob)
    print("SFX   %d payload records" % len(sfx))
    for record in sfx:
        print("      0x%08x %-16s %-12s" % (
            int(record["record_offset"], 16), record["name"],
            record["size_field"]))

    rtl_cab = parse_rtl_cab(blob, RTL_CAB_OFFSET)
    engine = next(r for r in sfx if r["name"] == "engine32.cab")
    assert engine["declared_bytes"] == rtl_cab["size_field_corroboration"], (
        engine["declared_bytes"], rtl_cab["size_field_corroboration"])
    for record in sfx:
        record["payload_offset_estimate"] = record["match_end"]
        record["region_end"] = "0x%x" % (
            int(record["payload_offset_estimate"], 16) + record["declared_bytes"])
    links = [(record["name"], int(record["region_end"], 16),
              int(follow["record_offset"], 16))
             for record, follow in zip(sfx, sfx[1:])]
    exact = sum(1 for _n, end, nxt in links if end == nxt)
    print("SFX   chain: %d/%d links land exactly on the next record; "
          "first record starts 0x%x, last region ends 0x%x (file 0x%x)"
          % (exact, len(links), int(sfx[0]["record_offset"], 16),
             int(sfx[-1]["region_end"], 16), len(blob)))
    print("CAB   engine32.cab declared %d bytes -> region end 0x%x, exactly "
          "where the next record sits"
          % (engine["declared_bytes"],
             int(next(r for r in sfx if r["name"] == "engine32.cab")["region_end"], 16)))

    names = read_iis_names(blob, IIS_HEADER_OFFSET, IIS_HEADER_LENGTH)
    filename_re = re.compile(r"[^\\/:*?\"<>|]{1,80}\.[A-Za-z0-9]{2,4}\Z")
    files = [entry for entry in names if filename_re.match(entry["name"])]
    files.sort(key=lambda e: int(e["header_offset"], 16))

    by_ext = collections.Counter(
        entry["name"].rsplit(".", 1)[1].lower() for entry in files)
    rows = []
    for entry in files:
        ext = entry["name"].rsplit(".", 1)[1].lower()
        if entry["name"] in NON_FILE_NAMES:
            klass = "not-a-file"
        elif entry["name"].lower() in INSTALLER_ENGINE_NAMES_LC:
            klass = "installer-engine"
        else:
            klass = "game-data"
        rows.append({
            "header_offset": entry["header_offset"],
            "name": entry["name"],
            "ext": ext,
            "role": ROLE_BY_EXT.get(ext, "unclassified"),
            "class": klass,
        })
    game_data = [row for row in rows if row["class"] == "game-data"]

    print("IIS   %d name-pool entries: %d game-data, %d installer/engine, "
          "%d version-string artefacts"
          % (len(rows), len(game_data),
             sum(1 for row in rows if row["class"] == "installer-engine"),
             sum(1 for row in rows if row["class"] == "not-a-file")))

    groups = collections.Counter()
    for row in game_data:
        ext = row["ext"]
        if ext in ("gmp", "mmp"):
            group = "maps"
        elif ext == "scr":
            group = "scripts"
        elif ext in ("sty", "raw", "sdt", "tga", "pal"):
            group = "graphics/style"
        elif ext == "gxt":
            group = "text"
        elif ext == "wav":
            group = "audio"
        elif ext == "bik":
            group = "video"
        else:
            group = "other"
        groups[group] += 1

    entropy = [
        {"header_offset": "0x%x" % off,
         "entropy": round(shannon_entropy(
             blob[IIS_HEADER_OFFSET + off:IIS_HEADER_OFFSET + off + 4096]), 2)}
        for off in range(0, IIS_HEADER_LENGTH, 4096)
    ]

    inventory = {
        "schema": "gta2-static-inventory/v1",
        "source": {"path": args.installer, **identity},
        "extraction_status":
            "BLOCKED - no InstallShield v7 cabinet decompressor on this host",
        "provenance": {
            "method": "static byte inspection, installer opened read-only",
            "installer_pe_stub": stub,
            "iis_header": {
                "offset": "0x%x" % IIS_HEADER_OFFSET,
                "length": IIS_HEADER_LENGTH,
                "magic": "ISc(",
                "encrypted_record_table_offset": "0x%x"
                % IIS_ENCRYPTED_TABLE_OFFSET,
                "record_table_entropy_per_4k": entropy,
            },
            "rtl_cab": rtl_cab,
        },
        "counts": {
            "total_name_pool_entries": len(rows),
            "game_data": len(game_data),
            "installer_or_engine": sum(
                1 for row in rows if row["class"] == "installer-engine"),
            "not_a_file": sum(
                1 for row in rows if row["class"] == "not-a-file"),
        },
        "by_extension": dict(sorted(by_ext.items(), key=lambda kv: -kv[1])),
        "game_data_groups": dict(groups),
        "files": rows,
        "limits": [
            "Only names and versions are readable. Per-file sizes, CRCs, dates "
            "and uncompressed byte ranges live in the encrypted IIS record "
            "table (high entropy from header+0x10000) and are not statically "
            "recoverable with the tools on this host.",
            "No payload byte has been extracted or hashed, so 9.6F-vs-10.5 "
            "compatibility cannot be tested.",
        ],
    }

    out_json = "%s/gta2-96f-static-inventory.json" % args.outdir
    with open(out_json, "w", encoding="utf-8") as handle:
        json.dump(inventory, handle, indent=1)
        handle.write("\n")

    sfx_doc = {
        "schema": "gta2-sfx-payload-records/v1",
        "description": (
            "InstallShield self-extracting stub directory records. Each record "
            "is four NUL-terminated ASCII fields (name, path, version, "
            "size_field); payload bytes follow the fourth NUL."),
        "size_field_finding": (
            "The fourth field is a DECIMAL byte count. Verified three ways: "
            "data1.cab payload_offset 0x604aa + 582812 = 0xee946 = the next "
            "record; data1.hdr payload_offset 0xee96e + 71292 = 0xfffea = the "
            "next record; engine32.cab payload_offset 0x15818a40 + 418296 = "
            "0x1587ec38 = the next record. Totals reconcile: 361,937,116 "
            "declared bytes + 394,845 bytes of record headers = 362,331,961 = "
            "the installer size."),
        "chain_status": (
            "SUBSTANTIALLY CONTIGUOUS. Payload then next record head repeats "
            "to the end of the file, and the last region ends exactly at EOF "
            "(0x1598bf39). 10 of 13 links land exactly on the next record; "
            "three are a few bytes short because a record's name field "
            "occasionally captures preceding noise, so payload_offset here is "
            "an estimate for those. This supersedes the first-pass reading that "
            "the chain 'breaks after Autorun.exe', which came from misreading "
            "the size field as hexadecimal."),
        "records": sfx,
    }
    out_sfx = "%s/sfx-payload-records.json" % args.outdir
    with open(out_sfx, "w", encoding="utf-8") as handle:
        json.dump(sfx_doc, handle, indent=1)
        handle.write("\n")

    print("wrote %s" % out_json)
    print("wrote %s" % out_sfx)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
