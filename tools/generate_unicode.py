#!/usr/bin/env python3
"""Generate shared compressed Unicode 18.0.0 tables for liunicode."""
import argparse
from array import array
import hashlib
from pathlib import Path
from urllib.request import urlopen
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[1]
VERSION = "18.0.0"
SOURCES = {
    'DerivedCoreProperties.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/DerivedCoreProperties.txt',
        '09c928886a178fcafd93c29e4bd59073a058e5a100b716d425cb563ab50f68c9'),
    'CaseFolding.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/CaseFolding.txt',
        'a004797658a457bec4dc11683e39f69249ea3b595b752dbea6721c4c9f587b0d'),
    'UnicodeData.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/UnicodeData.txt',
        '0736451de439ae7baf1425136617da495e09ee5afbe6e394374db7009ea08950'),
    'SpecialCasing.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/SpecialCasing.txt',
        '8538dea57c184f1ef3783885ea79677b10f6efa06423717157e63712f14d1ad2'),
    'DerivedNormalizationProps.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/DerivedNormalizationProps.txt',
        '98ac7f67d985fe781e317f6182e885e94cabb0c314769e6dd73e48b226931ccd'),
    'PropertyAliases.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/PropertyAliases.txt',
        '83b8df695f9da543dba02b0be2b8bd72f0b52836ad264be113c6d285021ef025'),
    'PropertyValueAliases.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/PropertyValueAliases.txt',
        '06c4c8eaf7b0bf34abe73b113da1215bd784ac254d4c223600b90267caa4bbbd'),
    'Scripts.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/Scripts.txt',
        '0071fd81b6aeae25f6e8bce8efec3066a6476a91b49bdb2f52dc76e817862a6a'),
    'ScriptExtensions.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/ScriptExtensions.txt',
        '5c9d34a922f687726f2a8bcf57d49f905987e51f1b21b58c95a00fbe255cec23'),
    'PropList.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/PropList.txt',
        'f438f532e8737bb8a2702126cdf9c4af5e357c58c7acf9d9eb2fc7c1a1d955d6'),
    'emoji-data.txt': (
        'https://www.unicode.org/Public/18.0.0/ucd/emoji/emoji-data.txt',
        '80d00f8e616a0ef27fd6b8de3b758c06383b5d917e2977709578e68baf733bf1'),
    'emoji-sequences.txt': (
        'https://www.unicode.org/Public/18.0.0/emoji/emoji-sequences.txt',
        '1823dce71f3dd9cb0ad1976797baee4ffbdc1909756f43af8ce4c57f732843a4'),
    'emoji-zwj-sequences.txt': (
        'https://www.unicode.org/Public/18.0.0/emoji/emoji-zwj-sequences.txt',
        'f61b5213bdf85a57741a9f903cb38d0f39a3d13be6ff4aca3b39857e859a9191'),
}

SOURCES.update({'GraphemeBreakProperty.txt': ('https://www.unicode.org/Public/18.0.0/ucd/auxiliary/GraphemeBreakProperty.txt', '0839dcb79e4ac639ecd538b1abf7c9d22e3f9dd265b7e182d33627aa4d75b45a'), 'WordBreakProperty.txt': ('https://www.unicode.org/Public/18.0.0/ucd/auxiliary/WordBreakProperty.txt', '8dbfa17063e11084201f33c3e76d485d3b9166930c71db8e39ed1c9234171aec'), 'GraphemeBreakTest.txt': ('https://www.unicode.org/Public/18.0.0/ucd/auxiliary/GraphemeBreakTest.txt', 'b0cf047ee94485bbdc846de2b902f5f8a815f6b674f9d04223cddadd91c9df31'), 'WordBreakTest.txt': ('https://www.unicode.org/Public/18.0.0/ucd/auxiliary/WordBreakTest.txt', '3dd70c071781276067c680d87303f60434adce7b067bd063194af374edad86a5'), 'NormalizationTest.txt': ('https://www.unicode.org/Public/18.0.0/ucd/NormalizationTest.txt', '25a50d816764b04abfb4a646d3eb2b2a803284c3873d9a06757b94fe4513dde3')})

