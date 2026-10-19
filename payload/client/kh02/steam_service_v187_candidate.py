"""Continuous release service using the proven shared owner and all checks.

Startup/storage admission belongs to the platform adapter. This class never
launches the game or replaces saves and does not supply synthetic AP rewards.
"""
from pathlib import Path

from .mickey_shared_client_v183_candidate import SharedClient
from .level_checks_v186_candidate import SteamChecks


class SteamService(SharedClient):
    def __init__(self, owner, *, journal_root, record, query_record=lambda value: None):
        self.steam_checks = None
        self.traps = None
        self.death_link=None

        def make_checks(admitted_owner):
            self.steam_checks = SteamChecks(admitted_owner, journal_root=Path(journal_root) / 'checks')
            return self.steam_checks

        try:
            super().__init__(owner, journal_root=journal_root, record=record,
                             query_record=query_record, check_factory=make_checks)
        except BaseException:
            if self.steam_checks is not None:
                self.steam_checks.close()
            raise

        from .traps_v244 import TrapService
        try:
            self.traps = TrapService(owner,Path(journal_root)/'traps',self.context.current_snapshot().key)
            from .deathlink_v249 import DeathLinkService
            self.death_link=DeathLinkService(owner,self.context)
        except BaseException:
            self.close()
            raise

    async def tick(self):
        status = await super().tick()
        if self.death_link is not None:
            try:
                await self.death_link.tick(active=status.state=='active',
                    can_apply=status.delivery=='complete' and self.delivery.pending is None)
            except BaseException:
                self.owner.revoke();self.faulted=True;raise
        difficulty = getattr(self.context, 'steam_difficulty', None)
        if status.state == 'active' and difficulty is not None:
            try:
                await difficulty.tick(self.owner)
            except BaseException:
                self.owner.revoke()
                self.faulted = True
                raise
        if self.traps is not None:
            try:
                if status.state == 'active' and status.delivery == 'complete':
                    await self.traps.tick(self)
                else:
                    self.traps.defer()
            except BaseException:
                self.owner.revoke(); self.faulted = True
                raise
        return status

    def close(self):
        try:
            if self.death_link is not None:
                self.death_link.close(); self.death_link = None
            if self.traps is not None:
                self.traps.close(); self.traps = None
            super().close()
        finally:
            if self.steam_checks is not None:
                self.steam_checks.close()
