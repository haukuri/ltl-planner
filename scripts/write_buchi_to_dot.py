from ltlplanner.buchi import Buchi
from ltlplanner.visualization import DotWriter

ltl = '[]<>a && []<>b '
buchi = Buchi.from_ltl(ltl)
writer = DotWriter(name='buchi')
writer.read_graph(buchi)
dot = writer.to_string()
print(dot)
