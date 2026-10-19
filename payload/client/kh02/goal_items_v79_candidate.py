"""Additive goal-item identities for candidate schema5; existing IDs unchanged.

Not yet a generated-world pool or native delivery registration. Wayfinders are
logical items. Zodiac AP IDs are distinct from their established native codes.
"""
from types import MappingProxyType

ITEM_SCHEMA_VERSION=5
WAYFINDER_IDS=MappingProxyType({
    "Terra's Wayfinder":0x020220,
    "Aqua's Wayfinder":0x020221,
    "Ventus's Wayfinder":0x020222,
})
ZODIAC_NAMES=('Aries','Taurus','Gemini','Cancer','Leo','Virgo','Libra','Scorpio',
              'Sagittarius','Capricorn','Aquarius','Pisces')
ZODIAC_IDS=MappingProxyType({name+' Relic':0x020223+i for i,name in enumerate(ZODIAC_NAMES)})
ZODIAC_NATIVE_CODES=MappingProxyType({ZODIAC_IDS[name+' Relic']:0x900+i
    for i,name in enumerate(ZODIAC_NAMES)})
GOAL_ITEM_IDS=MappingProxyType(dict(WAYFINDER_IDS)|dict(ZODIAC_IDS))
