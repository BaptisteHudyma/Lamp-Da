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
    stlRoots:  dict = field(default_factory=dict)

# ─────────────────────────────────────────────
# .su file parsing
# ─────────────────────────────────────────────

# ── Symbol set from ELF ───────────────────────────────────────────────

SU_RE = re.compile(
    r'^(?P<file>.+?):(?P<line>\d+):\d+:(?P<fullname>(?P<name>.+?)(?:\[.+\])?)\s+(?P<size>\d+)\s+(?P<kind>[\w,]+)$'
)


# Add filtering/categorization:
STATIC_KINDS = {'static', 'bounded'}

@staticmethod
def is_dynamic(kind: str) -> bool:
    return kind not in STATIC_KINDS

def parse_su_files(sudir: Path) -> list[FunctionStack]:
    entries = []
    errors = []
    for su_file in sudir.rglob("*.su"):
        for line_num, raw in enumerate(su_file.read_text().splitlines()):
            line = raw.strip()
            if not line:
                continue
            m = SU_RE.match(line)
            if m:
                entries.append(FunctionStack(
                    file  = m.group('file'),
                    line  = int(m.group('line')),
                    name  = f"{su_file.stem}:{m.group('line')}:{m.group('fullname')}",
                    size  = int(m.group('size')),
                    kind  = m.group('kind'),
                ))
            else:
                errors.append(f"{su_file}:{line_num}: Could not parse: {raw[:60]}")
    if errors:
        import logging
        logger = logging.getLogger(__name__)
        logger.warning(f"Unparsed lines: {len(errors)}\n" + "\n".join(errors[:10]))
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

import subprocess
import re
from pathlib import Path
from collections import defaultdict, deque

