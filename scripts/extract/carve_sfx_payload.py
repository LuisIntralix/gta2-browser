# Carve the InstallShield self-extractor payload records out of the installer.
# Usage: carve_sfx_payload.py <installer> <sfx-payload-records.json> <out-dir>
import json, sys, os
inst, recs, out = sys.argv[1:4]
d = json.load(open(recs))
with open(inst, 'rb') as f:
    for r in d['records']:
        off = int(r['payload_offset_estimate'], 16); n = r['declared_bytes']
        f.seek(off); b = f.read(n)
        name = os.path.basename(r['path'].replace('\\', '/'))
        open(os.path.join(out, name), 'wb').write(b)
        print(f"{name:14} {n:>11} magic={b[:4]!r}")
