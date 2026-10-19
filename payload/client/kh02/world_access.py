from __future__ import annotations

from dataclasses import dataclass
from enum import Enum


WORLD_NAMES = (
    "Castle Town",
    "The World Within",
    "Forest of Thorns",
    "Depths of Darkness",
)
WORLD_ACCESS_ITEM_IDS = {
    "Castle Town": 0x020212,
    "The World Within": 0x020213,
    "Forest of Thorns": 0x020214,
    "Depths of Darkness": 0x020215,
}
ALL_WORLD_MASK = (1 << len(WORLD_NAMES)) - 1


@dataclass(frozen=True)
class NativeDestinationSignature:
    key: str
    world_name: str
    map_path: str
    spawn_tag: str


# Captured native save-point routes. These are validation signatures for live
# records, not permission to synthesize records before the native owner and
# bounds have been proven.
NATIVE_DESTINATION_SIGNATURES = (
    NativeDestinationSignature(
        "castle-town", "Castle Town", "/Game/Levels/dw/dw_02/dw_02",
        "dw_02_Lv_Save_01",
    ),
    NativeDestinationSignature(
        "world-within", "The World Within", "/Game/Levels/dw/dw_03/dw_03",
        "dw_03_Lv_Save_01",
    ),
    NativeDestinationSignature(
        "forest-uncertain", "Forest of Thorns", "/Game/Levels/dw/dw_08/dw_08",
        "dw_08_Lv_Save_01",
    ),
    NativeDestinationSignature(
        "forest-rocky", "Forest of Thorns", "/Game/Levels/dw/dw_08/dw_08",
        "dw_08_Lv_Save_02",
    ),
    NativeDestinationSignature(
        "forest-paths-end", "Forest of Thorns", "/Game/Levels/dw/dw_08/dw_08",
        "dw_08_Lv_Save_03",
    ),
    NativeDestinationSignature(
        "depths", "Depths of Darkness", "/Game/Levels/dw/dw_09/dw_09",
        "dw_09_Lv_Save_01",
    ),
)

# The four extra rows supplied by the removable AreaSelectData patch.  These
# routes have passed live travel/restart validation and are gated by the same
# major-world item as their normal destination.
RESTART_DESTINATION_SIGNATURES = (
    NativeDestinationSignature(
        "restart-castle-town", "Castle Town", "/Game/Levels/dw/dw_01/dw_01",
        "dw_01_Lv_Start_01",
    ),
    NativeDestinationSignature(
        "restart-world-within", "The World Within", "/Game/Levels/dw/dw_03/dw_03",
        "dw_03_Lv_Start_01",
    ),
    NativeDestinationSignature(
        "restart-forest", "Forest of Thorns", "/Game/Levels/dw/dw_08/dw_08",
        "dw_08_Lv_Start_01",
    ),
    NativeDestinationSignature(
        "restart-depths", "Depths of Darkness", "/Game/Levels/dw/dw_09/dw_09",
        "dw_09_Lv_Start_01",
    ),
)

TELEPORT_ROUTE_SIGNATURES = NATIVE_DESTINATION_SIGNATURES + RESTART_DESTINATION_SIGNATURES


def native_destination_for_route(
    map_path: str, spawn_tag: str,
) -> NativeDestinationSignature:
    matches = tuple(
        destination for destination in TELEPORT_ROUTE_SIGNATURES
        if destination.map_path == map_path and destination.spawn_tag == spawn_tag
    )
    if len(matches) != 1:
        raise ValueError(f"Unsupported native teleport route: {map_path} / {spawn_tag}")
    return matches[0]


@dataclass(frozen=True)
class WorldAccessState:
    randomized: bool
    starting_world: str
    owned_mask: int

    def owns(self, world_name: str) -> bool:
        try:
            bit = WORLD_NAMES.index(world_name)
        except ValueError as error:
            raise ValueError(f"Unknown KH 0.2 world: {world_name}") from error
        return bool(self.owned_mask & (1 << bit))

    def owned_worlds(self) -> tuple[str, ...]:
        return tuple(name for name in WORLD_NAMES if self.owns(name))


@dataclass(frozen=True)
class TeleportDestination:
    """Game-discovered destination annotated with its AP major world."""

    key: str
    world_name: str
    is_current: bool = False
    native_available: bool = True
    enabled: bool = True


class WorldExitAction(Enum):
    VANILLA = "vanilla"
    RETURN_TO_COMPLETED_WORLD = "return_to_completed_world"