def load_sources(directory):
    data = {}
    directory.mkdir(parents=True, exist_ok=True)
    for name, (url, digest) in SOURCES.items():
        path = directory / name
        raw = path.read_bytes() if path.exists() else urlopen(url, timeout=45).read()
        if hashlib.sha256(raw).hexdigest() != digest:
            raise RuntimeError(f"{name} checksum mismatch")
        path.write_bytes(raw)
        data[name] = [tuple(field.strip() for field in line.split('#')[0].split(';'))
                      for line in raw.decode('utf-8').splitlines()
                      if line.split('#')[0].strip()]
    return data


def language_tables(data):
    properties = {'ID_Start': [], 'ID_Continue': [], 'Cased': [], 'Case_Ignorable': []}
    for fields in data['DerivedCoreProperties.txt']:
        if len(fields) != 2 or fields[1].strip() not in properties:
            continue
        bounds = fields[0].strip().split('..')
        first, last = int(bounds[0], 16), int(bounds[-1], 16)
        ranges = properties[fields[1].strip()]
        if ranges and ranges[-1][1] + 1 == first:
            ranges[-1] = ranges[-1][0], last
        else:
            ranges.append((first, last))
    fold = {}
    for fields in data['CaseFolding.txt']:
        if len(fields) >= 3 and fields[1] in ('C', 'S'):
            source, target = int(fields[0], 16), int(fields[2], 16)
            if source != target:
                fold[source] = target
    lower_full = {}
    upper_full = {}
    combining_classes = {}
    decompositions = {}
    for fields in data['UnicodeData.txt']:
        source = int(fields[0], 16)
        combining = int(fields[3])
        if combining:
            combining_classes[source] = combining
        if fields[5]:
            parts = fields[5].split()
            compatibility = parts[0].startswith('<')
            targets = tuple(int(value, 16) for value in parts[compatibility:])
            decompositions[source] = (targets, compatibility)
        if fields[12]:
            source, target = int(fields[0], 16), int(fields[12], 16)
            if source != target:
                upper_full[source] = (target,)
        if fields[13]:
            source, target = int(fields[0], 16), int(fields[13], 16)
            if source != target:
                lower_full[source] = (target,)
    for fields in data['SpecialCasing.txt']:
        if len(fields) < 5 or fields[4]:
            continue
        source = int(fields[0], 16)
        for mapping, field in ((lower_full, fields[1]), (upper_full, fields[3])):
            targets = tuple(int(value, 16) for value in field.split())
            if len(targets) > 3:
                raise RuntimeError(f'case mapping for {source:04X} exceeds three code points')
            if targets != (source,):
                mapping[source] = targets
            else:
                mapping.pop(source, None)
    composition_exclusions = set()
    for fields in data['DerivedNormalizationProps.txt']:
        if len(fields) < 2 or fields[1] != 'Full_Composition_Exclusion':
            continue
        bounds = fields[0].split('..')
        composition_exclusions.update(range(int(bounds[0], 16), int(bounds[-1], 16) + 1))
    compositions = {}
    for source, (targets, compatibility) in decompositions.items():
        if not compatibility and len(targets) == 2 and source not in composition_exclusions:
            if targets in compositions:
                raise RuntimeError(f'duplicate composition pair {targets}')
            compositions[targets] = source

    return properties, fold, lower_full, upper_full, combining_classes, decompositions, compositions


