"""Candidate plain-text policy. The pixel-fit budget requires live calibration."""
from dataclasses import dataclass
import unicodedata

@dataclass(frozen=True)
class PickupText:
    text: str
    full_text: str
    fallback: bool

def units(text: str) -> int:
    return len(text.encode('utf-16-le'))//2

def plain(text: str) -> str:
    if not isinstance(text,str) or len(text)>4096:
        raise ValueError('Expected a bounded item/player name')
    # This UI supports markup. Names must never supply tags, entities, control
    # characters or unpaired surrogates. Retain ordinary Unicode and apostrophes.
    substitutions={'<':'[','>':']','&':'+'}
    text=''.join(' ' if unicodedata.category(c).startswith('C') else substitutions.get(c,c) for c in text)
    return ' '.join(text.split())

def format_pickup(item: str, *, recipient: str | None=None,
                  recipient_slot: int | None=None, budget: int=48) -> PickupText:
    """Received/local: item only. Outgoing: item for recipient. Never sender."""
    if type(budget) is not int or not 16<=budget<=160:
        raise ValueError('Unsupported diagnostic text budget')
    name=plain(item) or 'Archipelago Item'
    if recipient is None:
        if recipient_slot is not None:raise ValueError('Slot requires an outgoing recipient')
        full=name;candidates=(full,'Archipelago Item')
    else:
        if type(recipient_slot) is not int or not 1<=recipient_slot<=65535:
            raise ValueError('Outgoing recipient requires a valid AP slot')
        alias=plain(recipient) or f'Player {recipient_slot}'
        full=f'{name} for {alias}'
        candidates=(full,f'Item for {alias}',f"Player {recipient_slot}'s Item",'Archipelago Item')
    text=next(candidate for candidate in candidates if units(candidate)<=budget)
    return PickupText(text,full,text!=full)

def diagnostic_cases():
    """Full long text is intentional for measuring the UI, not production use."""
    long_item='The Exceptionally Long Legendary Sword of the Seven Forgotten Kingdoms'
    long_player='Player With An Exceptionally Long Name'
    return [
        ('local-wayfinder',format_pickup("Terra's Wayfinder")),
        ('received-ability',format_pickup('Double Flight')),
        ('outgoing-short',format_pickup('Sword',recipient='Player 2',recipient_slot=2)),
        ('outgoing-long-unshortened',format_pickup(long_item,recipient=long_player,recipient_slot=2,budget=160)),
        ('outgoing-long-fallback',format_pickup(long_item,recipient=long_player,recipient_slot=2,budget=32)),
    ]
