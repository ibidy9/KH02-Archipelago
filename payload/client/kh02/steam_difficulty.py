"""Client difficulty command over the proven next-load preference controller."""
from .difficulty_preference_candidate import DifficultyPreferenceController, DifficultyPreferenceStore


class DifficultySession:
    def __init__(self, directory, key, *, notify=print):
        self.controller = DifficultyPreferenceController(DifficultyPreferenceStore(directory), key)
        self.preference = self.controller.store.load(key)
        self.last = None
        self.notify = notify

    def command(self, argument='status'):
        if argument.strip().lower() in ('', 'status'):
            if self.preference is None:
                return 'No client difficulty override is selected.'
            return f'Selected difficulty: {self.preference.desired.name.title()}. Changes apply after reloading your AP save.'
        self.preference = self.controller.choose(argument)
        self.last = None
        return f'{self.preference.desired.name.title()} selected. Wait for the client to confirm it is queued, then reload your AP save to apply it.'

    async def tick(self, owner):
        if self.preference is None:
            return
        observed = await owner.probe()
        if observed is None:
            return
        epoch, _ = observed
        identity = epoch, self.preference.revision
        if identity == self.last:
            return
        view = await self.controller.reconcile(owner, epoch=epoch)
        if view.current_matches:
            self.notify(f'Difficulty is now {self.preference.desired.name.title()}.')
            self.last = identity
        elif view.queued:
            self.notify('Difficulty change queued. Reload your AP save to apply it.')
            self.last = identity
