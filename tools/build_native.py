"""Compile native variants into build/runtime without touching the game."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--zig', type=Path, required=True)
    parser.add_argument('--variant', type=int, choices=range(1,9), action='append',
                        help='Build this selection only; repeat as needed. Default: all eight.')
    args = parser.parse_args()
    zig = args.zig.resolve()
    spec = json.loads((ROOT/'native/build-spec.json').read_text())
    common = [str(zig),'cc','-target','x86_64-windows-gnu','-shared','-O2','-Wall','-Wextra','-Werror']
    for index in args.variant or range(1,9):
        for kind,prefix,filename in [('startup','v187','dinput8.dll'),('transport','v277','Kh02FreshTransport.dll')]:
            variant = f'{prefix}{index:02}'
            stage = ROOT/'build/native-source'/variant
            output = ROOT/'build/runtime'/variant
            if stage.exists() or output.exists():
                raise ValueError(f'Existing build preserved: {variant}. Use a new checkout or move that build aside.')
            shutil.copytree(ROOT/'native'/kind/'common', stage)
            overlay = ROOT/'native'/kind/'variants'/variant
            if overlay.exists():
                shutil.copytree(overlay, stage, dirs_exist_ok=True)
            output.mkdir(parents=True)
            if kind == 'startup':
                defines = ['KH02_FILENAME_LIVE','KH02_FRESH_STORAGE_AUTH']
            else:
                defines = ['KH02_STRUCTURED_CLEAR','KH02_GAME_GRANT_CALLS=1','KH02_COSMETIC_GAME_CALLS=1',
                    'KH02_REUSABLE_SAVE_PIPELINE','KH02_FRESH_LIFECYCLE','KH02_GRANT_OWNER_LIVE',
                    'KH02_FRESH_TRANSPORT_RUNTIME','KH02_SAVE_TRANSPORT_DIAGNOSTIC','KH02_SAVE_HEADER_BINDING']
                if index <= 4:
                    defines.append('KH02_FRESH_SAVE_HARNESS')
            command = common + ['-D'+d for d in defines] + [str(stage/n) for n in spec[kind]['units']]
            command += ['-o',str(output/filename),'-lkernel32']
            if kind == 'transport':
                command += ['-lbcrypt','-luser32']
            print('Building '+variant, flush=True)
            subprocess.run(command, check=True)
    print('Native builds complete. Rebuilt DLLs require live validation before release.')

if __name__ == '__main__':
    main()
