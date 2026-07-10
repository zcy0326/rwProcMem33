#!/usr/bin/env python3
"""Build and verify one profile-specific KFI external module."""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import shlex
import shutil
import subprocess
import sys

try:
    from .select_profile import parse_report, select_profile
except ImportError:
    from select_profile import parse_report, select_profile


PROFILE_PATTERN = re.compile(r"^[a-z0-9][a-z0-9._-]{0,31}$")


def checked_run(command: list[str], *, dry_run: bool) -> None:
    print(shlex.join(command))
    if dry_run:
        return
    completed = subprocess.run(command, check=False)
    if completed.returncode != 0:
        raise RuntimeError(
            f"command failed with exit status {completed.returncode}: {command[0]}"
        )


def resolve_profile(explicit: str | None, report: pathlib.Path | None) -> str:
    if explicit and report:
        raise ValueError("use either --profile or --probe, not both")
    if report:
        values = parse_report(report)
        return select_profile(values["uname_r"])
    if explicit:
        return explicit
    raise ValueError("one of --profile or --probe is required")


def main() -> int:
    repository = pathlib.Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kernel-out", required=True, type=pathlib.Path)
    parser.add_argument("--profile")
    parser.add_argument("--probe", type=pathlib.Path)
    parser.add_argument("--arch", default="arm64")
    parser.add_argument("--jobs", type=int, default=max(1, os.cpu_count() or 1))
    parser.add_argument("--make", default="make")
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--allow-symbol-list", type=pathlib.Path)
    parser.add_argument("--no-llvm", action="store_true")
    parser.add_argument("--skip-verify", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    try:
        profile = resolve_profile(args.profile, args.probe)
        if PROFILE_PATTERN.fullmatch(profile) is None:
            raise ValueError(f"invalid profile name: {profile!r}")
        kernel_out = args.kernel_out.resolve()
        if not args.dry_run and not kernel_out.is_dir():
            raise ValueError(f"kernel build directory does not exist: {kernel_out}")
        if args.jobs <= 0:
            raise ValueError("--jobs must be positive")

        command = [
            args.make,
            "-C",
            str(kernel_out),
            f"M={repository / 'kernel'}",
            f"ARCH={args.arch}",
            f"KFI_PROFILE={profile}",
            f"-j{args.jobs}",
        ]
        if not args.no_llvm:
            command.append("LLVM=1")
        command.append("modules")
        checked_run(command, dry_run=args.dry_run)

        built_module = repository / "kernel" / "kfi.ko"
        output = (
            args.output.resolve()
            if args.output
            else repository / "dist" / profile / "kfi.ko"
        )
        print(f"artifact={output}")
        if args.dry_run:
            return 0
        if not built_module.is_file():
            raise RuntimeError(f"build produced no module: {built_module}")
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(built_module, output)

        if not args.skip_verify:
            verify = [
                sys.executable,
                str(repository / "scripts" / "verify_module.py"),
                str(output),
            ]
            if args.allow_symbol_list:
                verify.extend(
                    ["--allow-symbol-list", str(args.allow_symbol_list.resolve())]
                )
            checked_run(verify, dry_run=False)
    except (KeyError, OSError, RuntimeError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