def property_tables(data):
    BINARY = set('''ASCII ASCII_Hex_Digit Alphabetic Any Assigned Bidi_Control Bidi_Mirrored
    Case_Ignorable Cased Changes_When_Casefolded Changes_When_Casemapped
    Changes_When_Lowercased Changes_When_NFKC_Casefolded Changes_When_Titlecased
    Changes_When_Uppercased Dash Default_Ignorable_Code_Point Deprecated Diacritic
    Emoji Emoji_Component Emoji_Modifier Emoji_Modifier_Base Emoji_Presentation
    Extended_Pictographic Extender Grapheme_Base Grapheme_Extend Hex_Digit
    IDS_Binary_Operator IDS_Trinary_Operator ID_Continue ID_Start Ideographic
    Join_Control Logical_Order_Exception Lowercase Math Noncharacter_Code_Point
    Pattern_Syntax Pattern_White_Space Quotation_Mark Radical Regional_Indicator
    Sentence_Terminal Soft_Dotted Terminal_Punctuation Unified_Ideograph Uppercase
    Variation_Selector White_Space XID_Continue XID_Start'''.split())
    SIZE = 0x110000

    def rows(name):
        yield from data[name]

    def bounds(field):
        parts = field.split('..')
        return int(parts[0], 16), int(parts[-1], 16)

    def merge(ranges):
        output = []
        for first, last in sorted(ranges):
            if output and first <= output[-1][1] + 1:
                output[-1] = output[-1][0], max(last, output[-1][1])
            else:
                output.append((first, last))
        return output

    def append_range(ranges, first, last):
        if ranges and ranges[-1][1] + 1 == first:
            ranges[-1] = ranges[-1][0], last
        else:
            ranges.append((first, last))


    value_aliases = {'gc': {}, 'sc': {}}
    for fields in rows('PropertyValueAliases.txt'):
        if fields[0] in value_aliases:
            for alias in fields[1:]:
                value_aliases[fields[0]][alias] = fields[1]

    categories = sorted({canonical for canonical in value_aliases['gc'].values() if len(canonical) == 2
                         and canonical != 'LC'})
    category_ids = {name: index for index, name in enumerate(categories)}
    category_values = bytearray([category_ids['Cn']]) * SIZE
    bidi_mirrored = []
    range_start = None
    for fields in data['UnicodeData.txt']:
        cp = int(fields[0], 16)
        if fields[1].endswith(', First>'):
            range_start = cp
            continue
        first = range_start if range_start is not None else cp
        range_start = None
        category_values[first:cp + 1] = bytes([category_ids[fields[2]]]) * (cp - first + 1)
        if fields[9] == 'Y':
            bidi_mirrored.append((first, cp))

    script_names = sorted(set(value_aliases['sc'].values()))
    script_ids = {name: index for index, name in enumerate(script_names)}
    script_values = array('H', [script_ids['Zzzz']]) * SIZE
    for field, name, *_ in rows('Scripts.txt'):
        first, last = bounds(field)
        canonical = value_aliases['sc'][name]
        script_values[first:last + 1] = array('H', [script_ids[canonical]]) * (last - first + 1)

    extensions = {}
    for field, names, *_ in rows('ScriptExtensions.txt'):
        first, last = bounds(field)
        ids = tuple(script_ids[value_aliases['sc'][name]] for name in names.split())
        for cp in range(first, last + 1):
            extensions[cp] = ids

    binary_ranges = {name: [] for name in BINARY}
    for name in ('PropList.txt', 'DerivedCoreProperties.txt', 'DerivedNormalizationProps.txt', 'emoji-data.txt'):
        for field, prop, *_ in rows(name):
            if prop in binary_ranges:
                binary_ranges[prop].append(bounds(field))
    binary_ranges['ASCII'] = [(0, 0x7f)]
    binary_ranges['Any'] = [(0, SIZE - 1)]
    binary_ranges['Bidi_Mirrored'] = bidi_mirrored

    category_ranges = {name: [] for name in set(value_aliases['gc'].values())}
    script_ranges = {name: [] for name in script_names}
    extension_ranges = {name: [] for name in script_names}
    assigned = []
    for cp in range(SIZE):
        category = categories[category_values[cp]]
        for name, ranges in category_ranges.items():
            if category == name or (len(name) == 1 and category.startswith(name)) or (
                    name == 'LC' and category in ('Ll', 'Lt', 'Lu')):
                append_range(ranges, cp, cp)
        if category != 'Cn':
            append_range(assigned, cp, cp)
        script = script_values[cp]
        append_range(script_ranges[script_names[script]], cp, cp)
        for item in extensions.get(cp, (script,)):
            append_range(extension_ranges[script_names[item]], cp, cp)
    binary_ranges['Assigned'] = assigned
    for name, ranges in binary_ranges.items():
        if not ranges:
            raise RuntimeError(f'{name} has no Unicode {VERSION} ranges')

    property_aliases = {}
    property_sets = []

    def register(ranges):
        index = len(property_sets)
        property_sets.append(merge(ranges))
        return index


    binary_ids = {name: register(binary_ranges[name]) for name in sorted(BINARY)}
    for name, index in binary_ids.items():
        property_aliases[name] = index
    for fields in rows('PropertyAliases.txt'):
        if fields[1] in binary_ids:
            for alias in fields:
                property_aliases[alias] = binary_ids[fields[1]]
    category_ids = {name: register(category_ranges[name]) for name in sorted(category_ranges)}
    script_ids = {name: register(script_ranges[name]) for name in script_names}
    extension_ids = {name: register(extension_ranges[name]) for name in script_names}

    def aliases(values, indices):
        return sorted((alias, indices[canonical]) for alias, canonical in values.items())


    alias_tables = {
        'BinaryAliases': sorted(property_aliases.items()),
        'CategoryAliases': aliases(value_aliases['gc'], category_ids),
        'ScriptAliases': aliases(value_aliases['sc'], script_ids),
        'ExtensionAliases': aliases(value_aliases['sc'], extension_ids),
    }
    string_names = ('Basic_Emoji', 'Emoji_Keycap_Sequence', 'RGI_Emoji_Modifier_Sequence',
                    'RGI_Emoji_Flag_Sequence', 'RGI_Emoji_Tag_Sequence',
                    'RGI_Emoji_ZWJ_Sequence', 'RGI_Emoji')
    string_sets = {name: set() for name in string_names[:-1]}
    for filename in ('emoji-sequences.txt', 'emoji-zwj-sequences.txt'):
        for field, name, *_ in rows(filename):
            if name not in string_sets:
                continue
            if '..' in field:
                first, last = bounds(field)
                string_sets[name].update((cp,) for cp in range(first, last + 1))
            else:
                string_sets[name].add(tuple(int(cp, 16) for cp in field.split()))
    string_sets['RGI_Emoji'] = set().union(*(string_sets[name] for name in string_names[:-1]))

    return property_sets, alias_tables, string_names, string_sets


