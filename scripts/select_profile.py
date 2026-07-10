#!/usr/bin/env python3
"""Select a KFI build profile from probe_device.sh output."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys


PROFILE_BY_KERNEL = (
    ((6, 12), "android16-6.12"),
    ((6, 6), "android15-6.6"),
    ((6, 1), "android14-6.1"),
    ((5, 15), "android13-5.15"),
    ((5, 10), "android12-5.10"),
)


def parse_report(path: pathlib.Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("[") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip()
    return values


def kernel_pair(release: str) -> tuple[int, int]:
    match = re.match(r"^(\d+)\.(\d+)(?:\.|-|$)", release)
    if match is None:
        raise ValueError(f"cannot parse kernel release: {release!r}")
    return int(match.group(1)), int(match.group(2))


def select_profile(release: str) -> str:
    pair = kernel_pair(release)
    for expected, profile in PROFILE_BY_KERNEL:
        if pair == expected:
            return profile
    raise ValueError(f"unsupported kernel family: {pair[0]}.{pair[1]}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=pathlib.Path)
    parser.add_argument("--format", choices=("name", "json"), default="name")
    parser.add_argument("--require-arm64", action="store_true")
    args = parser.parse_args()

    try:
        values = parse_report(args.report)
        release = values["uname_r"]
        machine = values.get("uname_m", "unavailable")
        if args.require_arm64 and machine not in {"aarch64", "arm64"}:
            raise ValueError(f"target is not ARM64: {machine!r}")
        profile = select_profile(release)
    except (OSError, KeyError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    if args.format == "name":
        print(profile)
    else:
        print(
            json.dumps(
                {
                    "profile": profile,
                    "kernel_release": release,
                    "machine": machine,
                    "page_size": values.get("page_size", "unavailable"),
                    "android_sdk": values.get(
                        "ro.build.version.sdk", "unavailable"
                    ),
                },
                indent=2,
                sort_keys=True,
            )
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
