"""Windows startup verification. Reads only the installed runtime's exported record."""
import ctypes as c
import hashlib
import json
from pathlib import Path
import subprocess

from tools.save_filename_support import Evidence
from .storage_session_candidate import STEAM_BUILD, StartupProof


def verify_evidence(value, *, created, binding, root):
    """Pure validator, shared by the platform reader and offline tests."""
    expected_root = Path(root).as_posix().rstrip('/') + '/'
    if ((value.magic, value.version, value.size, value.mode,
         value.install_status, value.fault, value.overflow, value.flight,
         value.startup_error) !=
            (0x4b484649, 1, c.sizeof(Evidence), 3, 1, 0, 0, 0, 0)):
        return False
    if (value.created != created or bytes(value.binding).hex() != binding
            or value.root != expected_root or not 0 < value.count <= 32):
        return False
    critical = value.critical_menu
    if (critical.installed != 1 or critical.calls <= 0 or critical.permitted <= 0
            or critical.last_after & 0x20 != 0x20):
        return False
    return all(event.complete == 1 and event.name in ('KH0.2BbS_WW', 'KH0.2BbS_JP')
               and event.path == expected_root + event.name + '.png'
               for event in value.events[:value.count])


def running_games():
    # Exact executable basename across storefronts; never search process memory.
    command = """$ErrorActionPreference='Stop'; @(Get-Process -Name 'KINGDOM HEARTS 0.2 Birth by Sleep' -ErrorAction SilentlyContinue | ForEach-Object { $p=$_; @{pid=$p.Id;created=$p.StartTime.ToFileTimeUtc();path=$p.Path;window=$p.MainWindowHandle.ToInt64();modules=@($p.Modules | Where-Object {$_.ModuleName -ieq 'dinput8.dll'} | ForEach-Object {@{path=$_.FileName;base=$_.BaseAddress.ToInt64()}})}}) | ConvertTo-Json -Depth 5 -Compress"""
    result = subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive',
                             '-Command', command], capture_output=True, text=True,
                            check=True, timeout=15, creationflags=0x08000000)
    if not result.stdout.strip():
        return ()
    rows = json.loads(result.stdout)
    return tuple(rows if isinstance(rows, list) else [rows])


def require_closed():
    if running_games():
        raise ValueError('Close KH 0.2 normally before preparing or restoring AP startup')


class StartupReader:
    """One DLL load per launch; two fixed-size reads per observation, no polling loop."""
    def __init__(self, executable, dll, *, expected_dll_sha256, packaged_dll):
        self.executable = Path(executable).resolve(strict=True)
        self.dll = Path(dll).resolve(strict=True)
        if self.dll != self.executable.parent / 'dinput8.dll':
            raise ValueError('Startup DLL must be beside the selected executable')
        if hashlib.sha256(self.executable.read_bytes()).hexdigest() != STEAM_BUILD:
            raise ValueError('Unsupported Steam executable')
        if hashlib.sha256(self.dll.read_bytes()).hexdigest() != expected_dll_sha256:
            raise ValueError('Installed startup DLL differs from prepared package')
        self.expected_dll_sha256 = expected_dll_sha256
        packaged_dll = Path(packaged_dll).resolve(strict=True)
        if packaged_dll == self.dll:
            raise ValueError('Load evidence definitions from the package, not the removable installation')
        if hashlib.sha256(packaged_dll.read_bytes()).hexdigest() != expected_dll_sha256:
            raise ValueError('Packaged runtime differs from installed runtime')
        self.local = c.CDLL(str(packaged_dll))
        self.offset = c.addressof(Evidence.in_dll(self.local, 'Kh02FilenameEvidence')) - self.local._handle
        self.kernel = c.WinDLL('kernel32', use_last_error=True)
        self.kernel.OpenProcess.argtypes = [c.c_uint32, c.c_int, c.c_uint32]
        self.kernel.OpenProcess.restype = c.c_void_p
        self.kernel.ReadProcessMemory.argtypes = [c.c_void_p, c.c_void_p, c.c_void_p,
                                                  c.c_size_t, c.POINTER(c.c_size_t)]
        self.kernel.CloseHandle.argtypes = [c.c_void_p]

    def observe(self, preparation, *, games=None):
        games = running_games() if games is None else tuple(games)
        if not games:
            return None
        if len(games) != 1 or Path(games[0]['path']).resolve() != self.executable:
            raise ValueError('Multiple games or a different storefront is running')
        game = games[0]
        modules = [m for m in game['modules'] if Path(m['path']).resolve() == self.dll]
        if not modules:
            return None  # Startup has not loaded the prepared runtime yet.
        if len(modules) != 1:
            raise ValueError('Ambiguous startup runtime')
        handle = self.kernel.OpenProcess(0x1010, False, game['pid'])
        if not handle:
            raise OSError(c.get_last_error(), 'Cannot read AP startup evidence')
        try:
            samples = []
            for _ in range(2):
                value, count = Evidence(), c.c_size_t()
                if not self.kernel.ReadProcessMemory(handle, modules[0]['base'] + self.offset,
                        c.byref(value), c.sizeof(value), c.byref(count)) or count.value != c.sizeof(value):
                    raise OSError(c.get_last_error(), 'Incomplete AP startup evidence')
                samples.append(value)
            if bytes(samples[0]) != bytes(samples[1]):
                return None
            value = samples[1]
            if value.fault or value.overflow or value.startup_error:
                raise ValueError('Runtime reported a startup failure; delivery remains disabled')
            if not verify_evidence(value, created=game['created'],
                                   binding=preparation.binding, root=preparation.root):
                return None
            # Hash the large executable at admission, not repeatedly while its
            # opening screens are still initializing the exported record.
            if (hashlib.sha256(self.dll.read_bytes()).hexdigest() != self.expected_dll_sha256
                    or hashlib.sha256(self.executable.read_bytes()).hexdigest() != STEAM_BUILD):
                raise ValueError('Prepared installation changed')
            return StartupProof(game['pid'], game['created'], str(self.executable),
                                STEAM_BUILD, preparation.binding, preparation.root, True)
        finally:
            self.kernel.CloseHandle(handle)