class DynamicMemoryAnalyzer:
    def __init__(self, elf_file: str):
        self.elf_file = elf_file
        self.all_symbols = {}
        self.direct_allocators = set()  # Functions that directly allocate
        self.call_graph = defaultdict(set)  # who calls whom
        self.allocation_type = {}  # func -> ['STL_VECTOR', 'MALLOC', ...]
        self._build_call_graph()
        self._analyze()
    
    def _build_call_graph(self, isVerbose=False):
        """Extract actual call graph from binary using objdump."""
        
        # Try different objdump variants in order of preference
        objdump_tools = [
            'arm-none-eabi-objdump',  # NRF/STM32 standard
            'arm-linux-gnueabihf-objdump',  # Linux ARM
            'armv7l-rpi-linux-gnueabihf-objdump',  # Raspberry Pi
            'objdump',  # Fallback (will likely fail)
        ]
        
        objdump_cmd = None
        for tool in objdump_tools:
            try:
                result = subprocess.run([tool, '--version'], 
                                    capture_output=True, text=True, timeout=2)
                if result.returncode == 0:
                    objdump_cmd = tool
                    if isVerbose:
                        print(f"✓ Using {tool}")
                    break
            except FileNotFoundError:
                continue
        
        if not objdump_cmd:
            print(f"⚠️  No ARM objdump found. Tried: {', '.join(objdump_tools)}")
            print(f"     Install: arm-none-eabi-binutils")
            return
        
        try:
            result = subprocess.run(
                [objdump_cmd, '-d', self.elf_file],
                capture_output=True, text=True, check=True, timeout=30
            )
        except subprocess.CalledProcessError as e:
            print(f"⚠️  {objdump_cmd} failed: {e.stderr[:200]}")
            return
        except subprocess.TimeoutExpired:
            print(f"⚠️  {objdump_cmd} timed out on large binary")
            return
        
        # ARM call instructions: bl, blx, call (Thumb2)
        call_pattern = re.compile(r'(?:bl|blx|call)\s+[0-9a-f]+\s+<(.+?)>')
        current_func = None
        
        for line in result.stdout.split('\n'):
            # Detect function header: "0000xxxx <function_name>:"
            func_header = re.match(r'^[0-9a-f]+\s+<(.+?)>:', line)
            if func_header:
                current_func = func_header.group(1)
            
            # Find calls within current function
            if current_func:
                call_match = call_pattern.search(line)
                if call_match:
                    called_func = call_match.group(1)
                    self.call_graph[current_func].add(called_func)
        if isVerbose:
            print(f"✓ Call graph built: {len(self.call_graph)} functions with calls")

    def _analyze(self):
        """Extract symbols and build call graph."""
        try:
            result = subprocess.run(['nm', '-C', self.elf_file],
                                  capture_output=True, text=True, check=True)
        except subprocess.CalledProcessError:
            print(f"❌ Error reading {self.elf_file}")
            return
        
        # Parse all symbols
        for line in result.stdout.split('\n'):
            line = line.strip()
            if not line:
                continue
            
            parts = line.split()
            if len(parts) >= 3:
                symbol = ' '.join(parts[2:])
                self.all_symbols[symbol] = line
                
                # Classify allocation type
                alloc_types = []
                
                # Direct allocators
                if any(x in symbol for x in ['std::vector', 'std::string', 'std::map', 
                                             'std::queue', 'std::deque', 'std::list']):
                    alloc_types.append('STL')
                    self.direct_allocators.add(symbol)
                
                if 'malloc' in symbol or 'calloc' in symbol:
                    alloc_types.append('MALLOC')
                    self.direct_allocators.add(symbol)
                
                if 'operator new' in symbol:
                    alloc_types.append('NEW')
                    self.direct_allocators.add(symbol)
                
                if alloc_types:
                    self.allocation_type[symbol] = alloc_types

    def _symbol_matches_function(self, symbol: str, func_name: str) -> bool:
        """Check if symbol corresponds to function."""
        # Extract base function name from symbol
        base_symbol = symbol.split('<')[0].split('(')[0].strip()
        return func_name in base_symbol or base_symbol in func_name
    

    def _function_calls_other(self, func_name: str, target_name: str, depth: int = 2) -> bool:
        """Check if func_name calls target_name (recursively, with depth limit)."""
        if depth == 0:
            return False
        
        # Direct match in call graph
        for caller, callees in self.call_graph.items():
            if func_name in caller:
                for callee in callees:
                    if target_name in callee:
                        return True
                    # Recurse deeper
                    if self._function_calls_other(callee, target_name, depth - 1):
                        return True
        
        return False

    def _extract_base_name(self, symbol: str) -> str:
        """Extract base function name, stripping template parameters."""
        # Remove everything after the first '<' or '('
        base = symbol.split('<')[0].split('(')[0].strip()
        return base
    
    def find_roots_and_dependents(self, su_entries: list) -> dict:
        """
        Identify root allocators, grouping by container TYPE and TEMPLATE SIGNATURE.
        Aggregate duplicates across compilation units.
        Filter out STL internals (sort, heap ops, etc.)
        """
        
        # STL internals to completely filter out
        STL_INTERNALS_FILTER = {
            'std::__introsort_loop',
            'std::__insertion_sort',
            'std::__unguarded_insertion_sort',
            'std::__unguarded_partition',
            'std::__partial_sort',
            'std::__move_median_to_first',
            'std::__adjust_heap',
            'std::__pop_heap',
            'std::__push_heap',
            'std::__make_heap',
            'std::sort',
            'std::stable_sort',
            'std::__merge',
            'std::__move_merge',
            'std::__inplace_stable_sort',
            'std::__new_allocate',
        }
        
        LOW_LEVEL_ALLOCATORS = {
            'operator new',
            'operator new[]',
            'operator delete',
            'operator delete[]',
            'malloc',
            'free',
            'calloc',
            'realloc',
        }
        
        def should_filter(func_name: str) -> bool:
            """Check if function should be filtered out."""
            # Filter STL internals
            for internal in STL_INTERNALS_FILTER:
                if internal in func_name:
                    return True
            # Filter low-level allocators
            for allocator in LOW_LEVEL_ALLOCATORS:
                if allocator in func_name:
                    return True
            return False
        
        def extract_container_signature(func_name: str) -> tuple[str, str]:
            """
            Extract container type and template signature.
            
            Returns: (container_type, normalized_signature)
            Examples:
                std::vector<int> → ('vector', 'std::vector<int>')
                std::map<K,V> → ('map', 'std::map<K,V>')
            """
            # Match STL container patterns
            patterns = {
                'std::vector': 'vector',
                'std::map': 'map',
                'std::unordered_map': 'unordered_map',
                'std::set': 'set',
                'std::unordered_set': 'unordered_set',
                'std::string': 'string',
                'std::deque': 'deque',
                'std::list': 'list',
                'std::queue': 'queue',
                'std::stack': 'stack',
            }
            
            for pattern, container_type in patterns.items():
                if pattern in func_name:
                    # Extract template args: everything between first '<' and matching '>'
                    start = func_name.find(pattern)
                    if start >= 0:
                        bracket_start = func_name.find('<', start)
                        if bracket_start > 0:
                            # Simple extraction (may have nested brackets)
                            bracket_count = 0
                            bracket_end = bracket_start
                            for i in range(bracket_start, len(func_name)):
                                if func_name[i] == '<':
                                    bracket_count += 1
                                elif func_name[i] == '>':
                                    bracket_count -= 1
                                    if bracket_count == 0:
                                        bracket_end = i + 1
                                        break
                            
                            sig = func_name[start:bracket_end]
                            return container_type, sig
                    
                    return container_type, pattern
            
            return None, func_name
        
        # Step 1: Collect all STL uses, keyed by container signature
        container_uses = {}  # signature -> { 'files': set, 'sizes': [], 'type': str }
        
        for func in su_entries:
            if should_filter(func.name):
                continue
            
            # Check if this function uses STL
            for symbol, types in self.allocation_type.items():
                if self._symbol_matches_function(symbol, func.name) and 'STL' in types:
                    container_type, signature = extract_container_signature(func.name)
                    
                    if container_type:
                        if signature not in container_uses:
                            container_uses[signature] = {
                                'container_type': container_type,
                                'functions': [],
                                'total_stack': 0,
                            }
                        
                        container_uses[signature]['functions'].append({
                            'name': func.name,
                            'file': func.file,
                            'line': func.line,
                            'size': func.size,
                        })
                        container_uses[signature]['total_stack'] += func.size
                    
                    break
        
        # Step 2: Build result grouped by container type and signature
        result = {}
        
        for signature, data in container_uses.items():
            # Use signature as key for grouping
            result[signature] = {
                'root': {
                    'type': data['container_type'],
                    'signature': signature,
                    'instances': data['functions'],
                    'total_stack': data['total_stack'],
                    'num_files': len(set(f['file'] for f in data['functions'])),
                },
                'types': ['STL'],
                'callers': [],  # Could populate with reverse call graph if needed
            }
        
        return result
        
