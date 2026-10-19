"""Normal Steam client entry point. Requires the verified packaged runtime catalog."""
import argparse
import asyncio
import os
import sys
from pathlib import Path


def prompt_settings(directory, *, ask=input, notify=print, detect=None):
    """Remember connection choices only; never store passwords or touch saves."""
    import json
    directory = Path(directory)
    path = directory/'connection.json'
    try:
        previous = json.loads(path.read_text(encoding='utf-8')) if path.exists() else {}
    except (ValueError, OSError):
        previous = {}
    notify('KH 0.2 Archipelago — Steam beta candidate')
    notify('Keep the game closed until the client says AP is prepared.')
    settings = {}
    for key, label in (('server', 'AP server:port'), ('slot', 'AP player name')):
        default = previous.get(key, '')
        while True:
            value = ask(label + (f' [{default}]' if default else '') + ': ').strip()
            settings[key] = (value or default).strip('"')
            if settings[key]: break
            notify(label+' is required. Please try again.')
    if detect is None:
        from .steam_discovery import choose_setup
        detect = lambda: choose_setup(ask=ask, notify=notify)
    settings['exe'], settings['steam_id'] = detect()
    if len(settings['steam_id']) != 17 or not settings['steam_id'].isascii() or not settings['steam_id'].isdigit():
        raise ValueError('Steam account ID must contain 17 digits')
    directory.mkdir(parents=True, exist_ok=True)
    import uuid
    temporary = directory/('connection.'+uuid.uuid4().hex+'.json')
    with temporary.open('x', encoding='utf-8') as stream:
        json.dump(settings, stream, ensure_ascii=False, indent=2)
    os.replace(temporary, path)
    return [argument for key, value in settings.items() for argument in ('--'+key.replace('_', '-'), value)]


async def connection_check(args):
    """Read-only packaged-client diagnostic: authenticate, reconnect, never attach."""
    import json
    from CommonClient import server_loop
    from .steam_context_v187_candidate import SteamContext
    context = SteamContext(args.server, args.password)
    context.auth = args.slot
    async def connect():
        context.disconnected_intentionally = False
        context.server_task = asyncio.create_task(server_loop(context, args.server))
        for _ in range(200):
            if context.history_fault:
                raise ValueError(context.history_fault)
            try:
                _, snapshot = context.xp_admission.current(context)
                return snapshot
            except (ValueError, RuntimeError):
                await asyncio.sleep(.05)
        raise TimeoutError('AP connection/history did not become ready')
    try:
        first = await connect()
        await context.disconnect()
        if context.server_task is not None:
            await asyncio.wait_for(context.server_task, 10)
        if context.snapshot_is_current(first):
            raise ValueError('Disconnected history remained admitted')
        second = await connect()
        if second.key != first.key or not context.snapshot_is_current(second):
            raise ValueError('Reconnected identity/history differs')
        print(json.dumps(dict(connection_check=True, reconnect=True,
            seed=second.key.seed_name, item_count=len(second.items), game_attached=False)), flush=True)
    finally:
        await context.shutdown()


