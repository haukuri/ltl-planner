from ltlplanner.ts import rectworld
from ltlplanner.visualization import DotWriter

world = rectworld(3, 3)
writer = DotWriter(name='transition_system')
writer.read_graph(world)
dot = writer.to_string()
print(dot)