class Encoder:
    def __init__(self):
        self.data = bytearray()
        self.index = []
        self.tables = []
        self.shared = {}

    def integer(self, value):
        assert 0 <= value < 1 << 64
        while value >= 128:
            self.data.append((value & 127) | 128)
            value >>= 7
        self.data.append(value)

    def table(self, rows):
        rows = tuple(sorted((key, tuple(values)) for key, values in rows))
        if rows in self.shared:
            return self.shared[rows]
        index_start = len(self.index)
        widths = {len(values) for _, values in rows}
        width = next(iter(widths)) if len(widths) == 1 else 0
        previous = -1
        for ordinal, (key, values) in enumerate(rows):
            assert key > previous and 0 < len(values) <= 32
            if ordinal % 32 == 0:
                self.index.append((key, len(self.data)))
                self.integer(key)
            else:
                self.integer(key - previous)
            if not width:
                self.data.append(len(values))
            for value in values:
                self.integer(value)
            previous = key
        table = len(self.tables)
        self.tables.append((len(self.data), index_start, len(self.index) - index_start, width))
        self.shared[rows] = table
        return table

    def ranges(self, ranges):
        return self.table((first, (last - first,)) for first, last in ranges)


def sequence_tree(string_names, string_sets, fold):
    nodes = []
    shared = {}

    def intern(tree):
        children = tuple((cp, intern(child)) for cp, child in sorted(tree.items()) if cp >= 0)
        key = tree.get(-1, 0), children
        if key not in shared:
            shared[key] = len(nodes)
            nodes.append(key)
        return shared[key]

    roots = []
    for folded in (False, True):
        tree = {}
        for index, name in enumerate(string_names):
            for sequence in sorted(string_sets[name]):
                node = tree
                for cp in sequence:
                    node = node.setdefault(fold.get(cp, cp) if folded else cp, {})
                node[-1] = node.get(-1, 0) | (1 << index)
        roots.append(intern(tree))
    edges = [((parent << 21) | cp, (child,))
             for parent, (_, children) in enumerate(nodes) for cp, child in children]
    return roots, [mask for mask, _ in nodes], edges


