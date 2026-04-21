#!/usr/bin/env python3
"""
static_ram_check.py  —  compile-time RAM budget verifier
Reads GCC .su files + ELF map to verify static + stack RAM fits in budget.

Usage:
    python static_ram_check.py \
        --elf   build/my_app.elf \
        --sudir build/CMakeFiles/my_target.dir \
        --ram   262144            # total RAM in bytes (256 KB for NRF5240)
        --stack 8192              # stack budget in bytes
"""

"./_build/simulator/CMakeFiles/simulator_indexable.dir"

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



def get_elf_symbols(elf: Path) -> set[str]:
    """Extract all defined function symbols from the final ELF."""
    out = subprocess.check_output(
        ["nm", "--defined-only", "--demangle", str(elf)],
        text=True
    )
    symbols = set()
    for line in out.splitlines():
        # nm output: [address] [size] [type] [name]
        # We only want code symbols (type = T or t = .text section)
        parts = line.strip().split(None, 3)
        if len(parts) >= 3 and parts[-2].lower() in ('t', 'w'):
            symbols.add(parts[-1])
    return symbols

# ── Enum value resolver ───────────────────────────────────────────────

def build_enum_map(elf: Path) -> dict[tuple[str, int], str]:
    """
    Parse DWARF info to build {(EnumTypeName, int_value): 'ENUMERATOR_NAME'}.
    Lets us convert nm's  (DisplayMode)0  back to  DisplayMode::RAMP.
    """
    enum_map: dict[tuple[str, int], str] = {}
    try:
        out = subprocess.check_output(
            ["readelf", "--debug-dump=info", str(elf)],
            text=True, stderr=subprocess.DEVNULL
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        return enum_map

    current_enum: str | None = None
    enumerator_name: str | None = None
    for line in out.splitlines():
        if re.search(r'DW_TAG_enumeration_type', line):
            current_enum = None
        m = re.search(r'DW_AT_name\s*:.*?:\s*(\w+)', line)
        if m:
            if current_enum is None:
                current_enum = m.group(1)
            else:
                enumerator_name = m.group(1)
        m = re.search(r'DW_AT_const_value\s*:\s*(\d+)', line)
        if m and current_enum and enumerator_name:
            enum_map[(current_enum, int(m.group(1)))] = enumerator_name
    return enum_map


def resolve_nm_symbol(sym: str, enum_map: dict) -> str:
    """Convert nm's '(DisplayMode)0' back to 'DisplayMode::RAMP'."""
    def replace_cast(m):
        resolved = enum_map.get((m.group(1), int(m.group(2))))
        return f"{m.group(1)}::{resolved}" if resolved else m.group(0)
    return re.sub(r'\((\w+)\)(\d+)', replace_cast, sym)


# ── Symbol set from ELF ───────────────────────────────────────────────

def normalise(name: str) -> str:
    """Strip return type, collapse whitespace, lowercase."""
    name = re.sub(r'^(?:[\w:*&<>]+\s+)+(?=\w)', '', name.strip())
    name = re.sub(r'\s+', '', name).lower()
    return name


def build_elf_symbol_set(elf: Path) -> set[str]:
    """
    Return normalised symbol names from the final ELF,
    with enum template params resolved via DWARF debug info.
    """
    out = subprocess.check_output(
        ["nm", "--defined-only", "--demangle", str(elf)],
        text=True
    )
    enum_map = build_enum_map(elf)
    symbols = set()
    for line in out.splitlines():
        parts = line.strip().split(None, 3)
        if len(parts) >= 3 and parts[-2].lower() in ('t', 'w'):
            symbols.add(normalise(resolve_nm_symbol(parts[-1], enum_map)))
    return symbols

def filter_live_entries(entries: list, elf: Path):
    elf_symbols = build_elf_symbol_set(elf)
    live, dead = [], []
    for e in entries:
        (live if normalise(e.name) in elf_symbols else dead).append(e)
    return live, dead


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
                    file  = m.group("file"),
                    line  = int(m.group("line")),
                    name  = m.group("name"),
                    size  = int(m.group("size")),
                    kind  = m.group("kind"),
                ))
    return entries

# ─────────────────────────────────────────────
# ELF static RAM (.data + .bss) via readelf
# ─────────────────────────────────────────────

def get_static_ram(elf: Path) -> int:
    """Sum the sizes of .data and .bss sections from the ELF."""
    out = subprocess.check_output(
        ["readelf", "-S", "--wide", str(elf)],
        text=True
    )
    total = 0
    for line in out.splitlines():
        parts = line.split()
        # readelf -S columns: [Nr] Name Type Addr Off Size ES Flg ...
        # We look for .data and .bss by name
        for section in (".data", ".bss", ".noinit"):
            if section in parts:
                idx = parts.index(section)
                try:
                    # Size is hex, 2 fields after the name
                    total += int(parts[idx + 3], 16)
                except (IndexError, ValueError):
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
    dynamic = [e for e in entries if e.kind != "static"]
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
    entries   = parse_su_files(sudir)
    # entries, dead    = filter_live_entries(entries, elf)
    dead = None
    if dead:
        print(f"  ℹ  Excluded {len(dead)} dead template instantiation(s):")
        for e in dead:
            print(f"       {e.name}")

    static    = get_static_ram(elf)
    depth, chain = worst_case_stack(entries)
    dynamic_fns  = [e.name for e in entries if e.kind == "dynamic"]

    return RamReport(
        static_data    = static,
        max_call_stack = depth,
        total_ram      = total_ram,
        stack_budget   = stack_budget,
        worst_chain    = chain,
        dynamic_fns    = dynamic_fns,
    )

def report_and_assert(r: RamReport) -> bool:
    print("=" * 52)
    print("  Static RAM Analysis")
    print("=" * 52)
    print(f"  Static (.data+.bss) : {r.static_data:>8} bytes")
    print(f"  Worst-case stack    : {r.max_call_stack:>8} bytes")
    print(f"  ─────────────────────────────────────────")
    total_used = r.static_data + r.max_call_stack
    print(f"  Total estimated     : {total_used:>8} bytes")
    print(f"  RAM available       : {r.total_ram:>8} bytes")
    print(f"  Stack budget        : {r.stack_budget:>8} bytes")
    print()

    if r.worst_chain:
        print("  Worst-case call chain (by frame size):")
        for fn in r.worst_chain:
            print(f"    → {fn}")
        print()

    passed = True

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
              f"{r.stack_budget - r.max_call_stack} B headroom)")

# Not working yet: RAM estimation
#    if total_used > r.total_ram:
#        print(f"  ✗ FAIL — total RAM {total_used} B exceeds device RAM {r.total_ram} B")
#        passed = False
#    else:
#        print(f"  ✓ PASS — total RAM within device limit "
#              f"({total_used}/{r.total_ram} B)")

    print("=" * 52)
    return passed

# ─────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(description="Static RAM budget checker for embedded ELF binaries.")
    ap.add_argument("--elf",    required=True,  type=Path, help="Path to compiled .elf")
    ap.add_argument("--sudir",  required=True,  type=Path, help="Directory containing .su files")
    ap.add_argument("--ram",    required=False, type=int,  default='262144', help="Total device RAM in bytes")
    ap.add_argument("--stack",  required=False, type=int,  default='8192', help="Stack budget in bytes")
    args = ap.parse_args()

    r = analyse(args.elf, args.sudir, args.ram, args.stack)
    ok = report_and_assert(r)
    sys.exit(0 if ok else 1)

if __name__ == "__main__":
    main()
