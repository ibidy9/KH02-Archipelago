from .world import KH02World

from worlds.LauncherComponents import Component, Type, components
from .launcher import launch
components.append(Component("KH 0.2 Client", func=launch, component_type=Type.CLIENT))
