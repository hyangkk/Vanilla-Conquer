#!/usr/bin/env python3
"""Prepare isolated Vanilla TD save-export inputs from extracted C&C 1.06c data.

Usage: prepare_expansion_starts.py EXTRACTED_APP OUTPUT_DIR INSTALLED_DATA
Run the engine with VANILLA_MISSION_START_DIR=OUTPUT_DIR and
VANILLA_MISSION_START_SLOT=200. Never writes to the installed game directory.
"""
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import sys


def mix_hash(name):
    data = name.upper().encode('ascii')
    data += b'\0' * (-len(data) % 4)
    result = 0
    for offset in range(0, len(data), 4):
        result = (((result << 1) | (result >> 31)) + int.from_bytes(data[offset:offset+4], 'little')) & 0xffffffff
    return result


def read_mix(path):
    data = path.read_bytes()
    count, size = struct.unpack_from('<HI', data)
    start = 6 + 12 * count
    if start + size != len(data):
        raise ValueError(f'Unsupported MIX format: {path}')
    result = {}
    for index in range(count):
        key, offset, length = struct.unpack_from('<III', data, 6 + 12 * index)
        if offset + length > size:
            raise ValueError(f'Invalid MIX member: {path}')
        result[key] = data[start+offset:start+offset+length]
    return result


def prepare(source, output, installed):
    if output.exists():
        raise ValueError('Output must be a new directory')
    missions = []
    for faction, count in [('G', 7), ('B', 8)]:
        for index in range(count):
            root = f'SC{faction}{21+index:02}EA'
            missions.append(('sc-001.mix', root, root, f'COV {"GDI" if faction == "G" else "NOD"} {index+1:02}'))
    for faction, first in [('G', 28), ('B', 29)]:
        for index in range(2):
            missions.append(('sc-n64.mix', f'SC{faction}{first+index:02}EA', f'SC{faction}{70+index:02}EA', f'N64 {"GDI" if faction == "G" else "NOD"} {index+1:02}'))
    for faction, first, count in [('G', 30, 3), ('B', 31, 2)]:
        for index in range(count):
            missions.append(('sc-psx.mix', f'SC{faction}{first+index:02}EA', f'SC{faction}{60+index:02}EA', f'PSX {"GDI" if faction == "G" else "NOD"} {index+1:02}'))
    missions.append(('sc-psx.mix', 'SCG33EA', 'SCG72EA', 'PSX SECRET'))
    archives = {name: read_mix(source/name) for name in {mission[0] for mission in missions}}
    files, manifest, labels = {}, [], []
    for archive, old, new, prefix in missions:
        ini = archives[archive][mix_hash(old+'.INI')]
        name = re.search(br'^Name=([^\r\n]+)', ini, re.M).group(1).decode('ascii').strip()
        label = f'{prefix} {name}'
        if len(label) > 40:
            label = label[:40]
        if archive == 'sc-n64.mix':
            # The classic engine provides WINTER, not the patch's fourth SNOW theater.
            ini = ini.replace(b'Theater=SNOW', b'Theater=WINTER')
            ini = re.sub(br'^BuildLevel=[^\r\n]+', b'BuildLevel=15', ini, flags=re.M)
        files[new+'.INI'] = ini
        files[new+'.BIN'] = archives[archive][mix_hash(old+'.BIN')]
        labels.append(label)
        manifest.append(dict(archive=archive, source=old, mission=new, description=label,
                             ini_sha256=hashlib.sha256(ini).hexdigest(),
                             bin_sha256=hashlib.sha256(files[new+'.BIN']).hexdigest()))
    # The distribution's combined GENERAL.MIX supplies the expansion disc search path.
    read_mix(source/'general.mix')
    read_mix(source/'scores.mix')
    output.mkdir(parents=True)
    for name, data in files.items():
        (output/name).write_bytes(data)
    (output/'covertops').mkdir()
    shutil.copy2(source/'general.mix', output/'covertops/GENERAL.MIX')
    shutil.copy2(source/'scores.mix', output/'covertops/SCORES.MIX')
    (output/'missions.txt').write_text('\n'.join(m['mission'] for m in manifest)+'\n')
    (output/'descriptions.txt').write_text('\n'.join(labels)+'\n')
    (output/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    (output/'CONQUER.INI').write_text(f'[Paths]\nDataPath={installed}\nUserPath={output.resolve()}\n[Video]\nGameWidth=960\nGameHeight=600\nWindowed=yes\n[Options]\nCampaignAllTech=yes\n[Intro]\nPlayIntro=no\n')
    print(f'Prepared {len(manifest)} missions in {output}')


if __name__ == '__main__':
    prepare(*(Path(value).resolve() for value in sys.argv[1:]))
