# Write a SHA-256 manifest of an extracted game tree (asset profile v1).
# Usage: hash_manifest.py <game-dir> <out.json>
import hashlib, json, os, sys
root, out = sys.argv[1:3]
files = []
for dp, _, fns in os.walk(root):
    for fn in sorted(fns):
        p = os.path.join(dp, fn)
        b = open(p, 'rb').read()
        files.append({"path": os.path.relpath(p, root).replace(os.sep, '/'), "bytes": len(b),
                      "sha256": hashlib.sha256(b).hexdigest(), "magic": b[:4].hex()})
files.sort(key=lambda f: f["path"].lower())
json.dump({"schema": "gta2-asset-profile/v1", "profile": "rockstar-classics-9.6f-en",
           "installer_md5": "764848b25aed87b69da472f18c19580d",
           "extracted_with": "unshield 1.6.2 @ 0d1e7cb (local build)",
           "file_count": len(files), "total_bytes": sum(f["bytes"] for f in files), "files": files},
          open(out, 'w'), indent=1)
print(len(files), sum(f["bytes"] for f in files))
