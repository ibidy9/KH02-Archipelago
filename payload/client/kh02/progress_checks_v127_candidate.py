"""Pure candidate mapping only. Not wired to a live reader or public client.

Offsets describe the audited serialized switch block 0x3010..0x3015.
These flags can reset. Reporting/durable completion must not treat this decoder
as a permanent ledger, and neither story nor AP ability ownership is evidence.
"""
SWITCH_CHECKS=(
    (0x020421,1,0x10),(0x020422,1,0x20),
    (0x020423,1,0x40),(0x020424,1,0x80),
    (0x020430,2,0x40),(0x020431,3,0x01),
    (0x020432,2,0x80),(0x020433,3,0x02),
    (0x020434,2,0x20),(0x020435,2,0x10),(0x020436,2,0x08),
)

def completed_switch_checks(flags):
    if type(flags) is not bytes or len(flags)!=6:
        raise ValueError('Exact six-byte audited switch block required')
    return frozenset(location for location,offset,mask in SWITCH_CHECKS if flags[offset]&mask)

def completed_level_checks(level):
    # Caller must obtain the actual level from an authenticated stable owner;
    # this function does not establish a native field address or trust a save.
    if type(level) is not int or not 50<=level<=99:
        raise ValueError('Validated KH02 character level required')
    return frozenset(0x020400+n-51 for n in range(51,min(level,70)+1))