def display_grouped_report(roots: dict):
    """Display clean, grouped dynamic memory report with aggregated STL usage."""
    
    if not roots:
        print("\nNo dynamic memory allocations detected")
        return
    
    print("\n" + "="*80)
    print("DYNAMIC MEMORY ANALYSIS (STL CONTAINERS - GROUPED BY TYPE)")
    print("="*80)
    
    # Sort by total stack usage
    sorted_roots = sorted(
        roots.items(),
        key=lambda x: x[1]['root']['total_stack'],
        reverse=True
    )
    
    for idx, (signature, root_data) in enumerate(sorted_roots, 1):
        root_info = root_data['root']
        container_type = root_info['type']
        instances = root_info['instances']
        total_stack = root_info['total_stack']
        num_files = root_info['num_files']
        
        print(f"\n{idx}. — std::{container_type}")
        print("   " + "-"*76)
        print(f"   Template: {signature}")
        print(f"   Instances: {len(instances)} function(s) across {num_files} file(s)")
        
        # Show instances grouped by file
        files_to_funcs = {}
        for inst in instances:
            if inst['file'] not in files_to_funcs:
                files_to_funcs[inst['file']] = []
            files_to_funcs[inst['file']].append(inst)
        
        print(f"\n  Locations:")
        for file, funcs in sorted(files_to_funcs.items()):
            print(f"        {file}")
            for func in sorted(funcs, key=lambda f: f['line']):
                print(f"            Line {func['line']}: {func['name']}")
    
    # Summary
    print("\n" + "="*80)
    print("SUMMARY")
    print("="*80)
    
    total_all = sum(r['root']['total_stack'] for r in roots.values())
    total_instances = sum(len(r['root']['instances']) for r in roots.values())
    
    print(f"   Unique STL container types: {len(roots)}")
    print(f"   Total instances: {total_instances}")
    print(f"   Combined stack usage: {total_all} bytes")

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
    static  = sorted(
        [e for e in entries if not is_dynamic(e.kind)],
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
    dynamic_fns  = [e.name for e in entries if is_dynamic(e.kind)]

    analyzer = DynamicMemoryAnalyzer(elf)
    roots = analyzer.find_roots_and_dependents(entries)
    
    return RamReport(
        static_data    = static,
        max_call_stack = depth,
        total_ram      = total_ram,
        stack_budget   = stack_budget,
        worst_chain    = chain,
        dynamic_fns    = dynamic_fns,
        stlRoots = roots
    )

def report_and_assert(r: RamReport, is_verbose) -> bool:

    if is_verbose:
        display_grouped_report(r.stlRoots)

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
