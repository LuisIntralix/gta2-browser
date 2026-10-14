#!/usr/bin/env python3
"""Checksum-gated, static REA pilot. Does not execute or modify GTA2 files."""

import hashlib
import json
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
NODE = "node"
REA = ROOT / "node_modules/rea-agents/scripts/rea.mjs"
TARGET = ROOT / "downloads/GTA2INSTALLER.exe"
EXPECTED_MD5 = "764848b25aed87b69da472f18c19580d"


def main():
    started = datetime.now(timezone.utc)
    out = ROOT / "docs/evidence" / started.strftime("rea-pilot-%Y%m%dT%H%M%SZ")
    out.mkdir(parents=True, exist_ok=False)
    hashes = {name: hashlib.new(name) for name in ("md5", "sha1", "sha256")}
    with TARGET.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            for digest in hashes.values():
                digest.update(chunk)
    identity = {name: digest.hexdigest() for name, digest in hashes.items()}
    if identity["md5"] != EXPECTED_MD5:
        raise SystemExit("Installer checksum mismatch: analysis refused")
    summary = {
        "pilot_id": out.name,
        "started_utc": started.isoformat(),
        "target": str(TARGET),
        "target_bytes": TARGET.stat().st_size,
        "hashes": identity,
        "scope": "static installer inspection only; no game execution",
        "token_method": "ceil(UTF-8 characters / 4), rough text-size proxy, not billed usage",
        "runs": [],
    }
    commands = [
        ("native-overview", ["inspect", str(TARGET), "--json"]),
        ("pe-resources", ["inspect-pe-resources", str(TARGET),
                          "--max-file-bytes", "536870912", "--json"]),
    ]
    for name, args in commands:
        command = [NODE, str(REA), *args]
        before = time.perf_counter()
        result = subprocess.run(command, cwd=ROOT, capture_output=True, timeout=120)
        seconds = time.perf_counter() - before
        (out / f"{name}.json").write_bytes(result.stdout)
        (out / f"{name}.stderr.txt").write_bytes(result.stderr)
        decoded = result.stdout.decode("utf-8", errors="replace")
        record = {
            "command": command,
            "exit_code": result.returncode,
            "wall_seconds": round(seconds, 4),
            "stdout_bytes": len(result.stdout),
            "stdout_characters": len(decoded),
            "rough_text_tokens": (len(decoded) + 3) // 4,
            "stdout_sha256": hashlib.sha256(result.stdout).hexdigest(),
            "output_file": str(out / f"{name}.json"),
        }
        try:
            payload = json.loads(decoded)
            record["top_level_keys"] = list(payload) if isinstance(payload, dict) else []
        except json.JSONDecodeError:
            record["valid_json"] = False
        summary["runs"].append(record)
        print(json.dumps(record), flush=True)
    with TARGET.open("rb") as source:
        after_md5 = hashlib.file_digest(source, "md5").hexdigest()
    summary["post_test_md5"] = after_md5
    summary["installer_unchanged"] = after_md5 == EXPECTED_MD5
    summary["finished_utc"] = datetime.now(timezone.utc).isoformat()
    (out / "measurements.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({"measurements": str(out / "measurements.json"),
                      "installer_unchanged": summary["installer_unchanged"]}))


if __name__ == "__main__":
    main()