class WorldRouteAction(Enum):
    ALLOW = "allow"
    BLOCK_LOCKED = "block_locked"
    REFUSE_UNKNOWN = "refuse_unknown"


@dataclass(frozen=True)
class WorldRouteDecision:
    action: WorldRouteAction
    destination: NativeDestinationSignature | None

    @property
    def allowed(self) -> bool:
        return self.action is WorldRouteAction.ALLOW


def guard_native_world_route(
    map_path: str,
    spawn_tag: str,
    access: WorldAccessState,
) -> WorldRouteDecision:
    """Fail-closed policy for the eventual game-side transition hook."""
    try:
        destination = native_destination_for_route(map_path, spawn_tag)
    except ValueError:
        return WorldRouteDecision(WorldRouteAction.REFUSE_UNKNOWN, None)
    if not access.randomized or access.owns(destination.world_name):
        return WorldRouteDecision(WorldRouteAction.ALLOW, destination)
    return WorldRouteDecision(WorldRouteAction.BLOCK_LOCKED, destination)


@dataclass(frozen=True)
class WorldResetPolicy:
    """AP-owned reset policy; game-side routing is deliberately separate.

    A reset may rewind local story/encounter state, but AP locations and player
    progression are never reset.  Availability begins when the world is owned,
    not when it is completed.
    """

    world_name: str
    available: bool
    preserve_chests: bool = True
    preserve_objectives: bool = True
    preserve_player_state: bool = True


def world_reset_policy(world_name: str, access: WorldAccessState) -> WorldResetPolicy:
    if world_name not in WORLD_NAMES:
        raise ValueError(f"Unknown KH 0.2 world: {world_name}")
    return WorldResetPolicy(world_name=world_name, available=access.owns(world_name))


def filter_teleport_destinations(
    destinations: tuple[TeleportDestination, ...],
    access: WorldAccessState,
) -> tuple[TeleportDestination, ...]:
    """Apply AP ownership while preserving the native current-world entry.

    The current destination stays in the list so the game can render it greyed
    out normally. The eventual game-side adapter must discover destination
    records natively and call this policy; it must not manufacture save points.
    """
    for destination in destinations:
        if destination.world_name not in WORLD_NAMES:
            raise ValueError(
                f"Unsupported teleport destination world: {destination.world_name}"
            )
    if not access.randomized:
        return destinations
    return tuple(
        destination for destination in destinations
        if destination.is_current or access.owns(destination.world_name)
    )


def gate_teleport_destinations(
    destinations: tuple[TeleportDestination, ...],
    access: WorldAccessState,
) -> tuple[TeleportDestination, ...]:
    """Return every native row with its final enabled state.

    AP ownership can only make a row more restrictive.  It never bypasses the
    game's own availability for save points that have not been reached yet.
    The current row also remains disabled through the normal native rule.
    """
    gated = []
    for destination in destinations:
        if destination.world_name not in WORLD_NAMES:
            raise ValueError(
                f"Unsupported teleport destination world: {destination.world_name}"
            )
        ap_available = not access.randomized or access.owns(destination.world_name)
        gated.append(TeleportDestination(
            key=destination.key,
            world_name=destination.world_name,
            is_current=destination.is_current,
            native_available=destination.native_available,
            enabled=(
                destination.enabled
                and destination.native_available
                and ap_available
                and not destination.is_current
            ),
        ))
    return tuple(gated)


def world_exit_action(*, randomized: bool) -> WorldExitAction:
    """Policy for a normal world-completion transition.

    Destination/spawn selection intentionally remains outside this pure policy
    until each native first-save record has been captured and verified.
    """
    return (
        WorldExitAction.RETURN_TO_COMPLETED_WORLD
        if randomized else WorldExitAction.VANILLA
    )


def received_world_access(
    item_ids, *, randomized: bool, starting_world: str,
) -> WorldAccessState:
    if starting_world not in WORLD_NAMES:
        raise ValueError(f"Unsupported starting world: {starting_world}")
    if not randomized:
        return WorldAccessState(False, starting_world, ALL_WORLD_MASK)
    received = set(item_ids)
    mask = 1 << WORLD_NAMES.index(starting_world)
    for bit, world_name in enumerate(WORLD_NAMES):
        if WORLD_ACCESS_ITEM_IDS[world_name] in received:
            mask |= 1 << bit
    return WorldAccessState(True, starting_world, mask)
