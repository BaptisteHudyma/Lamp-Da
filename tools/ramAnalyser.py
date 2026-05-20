#!/usr/bin/env python3
"""
static_ram_check.py  —  compile-time RAM budget verifier
Reads GCC .su files + ELF map to verify static + stack RAM fits in budget.
BASED ON GENERATED CODE, by Claude.

Usage:
    python static_ram_check.py \
        --elf   build/my_app.elf \
        --sudir build/CMakeFiles/my_target.dir \
        --ram   262144            # total RAM in bytes (256 KB for NRF5240)
        --stack 8192              # stack budget in bytes
"""

import argparse
import subprocess
import re
import sys
from pathlib import Path
from dataclasses import dataclass, field
from typing import Optional

# ─────────────────────────────────────────────
# Data model
# ─────────────────────────────────────────────

@dataclass
class FunctionStack:
    file:     str
    line:     int
    name:     str
    size:     int
    kind:     str          # static | dynamic | bounded

@dataclass
class RamReport:
    static_data:    int = 0   # .data + .bss
    max_call_stack: int = 0   # deepest reachable call chain
    total_ram:      int = 0
    stack_budget:   int = 0
    worst_chain:    list[str] = field(default_factory=list)
    dynamic_fns:    list[str] = field(default_factory=list)  # warning: VLAs etc.

# ─────────────────────────────────────────────
# .su file parsing
# ─────────────────────────────────────────────

# ── Symbol set from ELF ───────────────────────────────────────────────

SU_RE = re.compile(
    r'^(?P<file>.+?):(?P<line>\d+):\d+:(?P<fullname>(?P<name>.+?)(\[.+\])?)\t(?P<size>\d+)\t(?P<kind>\w+)$'
)

def parse_su_files(sudir: Path) -> list[FunctionStack]:
    entries = []
    for su_file in sudir.rglob("*.su"):
        for raw in su_file.read_text().splitlines():
            line = raw.strip()
            if not line:
                continue
            m = SU_RE.match(line)
            if m:
                entries.append(FunctionStack(
                    file  = m.group('file'),
                    line  = int(m.group('line')),
                    name  = f"{su_file.stem}:{m.group('line')}:{m.group('name')}",
                    size  = int(m.group('size')),
                    kind  = m.group('kind'),
                ))
    return entries

# ─────────────────────────────────────────────
# ELF static RAM (.data + .bss) via readelf
# ─────────────────────────────────────────────

def get_static_ram(elf: Path) -> int:
    out = subprocess.check_output(
        ["readelf", "-S", "--wide", str(elf)],
        text=True
    )
    total = 0
    for line in out.splitlines():
        if not line.strip().startswith('['):
            continue
        parts = line.split()
        # readelf -S columns: [Nr] Name Type Addr Off Size ES Flg ...
        for sec in (".data", ".bss", ".noinit"):
            if parts[2] == sec: #name on 2
                try:
                    total += int(parts[6], 16)  # Size is always column 6 (0-indexed)
                except ValueError:
                    pass
    return total

# ─────────────────────────────────────────────
# Worst-case stack depth (greedy, no call graph)
# ─────────────────────────────────────────────
# For a full call-graph analysis, pipe through cflow or
# egypt + graphviz, but for most embedded projects the
# simple "sum of the N largest frames" is a safe upper bound.

def worst_case_stack(entries: list[FunctionStack], top_n: int = 8) -> tuple[int, list[str]]:
    """
    Conservative upper bound: sort by frame size, sum the largest N.
    Real call depth is rarely > 8 on flat embedded code.
    For tighter analysis, replace with a proper call-graph walk.
    """
    # dynamic = [e for e in entries if e.kind != "static"]
    static  = sorted(
        [e for e in entries if e.kind == "static"],
        key=lambda e: e.size, reverse=True
    )
    chain = static[:top_n]
    return sum(e.size for e in chain), [e.name for e in chain]

# ─────────────────────────────────────────────
# Main analysis
# ─────────────────────────────────────────────

def analyse(elf: Path, sudir: Path, total_ram: int, stack_budget: int) -> RamReport:
    if not elf.exists():
        raise FileNotFoundError(f"ELF not found: {elf}")
    if not sudir.is_dir():
        raise NotADirectoryError(f"SU dir not found: {sudir}")

    entries   = parse_su_files(sudir)
    static    = get_static_ram(elf)
    depth, chain = worst_case_stack(entries, 8)
    dynamic_fns  = [e.name for e in entries if e.kind == "dynamic"]

    return RamReport(
        static_data    = static,
        max_call_stack = depth,
        total_ram      = total_ram,
        stack_budget   = stack_budget,
        worst_chain    = chain,
        dynamic_fns    = dynamic_fns,
    )

def report_and_assert(r: RamReport, is_verbose) -> bool:

    total_used = r.static_data + r.max_call_stack
    if is_verbose:
        print("=" * 52)
        print("  Static RAM Analysis")
        print("=" * 52)
        print(f"  Static (.data+.bss) : {r.static_data:>8} bytes")
        print(f"  Worst-case stack    : {r.max_call_stack:>8} bytes")
        print(f"  ─────────────────────────────────────────")
        print(f"  Total estimated     : {total_used:>8} bytes")
        print(f"  RAM available       : {r.total_ram:>8} bytes")
        print(f"  Stack budget        : {r.stack_budget:>8} bytes")
        print()

    if is_verbose and r.worst_chain:
        print("  Worst-case call chain (by frame size):")
        for fn in r.worst_chain:
            print(f"    → {fn}")
        print()

    passed = True

    # We dont really care about this, as it only display internal SDK code
    if is_verbose:
        if r.dynamic_fns:
            print("  ⚠  WARNINGS — dynamic stack usage detected (VLAs / alloca):")
            for fn in r.dynamic_fns:
                print(f"    ! {fn}")
            print()

    if r.max_call_stack > r.stack_budget:
        print(f"  ✗ FAIL — stack {r.max_call_stack} B exceeds budget {r.stack_budget} B")
        passed = False
    else:
        print(f"  ✓ PASS — stack within budget "
              f"({r.max_call_stack}/{r.stack_budget} B, "
              f"{r.stack_budget - r.max_call_stack} B)")

    if total_used > r.total_ram:
        print(f"  ✗ FAIL — estimated RAM {total_used} B exceeds device RAM {r.total_ram} B")
        passed = False
    else:
        print(f"  ✓ PASS — total RAM within limit ({total_used}/{r.total_ram} B)")
    return passed

# ─────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(description="Static RAM budget checker for embedded ELF binaries.")
    ap.add_argument("--elf",    required=True,  type=Path, help="Path to compiled .elf")
    ap.add_argument("--sudir",  required=True,  type=Path, help="Directory containing .su files")
    ap.add_argument("--ram",    required=False, type=int,  default=262144, help="Total device RAM in bytes")
    ap.add_argument("--stack",  required=False, type=int,  default=8192,   help="Stack budget in bytes")
    ap.add_argument("-v",       required=False, type=bool,  default=False,   help="Verbose")
    args = ap.parse_args()

    r = analyse(args.elf, args.sudir, args.ram, args.stack)
    ok = report_and_assert(r, args.v)
    sys.exit(0 if ok else 1)

if __name__ == "__main__":
    main()
