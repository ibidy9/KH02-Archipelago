"""Release composition of the separately proven check reporters."""
from pathlib import Path

from .chest_checks_v125_candidate import CombinedChecks
from .level_reporting_v186_candidate import ProgressJournal, ProgressReporter


class SteamChecks:
    def __init__(self, owner, *, journal_root):
        self.context = owner.session.context
        root = Path(journal_root)
        root.mkdir(parents=True, exist_ok=True)
        self.journal = ProgressJournal(root / 'progress.sqlite', key=owner.prepared.storage.key)
        self.closed = False
        try:
            self.base = CombinedChecks(owner)
            self.progress = ProgressReporter(self.context,
                key=owner.prepared.storage.key, cookie=owner.prepared.cookie,
                read=owner.read_progress, current=owner.objective_report_current,
                journal=self.journal)
        except BaseException:
            self.close()
            raise

    async def poll(self):
        if self.closed:
            raise RuntimeError('Check reporters are closed')
        snapshot = self.context.current_snapshot()
        result = tuple(await self.base.poll())
        if not self.context.snapshot_is_current(snapshot):
            return result
        return result + tuple(await self.progress.poll())

    def close(self):
        if not self.closed:
            self.closed = True
            self.journal.close()
