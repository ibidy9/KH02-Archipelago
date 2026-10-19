"""Prepare a normal authenticated seed without research paths or fixture rewards."""
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import secrets
import struct

from .storage_session_candidate import StorageSession, STEAM_BUILD
from .steam_profiles_v187_candidate import open_profile, plain_directory
from .steam_preparation_v187_candidate import prepare, read_associations
from .steam_save_copy import copy_and_decode
from .steam_installation import Installation
from .multi_record_journal_candidate import initialize
from . import steam_runtime
from tools.save_filename_support import wire, resume_wire, filetime


@dataclass(frozen=True)
class Launch:
    session: object
    prepared: object
    installation: object
    runtime: object
    directory: Path
    profile: Path


def prepare_launch(context, *, executable, profiles_root, package_root, steam_id,
                   player_name, running_games, require_closed, resume_target=1):
    require_closed()
    executable = Path(executable).absolute()
    plain_directory(executable.parent)
    if executable.is_symlink() or hashlib.sha256(executable.read_bytes()).hexdigest() != STEAM_BUILD:
        raise ValueError('Select the supported Steam KH 0.2 executable')
    xp, snapshot = context.xp_admission.current(context)
    mickey, paired = context.mickey_admission.current(context)
    if xp.mickey != mickey or paired != snapshot:
        raise ValueError("XP and Mickey admissions differ")
    config = mickey.goal
    slot_data = dict(schema_version=4, item_schema_version=5,
        xp_multiplier_halves=xp.halves,
        game='Kingdom Hearts 0.2 Birth by Sleep', goal=config.selected_goal.value,
        goal_requirement=config.requirement.value, starting_world=config.starting_world,
        randomized_world_access=True, minimal_clear_content=True, critical_available=True,
        mickey=mickey.mode)
    profile, metadata = open_profile(profiles_root, snapshot.key, slot_data, player_name=player_name)
    # Incomplete prior startup transactions must be recovered explicitly while
    # closed; never overwrite them or blindly reuse their receipt session.
    targets = [executable.parent / n for n in
               ('dinput8.dll', 'kh02-ap-ready.bin', 'kh02-ap-resume.bin')]
    if any(p.exists() or p.is_symlink() for p in targets):
        raise ValueError('Existing startup files preserved; recover the previous installation first')
    directory = profile / 'sessions' / secrets.token_hex(16)
    directory.mkdir()
    decoded = copy_and_decode(profile, directory, steam_id=steam_id,
        require_closed=require_closed, decoder_script=Path(package_root)/'tools/decode_kh02_save_copy.ps1')
    mode = 'fresh' if decoded is None else 'resume'
    runtime = steam_runtime.select(package_root, mode=mode, starting_world=config.starting_world)
    journal = profile / 'receipts' / (directory.name + '.sqlite')
    old_journals = tuple((profile / 'receipts').glob('*.sqlite'))
    if any(p.is_symlink() for p in old_journals):
        raise ValueError('Receipt journals cannot be links')
    associations = read_associations(old_journals) if decoded is not None else ()
    mods = executable.parent.parent.parent / 'Content/Paks/~mods'
    plain_directory(mods)
    mods.mkdir(parents=True, exist_ok=True)
    prototype = mods / 'KH02_AP_RestartPrototype_P.pak'
    menu = mods / 'TresGame-WindowsNoEditor_99_P.pak'
    installation = Installation(directory / 'installation', require_closed=require_closed,
                                allowed_targets=(*targets, prototype, menu))
    additions = []
    for target, name in ((prototype, 'prototype'), (menu, 'menu')):
        data = runtime.read(name)
        if target.is_symlink():
            raise ValueError('Linked mod preserved; cannot prepare')
        if target.exists():
            if target.read_bytes() != data:
                raise ValueError('Conflicting mod preserved: ' + str(target))
        else:
            additions.append((target, data))
    session = StorageSession(context)
    prepared = None
    def publish(storage, current):
        nonlocal prepared
        initialize(journal)
        prepared = prepare(session, storage, journal_path=journal,
            runtime_start=config.starting_world, raw=None if decoded is None else decoded[0],
            associations=associations, target=resume_target)
        ready = bytearray(wire(executable, profile/'saves', profile/'claims',
            bytes.fromhex(storage.binding), prepared.nonce, prepared=storage.prepared))
        struct.pack_into('<Q', ready, 24, storage.expires)
        additions.extend(((targets[0], runtime.read('startup')), (targets[1], bytes(ready))))
        if decoded is not None:
            additions.append((targets[2], resume_wire(ready, decoded[1])))
        for name, data in (('prepared-setup.bin', prepared.payload), ('prepared-ready.bin', ready)):
            with (directory/name).open('xb') as stream:
                stream.write(data)
        with (directory/'launch.json').open('x', encoding='utf-8') as stream:
            json.dump(dict(mode=mode, profile=str(profile), executable=str(executable),
                generation=storage.generation, profile_generation=metadata['generation'],
                binding=storage.binding, runtime_start=config.starting_world,
                resume_target=resume_target, runtime_hashes=runtime.hashes), stream, indent=2)
        if not context.snapshot_is_current(current):
            raise ValueError('AP history changed during launch preparation')
        containers = tuple((profile/'saves').iterdir())
        if decoded is None:
            if containers:
                raise ValueError('Fresh save folder changed during preparation')
        else:
            selected = [p for p in containers if p.name in ('KH0.2BbS_WW.png', 'KH0.2BbS_JP.png')]
            if (len(selected) != 1 or selected[0].is_symlink()
                    or hashlib.sha256(selected[0].read_bytes()).hexdigest() != decoded[1]):
                raise ValueError('AP save changed after receipt preparation')
        installation.install(additions)
    now = filetime()
    # Per-launch publication identity; profile ownership is the immutable AP scope.
    # Associations are resolved by exact record + scope, not by this launch nonce.
    try:
        session.prepare(root=str(profile/'saves'), executable=str(executable),
            executable_sha256=STEAM_BUILD, prepared=now, expires=now+3600*10000000,
            generation=secrets.token_hex(16), running_games=running_games, publish=publish)
    except BaseException as error:
        if installation.manifest.exists():
            try:
                installation.restore()
            except Exception as cleanup_error:
                raise RuntimeError('Preparation failed; preserve startup files and close the game normally. '
                    f'Recovery: {installation.manifest}. Reason: {cleanup_error}') from error
        raise
    return Launch(session, prepared, installation, runtime, directory, profile)
