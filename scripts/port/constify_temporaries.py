#!/usr/bin/env python3
"""Add `const` to reference parameters that callers bind temporaries to.

MSVC lets a temporary bind to a non-const lvalue reference; clang does not.
Reads a probe log (clang output), finds every "passing argument to parameter
'p' here" note that follows a "cannot bind to a temporary" error, and makes
that parameter `const T&` in every declaration and definition of the same
function in research/port-work/core/Source. Functions that really write
through the parameter then fail to compile and are fixed by hand.

Usage: constify_temporaries.py <probe.log>   (prints what it changed)
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / 'research/port-work/core/Source'
ERR = re.compile(r"non-const lvalue reference to type '[^']+' cannot bind to a temporary|binding reference of type '[^']+' to value of type 'const [^']+' drops 'const' qualifier")
TYPED_REF = re.compile(r'^\s*(?:struct\s+|class\s+)?[A-Z][\w:]*(?:<[^>]*>)?\s*&\s*(?:[A-Za-z_]\w*)?\s*(?:=[^,]*)?$', re.S)
NOTE = re.compile(r"^(.*?):(\d+):(\d+): note: passing argument to parameter '(\w+)' here")
LVALUE = re.compile(r"^(.*?):(\d+):(\d+): note: candidate function not viable: (?:expects an lvalue for (\d+)(?:st|nd|rd|th) argument|(\d+)(?:st|nd|rd|th) argument \('const [^']+'\) would lose const qualifier)")
NAME = re.compile(r'(operator\s*[^\s(]+|[A-Za-z_]\w*)\s*$')


def read(p):
    return p.read_text(encoding='latin-1')


def param_index(text, open_paren, col_pos):
    depth, idx = 0, 0
    for ch in text[open_paren + 1:col_pos]:
        if ch in '(<[':
            depth += 1
        elif ch in ')>]':
            depth -= 1
        elif ch == ',' and depth == 0:
            idx += 1
    return idx


def split_params(text, open_paren):
    """Return [(start, end)] spans of the top-level parameters and the close index."""
    depth, start, spans = 0, open_paren + 1, []
    for i in range(open_paren + 1, len(text)):
        ch = text[i]
        if ch in '(<[':
            depth += 1
        elif ch in ')>]':
            if depth == 0:
                spans.append((start, i))
                return spans, i
            depth -= 1
        elif ch == ',' and depth == 0:
            spans.append((start, i))
            start = i + 1
    return spans, len(text)


def constify_span(text, a, b):
    seg = text[a:b]
    if '&' not in seg or re.search(r'\bconst\b', seg.split('&')[0]):
        return text, False
    m = re.match(r'(\s*)', seg)
    new = seg[:m.end()] + 'const ' + seg[m.end():]
    return text[:a] + new + text[b:], True


def main():
    log = Path(sys.argv[1]).read_text(errors='replace').splitlines()
    targets = set()
    for i, line in enumerate(log):
        if not ERR.search(line):
            continue
        for nxt in log[i + 1:i + 8]:
            m = NOTE.match(nxt)
            if m:
                targets.add((Path(m.group(1)).resolve(), int(m.group(2)), int(m.group(3))))
                break
    funcs = set()
    for line in log:
        m = LVALUE.match(line)
        if not m:
            continue
        path = Path(m.group(1)).resolve()
        text = read(path)
        lines = text.split('\n')
        pos = sum(len(l) + 1 for l in lines[:int(m.group(2)) - 1]) + int(m.group(3)) - 1
        n = re.match(r'(operator\s*[^\s(]+?|[A-Za-z_]\w*)\s*\(', text[pos:pos + 80])
        if n:
            funcs.add((n.group(1), int(m.group(4) or m.group(5)) - 1))
    for path, line, col in sorted(targets):
        text = read(path)
        lines = text.split('\n')
        pos = sum(len(l) + 1 for l in lines[:line - 1]) + col - 1
        open_paren = text.rfind('(', 0, pos)
        m = NAME.search(text[:open_paren])
        if not m:
            continue
        funcs.add((m.group(1), param_index(text, open_paren, pos)))
    changed = 0
    for path in sorted(SRC.glob('*.[ch]pp')):
        text = read(path)
        orig = text
        for name, idx in funcs:
            pat = re.escape(name) + r'\s*\(' if name.startswith('operator') else r'\b' + re.escape(name) + r'\s*\('
            for m in reversed(list(re.finditer(pat, text))):
                spans, close = split_params(text, m.end() - 1)
                if idx >= len(spans):
                    continue
                # Only `ClassType& name` / `ClassType&` (declarations and
                # definitions); calls such as f(flags & 1) never match.
                if not TYPED_REF.match(text[spans[idx][0]:spans[idx][1]]):
                    continue
                text, did = constify_span(text, *spans[idx])
                changed += did
        if text != orig:
            path.write_text(text, encoding='latin-1')
            print('updated', path.name)
    print(f'{len(funcs)} functions, {changed} parameters made const')


if __name__ == '__main__':
    main()
