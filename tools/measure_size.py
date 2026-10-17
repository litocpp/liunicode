#!/usr/bin/env python3
"""Measure the Linux Release consumers in tests/size using LLVM's public tools."""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-dir', type=Path,
                        default=ROOT / 'tests/size/build/release/bin/liunicode-size')
    parser.add_argument('--llvm-bin', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/size')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)

    def tool(name):
        return str(args.llvm_bin / name) if args.llvm_bin else name

    def run(name, *arguments):
        return subprocess.check_output([tool(name), *map(str, arguments)], text=True)

    expected = {'empty': [], 'casing': ['casing'], 'segmentation': ['segmentation'],
                'normalization': ['normalization'],
                'all': ['casing', 'normalization', 'properties', 'segmentation']}
    results = {}
    for name, groups in expected.items():
        source = args.bin_dir / name
        stripped = args.output / name
        shutil.copyfile(source, stripped)
        run('llvm-strip', stripped)
        sections = run('llvm-size', '-A', source)
        sizes = {fields[0]: int(fields[1]) for line in sections.splitlines()
                 if len(fields := line.split()) >= 2 and fields[1].isdigit()}
        symbols = run('llvm-nm', '--demangle', '--print-size', source)
        data = '\n'.join(line for line in symbols.splitlines() if 'liunicode::data_' in line)
        actual = sorted(set(re.findall(r'data_(\w+)::', data)))
        if actual != groups:
            raise RuntimeError(f'{name} linked {actual}, expected {groups}')
        dynamic = run('llvm-readelf', '-d', source)
        libraries = re.findall(r'Shared library: \[(.*?)\]', dynamic)
        if any('icu' in name or 'libstdc++' in name or 'libc++' in name for name in libraries):
            raise RuntimeError(f'unexpected runtime dependency: {libraries}')
        (args.output / (name + '.symbols')).write_text(data + '\n')
        (args.output / (name + '.sections')).write_text(sections)
        results[name] = {'stripped': stripped.stat().st_size, 'text': sizes.get('.text', 0),
                         'rodata': sizes.get('.rodata', 0), 'data_groups': actual,
                         'libraries': libraries}
    for name, values in results.items():
        values['increment'] = values['stripped'] - results['empty']['stripped']
        print(f"{name}: {values['increment']} additional bytes, groups={values['data_groups']}")
    (args.output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')


if __name__ == '__main__':
    main()
