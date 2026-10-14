#!/usr/bin/env python3
"""GTA2 browser port check entry point (D02).

Usage:
    python3 scripts/check_port.py --suite <name>

Suites: numeric, assets, save, render, slice, network, campaign, perf, release.

Missing suites exit nonzero with a precise message; no green placeholder tests.
"""
import argparse
import sys

SUITES = (
    "numeric", "assets", "save", "render", "slice",
    "network", "campaign", "perf", "release",
)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Run a GTA2 browser port test suite.")
    parser.add_argument("--suite", required=True, help="Suite name.")
    args = parser.parse_args(argv)

    if args.suite not in SUITES:
        print(
            f"check_port: unknown suite {args.suite!r}; expected one of {list(SUITES)}",
            file=sys.stderr,
        )
        return 2

    print(
        f"check_port: suite={args.suite}: not implemented — "
        "no port suites yet; the asset-free math probe remains "
        "scripts/run_gta2_math_probe.py.",
        file=sys.stderr,
    )
    return 2


if __name__ == "__main__":
    sys.exit(main())
