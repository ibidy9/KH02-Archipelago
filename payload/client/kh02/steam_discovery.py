"""Read Steam's own configuration only; never inspect or adopt game saves."""
from pathlib import Path
import re

GAME = Path('KINGDOM HEARTS HD 2.8 Final Chapter Prologue/KINGDOM HEARTS 0.2 Birth by Sleep/Binaries/Win64/KINGDOM HEARTS 0.2 Birth by Sleep.exe')


def vdf(text):
    tokens = iter(re.findall(r'"((?:\\.|[^"\\])*)"|([{}])', text))
    def block():
        result = {}
        for quoted, symbol in tokens:
            if symbol == '}': return result
            if symbol: raise ValueError('Invalid Steam configuration')
            value, marker = next(tokens)
            result[quoted] = block() if marker == '{' else value.replace('\\\\', '\\').replace('\\"', '"')
        return result
    return block()


def discover_at(root, active_user=0):
    root = Path(root)
    libraries = [root]
    config = root/'steamapps/libraryfolders.vdf'
    if config.exists():
        libraries += [Path(row['path']) for row in vdf(config.read_text(encoding='utf-8'))['libraryfolders'].values()
                      if isinstance(row, dict) and 'path' in row]
    games = list(dict.fromkeys(str(p.resolve()) for lib in libraries
                               if (p := lib/'steamapps/common'/GAME).is_file()))
    accounts = []
    config = root/'config/loginusers.vdf'
    if config.exists():
        for identity, row in vdf(config.read_text(encoding='utf-8'))['users'].items():
            if len(identity) == 17 and identity.isascii() and identity.isdigit():
                accounts.append((identity, row.get('PersonaName', 'Steam account')))
    active = str(76561197960265728 + active_user) if active_user else None
    selected = next((identity for identity, _ in accounts if identity == active), None)
    return games, accounts, selected


def discover():
    import winreg
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam') as key:
        root = winreg.QueryValueEx(key, 'SteamPath')[0]
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam\ActiveProcess') as key:
            active = winreg.QueryValueEx(key, 'ActiveUser')[0]
    except OSError:
        active = 0
    return discover_at(root, active)


def choose_setup(*, ask=input, notify=print):
    games, accounts, active = discover()
    def choose(rows, label):
        if not rows: raise ValueError(f'{label} was not found. Open Steam and sign in, then try again.')
        if len(rows) == 1: return rows[0][0]
        for number, (_, name) in enumerate(rows, 1): notify(f'{number}. {name}')
        while True:
            answer = ask(f'Choose {label} (1–{len(rows)}): ').strip()
            if answer.isdigit() and 1 <= int(answer) <= len(rows): return rows[int(answer)-1][0]
            notify('Enter one of the numbers shown above.')
    game = choose([(p, p) for p in games], 'Steam installation')
    identity = active or choose(accounts, 'Steam account')
    notify('Steam game and account detected.')
    return game, identity


def automatic_setup():
    """Release setup: require a live Steam process and its active account."""
    import ctypes as c
    from ctypes import wintypes as w
    import winreg
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam') as key:
        root = Path(winreg.QueryValueEx(key, 'SteamPath')[0])
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam\ActiveProcess') as key:
        pid = winreg.QueryValueEx(key, 'pid')[0]
        active = winreg.QueryValueEx(key, 'ActiveUser')[0]
    if not pid or not active:
        raise ValueError('Open Steam and sign into the account you will play with, then reconnect.')
    kernel = c.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
    kernel.OpenProcess.restype = w.HANDLE
    kernel.QueryFullProcessImageNameW.argtypes = [w.HANDLE, w.DWORD, w.LPWSTR, c.POINTER(w.DWORD)]
    kernel.CloseHandle.argtypes = [w.HANDLE]
    handle = kernel.OpenProcess(0x1000, False, pid)
    if not handle: raise ValueError('Steam is not running. Open Steam and reconnect.')
    try:
        path, size = c.create_unicode_buffer(32768), w.DWORD(32768)
        if not kernel.QueryFullProcessImageNameW(handle, 0, path, c.byref(size)):
            raise ValueError('Cannot verify the active Steam process. Restart Steam and reconnect.')
        if Path(path.value).resolve() != (root/'steam.exe').resolve():
            raise ValueError('Steam process does not match its installation. Restart Steam and reconnect.')
    finally:
        kernel.CloseHandle(handle)
    games, accounts, identity = discover_at(root, active)
    if not identity:
        raise ValueError('Steam has no verified signed-in account. Sign into Steam and reconnect.')
    if len(games) != 1:
        raise ValueError('Could not identify one installed Steam 2.8 collection. Check its Steam installation.')
    return games[0], identity
