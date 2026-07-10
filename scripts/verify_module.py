#!/usr/bin/env python3
"""Validate a KFI external-module artifact before deployment."""

from __future__ import annotations

import argparse
import pathlib
import re
import shutil
import subprocess
import sys
from typing import Iterable


DEFAULT_DENIED_SYMBOLS = {"kallsyms_lookup_name"}


def run(tool: str, *arguments: str) -> str:
    executable = shutil.which(tool)
    if executable is None:
        raise RuntimeError(f"required tool not found: {tool}")
    completed = subprocess.run(
        [executable, *arguments],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if completed.returncode != 0:
        detail = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(f"{tool} failed: {detail}")
    return completed.stdout


def header_value(header: str, label: str) -> str:
    match = re.search(rf"^\s*{re.escape(label)}:\s*(.+?)\s*$", header, re.MULTILINE)
    if match is None:
        raise RuntimeError(f"readelf header has no {label!r} field")
    return match.group(1)


def undefined_symbols(symbol_table: str) -> set[str]:
    result: set[str] = set()
    for line in symbol_table.splitlines():
        fields = line.split()
        if "UND" not in fields or len(fields) < 8:
            continue
        name = fields[-1].split("@", 1)[0]
        if name:
            result.add(name)
    return result


def load_symbol_list(path: pathlib.Path) -> set[str]:
    symbols: set[str] = set()
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.split("#", 1)[0].strip()
        if line:
            symbols.add(line)
    return symbols


def relocation_count(relocations: str) -> int:
    return sum(
        1
        for line in relocations.splitlines()
        if re.match(r"^\s*[0-9a-fA-F]+\s+", line)
    )


def sorted_csv(values: Iterable[str]) -> str:
    return ", ".join(sorted(values))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("module", type=pathlib.Path)
    parser.add_argument("--expected-machine", default="AArch64")
    parser.add_argument("--expected-abi", default="1.1")
    parser.add_argument("--allow-symbol-list", type=pathlib.Path)
    parser.add_argument("--deny-symbol", action="append", default=[])
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    module = args.module.resolve()
    if not module.is_file():
        print(f"error: module does not exist: {module}", file=sys.stderr)
        return 2

    try:
        header = run("readelf", "-h", str(module))
        symbols_text = run("readelf", "-Ws", str(module))
        relocations = run("readelf", "-r", str(module))
        machine = header_value(header, "Machine")
        elf_type = header_value(header, "Type")
        elf_class = header_value(header, "Class")
        vermagic = run("modinfo", "-F", "vermagic", str(module)).strip()
        abi = run("modinfo", "-F", "kfi_abi", str(module)).strip()
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    errors: list[str] = []
    if args.expected_machine.lower() != "any" and args.expected_machine.lower() not in machine.lower():
        errors.append(
            f"machine mismatch: expected {args.expected_machine!r}, found {machine!r}"
        )
    if "REL" not in elf_type:
        errors.append(f"ELF is not relocatable: {elf_type}")
    if elf_class != "ELF64":
        errors.append(f"unexpected ELF class: {elf_class}")
    if not vermagic:
        errors.append("module has no vermagic")
    if abi != args.expected_abi:
        errors.append(f"KFI ABI mismatch: expected {args.expected_abi!r}, found {abi!r}")

    undefined = undefined_symbols(symbols_text)
    denied = DEFAULT_DENIED_SYMBOLS | set(args.deny_symbol)
    denied_found = undefined & denied
    if denied_found:
        errors.append(f"denied undefined symbols: {sorted_csv(denied_found)}")

    if args.allow_symbol_list is not None:
        allowed = load_symbol_list(args.allow_symbol_list)
        unexpected = undefined - allowed
        if unexpected:
            errors.append(f"symbols absent from allowlist: {sorted_csv(unexpected)}")

    print(f"module={module}")
    print(f"machine={machine}")
    print(f"class={elf_class}")
    print(f"type={elf_type}")
    print(f"vermagic={vermagic}")
    print(f"kfi_abi={abi}")
    print(f"undefined_symbol_count={len(undefined)}")
    print(f"relocation_count={relocation_count(relocations)}")

    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1

    print("result=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