def emit_rows(lines, rows, width=4):
    for start in range(0, len(rows), width):
        lines.append('    ' + ', '.join(rows[start:start + width]) + ',')


def emit_group(directory, name, encoder, constants, declarations=(), definitions=()):
    header = [f'// Generated by tools/generate_unicode.py; Unicode {VERSION}, see LICENSE.unicode.',
              '// clang-format off']
    namespace = [f'namespace liunicode::data_{name} {{', 'using namespace tables;']
    footer = ['}', '// clang-format on']
    interface = header + [f'module liunicode:data.{name};', 'import :tables;'] + namespace
    interface += ['extern const Storage storage;']
    interface += [f'inline constexpr unsigned {key} = {value};' for key, value in constants.items()]
    interface += list(declarations) + footer
    (directory / (name + '.cppm')).write_text('\n'.join(interface) + '\n')

    lines = header + ['module liunicode;', f'import :data.{name};', 'import :tables;'] + namespace
    lines += ['const char Data[] =']
    for start in range(0, len(encoder.data), 32):
        lines.append('    "' + ''.join(f'\\x{value:02x}' for value in encoder.data[start:start + 32]) + '"')
    lines += [';', 'const Checkpoint Checkpoints[] = {']
    emit_rows(lines, [f'{{{key}ull, {offset}}}' for key, offset in encoder.index])
    lines += ['};', 'const Table Tables[] = {']
    emit_rows(lines, [f'{{{end}, {index}, {count}, {width}}}' for end, index, count, width in encoder.tables])
    lines += ['};', 'constinit const Storage storage{Data, Checkpoints, Tables};']
    lines += list(definitions) + footer
    (directory / (name + '.cpp')).write_text('\n'.join(lines) + '\n')
    print(f'{name}: {len(encoder.data)} packed bytes, {len(encoder.index)} checkpoints, {len(encoder.tables)} tables')


def property_rows(data, file, prop):
    for fields in data[file]:
        if len(fields) >= 2 and fields[1] == prop:
            parts = fields[0].split('..')
            yield int(parts[0], 16), int(parts[-1], 16)


def segmentation(data, output):
    encoder = Encoder()
    constants = {}
    extra = []
    for name, file in [('Grapheme', 'GraphemeBreakProperty.txt'), ('Word', 'WordBreakProperty.txt')]:
        names = ['Other'] + sorted({f[1] for f in data[file]})
        extra.append('enum class ' + name + ' : unsigned { ' + ', '.join(names) + ' };')
        rows = []
        for fields in data[file]:
            parts = fields[0].split('..'); first, last = int(parts[0],16), int(parts[-1],16)
            rows.append((first, (last-first, names.index(fields[1]))))
        constants[name+'Table'] = encoder.table(rows)
    names = ['None', 'Consonant', 'Extend', 'Linker']
    extra.append('enum class Indic : unsigned { ' + ', '.join(names) + ' };')
    rows = []
    for f in data['DerivedCoreProperties.txt']:
        if len(f) >= 3 and f[1] == 'InCB':
            parts=f[0].split('..');first,last=int(parts[0],16),int(parts[-1],16)
            rows.append((first,(last-first,names.index(f[2]))))
    constants['IndicTable'] = encoder.table(rows)
    for name,file,prop in [('Pictographic','emoji-data.txt','Extended_Pictographic'),
                           ('Alphabetic','DerivedCoreProperties.txt','Alphabetic'),
                           ('Ideographic','PropList.txt','Ideographic'),
                           ('Whitespace','PropList.txt','White_Space')]:
        constants[name+'Table'] = encoder.ranges(property_rows(data,file,prop))
    emit_group(output,'segmentation',encoder,constants,extra)