async def run(args):
    # Packaging supplies CommonClient. No research room or seeded fixture imports.
    from CommonClient import server_loop, ClientCommandProcessor
    from .steam_context_v187_candidate import SteamContext
    from .steam_launch_v187_candidate import prepare_launch
    from .steam_platform import StartupReader, running_games, require_closed
    from .steam_coordinator_v187_candidate import serve_prepared

    class Commands(ClientCommandProcessor):
        def _cmd_difficulty(self, value='status'):
            """Choose beginner, standard, proud, or critical; changes apply on reload."""
            controller = getattr(self.ctx, 'steam_difficulty', None)
            if controller is None:
                self.output('Connect and prepare your AP seed first.')
                return False
            try:
                self.output(controller.command(value))
                return True
            except ValueError as error:
                self.output(str(error))
                return False

    class Context(SteamContext):
        command_processor = Commands
        session_ended = False
        cleanup_finished = False

        async def connect(self, address=None):
            if self.session_ended or self.exit_event.is_set():
                notify('This game session has ended. Close and reopen the KH client before launching the game again.')
                return
            await super().connect(address)

        def make_gui(self):
            from kvui import GameManager
            class KH02Manager(GameManager):
                base_title = 'Kingdom Hearts 0.2 Archipelago Client'

                def on_start(self):
                    super().on_start()
                    from kivy.core.window import Window
                    Window.bind(on_request_close=self.request_close)

                def request_close(self, *args, **kwargs):
                    if self.ctx.cleanup_finished:
                        return False
                    if not self.ctx.exit_event.is_set():
                        notify('Closing AP. If KH 0.2 is running, close it normally to finish cleanup.')
                    self.ctx.exit_event.set()
                    return True

                def on_stop(self):
                    from kivy.core.window import Window
                    Window.unbind(on_request_close=self.request_close)
                    super().on_stop()
            return KH02Manager

    context = Context(args.server, args.password)
    context.auth = args.slot
    import logging
    notify = logging.getLogger('Client').info if getattr(args, 'gui', False) else print
    if getattr(args, 'gui', False):
        context.run_gui()
    else:
        context.run_cli()
    context.server_task = asyncio.create_task(server_loop(context, args.server))
    launch = None
    handed_off = False
    try:
        notify('Keep Steam signed in and KH 0.2 closed until AP is prepared. Launch 2.8 normally, then select 0.2.')
        notify('Commands: /help; /difficulty status; /difficulty beginner|standard|proud|critical. Difficulty changes apply after reloading your AP save.')
        history_wait_announced = False
        while not context.exit_event.is_set():
            if context.history_fault:
                raise ValueError(context.history_fault)
            if context.pinned_key is not None and not history_wait_announced:
                notify('Connected to AP. Checking item history and preparing isolated saves; do not launch KH yet.')
                history_wait_announced = True
            if context.history_wait_started is not None:
                import time
                if time.monotonic() - context.history_wait_started > 30:
                    raise TimeoutError('AP item history was not confirmed. The game has not been prepared.')
            try:
                context.xp_admission.current(context)
                break
            except (ValueError, RuntimeError):
                await asyncio.sleep(.25)
        if context.exit_event.is_set():
            return
        if getattr(args, 'gui', False):
            from .steam_discovery import automatic_setup
            args.exe, args.steam_id = automatic_setup()
        # Keep preparation synchronous with respect to AP packet handling, as the
        # existing StorageSession publish contract requires. Every boundary checks
        # the admitted snapshot, installation bytes and closed-game state.
        launch = prepare_launch(context, executable=args.exe, profiles_root=args.profiles,
            package_root=args.package, steam_id=args.steam_id, player_name=context.auth,
            running_games=running_games, require_closed=require_closed,
            resume_target=args.resume_slot)
        from .steam_difficulty import DifficultySession
        context.steam_difficulty = DifficultySession(launch.profile/'preferences',
            launch.prepared.storage.key, notify=notify)
        reader = StartupReader(args.exe, Path(args.exe).parent/'dinput8.dll',
            expected_dll_sha256=launch.runtime.hashes['startup'],
            packaged_dll=launch.runtime.startup)
        from .stable_runtime_v245 import RuntimeLease
        with RuntimeLease(require_closed=require_closed) as runtime_lease:
            transport=runtime_lease.stage(launch.runtime.transport,launch.runtime.hashes['transport'])
            handed_off = True
            await serve_prepared(launch.session, launch.prepared, reader=reader,
                installation=launch.installation, transport_dll=transport,
                transport_sha256=launch.runtime.hashes['transport'],
                session_directory=launch.directory, journal_root=launch.profile/'shared-client',
                running_games=running_games, stop=context.exit_event,
                notify=notify)
        # The coordinator has completed cleanup. This window must not claim
        # readiness for a second game process by reconnecting only the server.
        context.session_ended = True
        context.cleanup_finished = True
        await context.disconnect()
        notify('Session finished. Close and reopen the KH client before your next game launch.')
        if not context.exit_event.is_set():
            await context.exit_event.wait()
    except Exception as error:
        context.session_ended = True
        if not getattr(args, 'gui', False):
            raise
        if launch is not None and not handed_off and not running_games():
            launch.installation.restore()
            launch = None
        logging.getLogger('Client').exception('Client stopped: %s', error)
        await context.disconnect()
        notify('Preparation stopped. Keep the game closed; close and reopen this client after correcting the error.')
        # Keep the error visible in the AP window. Native cleanup is completed by
        # serve_prepared before it propagates an error.
        await context.exit_event.wait()
    finally:
        if launch is not None and not handed_off:
            # Reader construction can fail after installation. Preserve files if
            # the user has already launched; do not force-close or remove live files.
            if running_games():
                notify('Close KH 0.2 normally to finish restoring its startup files.')
                while running_games():
                    await asyncio.sleep(1)
            launch.installation.restore()
        context.session_ended = True
        context.cleanup_finished = True
        if getattr(args, 'gui', False) and context.ui is not None:
            context.ui.stop()
        await context.shutdown()


def main():
    parser = argparse.ArgumentParser(description='Kingdom Hearts 0.2 Archipelago Steam client')
    parser.add_argument('--server', help='Archipelago server:port')
    parser.add_argument('--slot', help='Player slot name')
    parser.add_argument('--password')
    parser.add_argument('--exe', type=Path, help='Supported Steam game executable')
    parser.add_argument('--connection-check', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--steam-id', help='17-digit Steam account ID for save verification')
    parser.add_argument('--recover', type=Path, help='Restore startup files from this AP session folder, with the game closed')
    parser.add_argument('--package', type=Path, default=(Path(sys.executable).parent
        if getattr(sys, 'frozen', False) else Path(__file__).resolve().parents[2]))
    parser.add_argument('--profiles', type=Path,
        default=Path(os.environ.get('LOCALAPPDATA', Path.home()))/'KH02AP/profiles')
    parser.add_argument('--resume-slot', type=int, choices=(0, 1), default=1,
                        help='Initial load: 0 autosave, 1 manual slot 1 (default)')
    arguments = None
    if len(sys.argv) == 1:
        arguments = prompt_settings(Path(os.environ.get('LOCALAPPDATA', Path.home()))/'KH02AP/client')
    args = parser.parse_args(arguments)
    if args.connection_check:
        if not args.server or not args.slot:
            parser.error('--server and --slot are required for a connection check')
        asyncio.run(connection_check(args))
        return
    if args.recover:
        if not args.exe:
            parser.error('--exe is required for recovery')
        from .steam_recovery import recover
        from .steam_platform import require_closed
        recover(args.recover, executable=args.exe, require_closed=require_closed)
        print('Normal startup restored. AP saves and receipts were preserved.')
        return
    if not all((args.server, args.slot, args.steam_id, args.exe)):
        parser.error('--server, --slot, --steam-id and --exe are required unless using --recover')
    asyncio.run(run(args))


if __name__ == '__main__':
    main()
