"""Normal continuous service lifecycle; no test stages or synthetic rewards."""
import asyncio
import json
import os
import subprocess
import time
from pathlib import Path

from .controlled_attempt_log_candidate import record_attempt
from .transition_wait_v212_candidate import TransitionWaitTransport as LevelTransport
from .shutdown_transport_v191_candidate import GameConnectionEnded
from .steam_service_v187_candidate import SteamService
from tools.save_filename_support import filetime


def record_policy(directory, entry):
    sequence, state = entry.get('sequence'), entry.get('state')
    if type(sequence) is not int or sequence < 1 or state not in ('reserved', 'acknowledged'):
        raise ValueError('Invalid policy diagnostic')
    with (Path(directory) / f'{sequence:08d}-{state}.json').open('x', encoding='utf-8') as stream:
        json.dump(entry, stream, sort_keys=True)
        stream.flush()
        os.fsync(stream.fileno())


async def serve_prepared(session, prepared, *, reader, installation, transport_dll,
                         transport_sha256, session_directory, journal_root,
                         running_games, stop, notify=print,
                         owner_factory=LevelTransport, service_factory=SteamService,
                         clock=filetime, interval=.5):
    """Wait for manual launch, attach once, and retain native resources until exit.

    Installation/preparation has already completed. There are no user-controlled
    stage files. Stop means stop delivery, never terminate the game. Cleanup must
    finish before another launch/session is permitted.
    """
    if not .05 <= interval <= 5:
        raise ValueError('Bounded service interval required')
    last_status = None
    waiting_since = None
    waiting_reported = False
    active_seen = False
    def report_status(status):
        nonlocal last_status, waiting_since, waiting_reported, active_seen
        state = status.state
        if state == 'active':
            if not active_seen or waiting_reported:
                notify('AP status: active')
            active_seen = True
            waiting_since = None
            waiting_reported = False
        elif state == 'waiting_for_owner':
            if waiting_since is None:
                waiting_since = time.monotonic()
            # Initial title-screen waiting is expected. Report prolonged loss
            # after admission, while keeping ordinary room transitions quiet.
            if active_seen and not waiting_reported and time.monotonic() - waiting_since >= 30:
                notify('AP is waiting for the loaded game. If you have control, pause and check the connection before continuing.')
                waiting_reported = True
        else:
            waiting_since = None
            if state != last_status:
                notify('AP status: ' + state)
        last_status = state

    owner = service = None
    detection_delayed = False
    async def inventory():
        # A timed-out subprocess is reaped by subprocess.run. Its absence of
        # output is NOT evidence that the game is absent. Retry only discovery;
        # never resend an owner command or weaken startup proof validation.
        nonlocal detection_delayed
        try:
            games = await asyncio.to_thread(running_games)
        except subprocess.TimeoutExpired:
            if not detection_delayed:
                notify('Game detection is delayed; retrying. Keep the client open and wait for AP to attach before loading.')
                detection_delayed = True
            return None
        if detection_delayed:
            notify('Game detection recovered.')
            detection_delayed = False
        return games
    attempts = Path(session_directory) / 'attempts'
    attempts.mkdir(exist_ok=False)
    policies = Path(session_directory) / 'policies'
    policies.mkdir(exist_ok=False)
    try:
        notify('AP is prepared. Launch the game yourself and wait at the title screen.')
        while not stop.is_set():
            if clock() >= prepared.storage.expires:
                raise TimeoutError('Prepared launch expired; reconnect with the game closed')
            games = await inventory()
            if games is None:
                await asyncio.sleep(interval)
                continue
            # A query can span sleep/resume. Recheck the original authorization
            # deadline before observing or admitting a process; never extend it.
            if clock() >= prepared.storage.expires:
                raise TimeoutError('Prepared launch expired; reconnect with the game closed')
            proof = await asyncio.to_thread(reader.observe, prepared.storage, games=games)
            if proof is None:
                await asyncio.sleep(interval)
                continue
            # The proof came from this precise process inventory, not a fabricated
            # ready proof for each process with a matching name.
            if len(games) != 1 or not games[0].get('window'):
                await asyncio.sleep(interval)
                continue
            session.admit_process(proof, now=clock(), running_games=lambda: (proof,))
            owner = owner_factory(transport_dll, dll_sha256=transport_sha256,
                session=session, prepared=prepared, proof=proof, window=games[0]['window'],
                record=lambda entry: record_attempt(attempts, entry))
            session.context.native_revoke = owner.revoke
            await asyncio.to_thread(owner.start)
            await owner.configure_mickey()
            await owner.configure_xp()
            service = service_factory(owner, journal_root=journal_root,
                                      record=lambda entry: record_policy(policies, entry))
            notify('AP is attached. You may start or load this seed’s game.')
            await service.run(stop, interval=interval,
                              observe=report_status)
            break
    except GameConnectionEnded:
        notify('Game connection ended. Waiting for the original game process to finish closing.')
    except Exception as error:
        notify('AP attachment/service failed: ' + str(error))
        raise
    finally:
        # Do not unload a hook into a running process, including on interruption.
        # This awaited task is shielded so cancellation still waits for normal exit.
        async def cleanup():
            try:
                if service is not None:
                    service.close()
            finally:
                if owner is not None:
                    owner.revoke()
                    try:
                        await asyncio.to_thread(owner.stop)
                    except Exception:
                        if not owner.arm_sent:
                            raise
                        notify('Native cleanup is pending; keep the client open and close the game normally.')
                    if not owner.closed:
                        notify('Delivery stopped. Close KH 0.2 normally to finish cleanup.')
                        while not await asyncio.to_thread(owner.game_exited):
                            await asyncio.sleep(interval)
                        await asyncio.to_thread(owner.stop)
                # A game may have launched before admission or during an error.
                # Keep its installed startup files until it has closed normally.
                warned = False
                while True:
                    games = await inventory()
                    if games is None:
                        await asyncio.sleep(interval)
                        continue
                    if not games:
                        break
                    if not warned:
                        notify('Close KH 0.2 normally to restore startup files.')
                        warned = True
                    await asyncio.sleep(interval)
                await asyncio.to_thread(installation.restore)
                session.context.native_revoke = lambda: None
                notify('Game closed. AP saves retained; normal startup restored.')
        cleanup_task = asyncio.create_task(cleanup())
        while not cleanup_task.done():
            try:
                await asyncio.shield(cleanup_task)
            except asyncio.CancelledError:
                continue
        cleanup_task.result()