def generate(data, output):
    output.mkdir(parents=True, exist_ok=True)
    properties, fold, lower, upper_full, combining, decompositions, compositions = language_tables(data)
    encoder = Encoder(); constants = {}
    for name,ranges in properties.items(): constants[name+'Table'] = encoder.ranges(ranges)
    for name,values in [('FoldTable',fold)]:
        constants[name] = encoder.table((key,(value,)) for key,value in values.items())
    for name,values in [('LowercaseTable',lower),('UppercaseTable',upper_full)]: constants[name] = encoder.table(values.items())
    inverse = {}
    for cp,target in sorted(fold.items()): inverse.setdefault(target,[]).append(cp)
    constants['FoldSourcesTable'] = encoder.table(inverse.items())
    emit_group(output,'casing',encoder,constants)
    encoder = Encoder(); constants = {}
    constants['CombiningTable'] = encoder.table((key,(value,)) for key,value in combining.items())
    constants['DecompositionTable'] = encoder.table((cp,(int(compat),*points)) for cp,(points,compat) in decompositions.items())
    constants['CompositionTable'] = encoder.table(((a<<21)|b,(cp,)) for (a,b),cp in compositions.items())
    emit_group(output,'normalization',encoder,constants)
    sets, aliases, string_names, string_sets = property_tables(data)
    encoder = Encoder()
    ids = [encoder.ranges(rows) for rows in sets]
    roots,masks,edges = sequence_tree(string_names,string_sets,fold)
    constants = {'StringEdgesTable':encoder.table(edges)}
    aliases = {name:[(alias,ids[index]) for alias,index in rows] for name,rows in aliases.items()}
    aliases['StringAliases'] = sorted((name,index) for index,name in enumerate(string_names))
    names = sorted({name for rows in aliases.values() for name,_ in rows})
    offsets={};size=0
    for name in names: offsets[name]=size;size+=len(name)+1
    declarations = [f'extern const char Names[{size + 1}];']
    definitions = ['const char Names[] ='] + [f'    "{name}\\0"' for name in names] + [';']
    for table,rows in aliases.items():
        declarations.append(f'extern const Alias {table}[{len(rows)}];')
        definitions.append(f'const Alias {table}[] = {{')
        emit_rows(definitions,[f'{{{offsets[name]}, {index}}}' for name,index in rows]);definitions.append('};')
    declarations += [f'extern const unsigned char StringMasks[{len(masks)}];',
                     f'extern const unsigned StringRoots[{len(roots)}];',
                     f'extern const unsigned StringLengths[{len(string_names)}];']
    definitions.append('const unsigned char StringMasks[] = {')
    emit_rows(definitions,[str(mask) for mask in masks],24);definitions.append('};')
    definitions.append('const unsigned StringRoots[] = {'+', '.join(map(str,roots))+'};')
    definitions.append('const unsigned StringLengths[] = {'+', '.join(str(max(map(len,string_sets[name]))) for name in string_names)+'};')
    emit_group(output,'properties',encoder,constants,declarations,definitions)
    segmentation(data,output)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-dir',type=Path,default=ROOT/'build/unicode'/VERSION)
    parser.add_argument('--output',type=Path,default=ROOT/'src/data')
    parser.add_argument('--check', action='store_true', help='Check generated files without modifying them')
    args=parser.parse_args()
    data=load_sources(args.data_dir)
    if args.check:
        with TemporaryDirectory() as directory:
            target=Path(directory)
            generate(data,target)
            expected = {path.name for path in target.iterdir()}
            saved_files = {path.name for suffix in ('*.cppm', '*.cpp') for path in args.output.glob(suffix)}
            if expected != saved_files:
                raise RuntimeError(f'{args.output} generated file set differs: {sorted(expected ^ saved_files)}')
            for generated in sorted(target.iterdir()):
                saved=args.output/generated.name
                if not saved.is_file() or saved.read_bytes()!=generated.read_bytes():
                    raise RuntimeError(f'{saved} differs from generated Unicode data')
        print('generated Unicode data is reproducible')
    else:
        generate(data,args.output)

if __name__ == '__main__': main()
