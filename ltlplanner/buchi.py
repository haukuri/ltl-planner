import networkx as nx

from .promela import parse as parse_promela, find_states
from .booleans.parser import parse as parse_boolean_expression
from .ltl2ba_wrapper import run_ltl2ba

class BuchiGraph(nx.DiGraph):
    def __init__(self, initial: set[str], accept: set[str], incoming_graph_data=None, **attr):
        super().__init__(incoming_graph_data, **attr)
        self.initial = initial if initial else set()
        self.accept = accept if accept else set()

def from_ltl(formula: str) -> BuchiGraph:
    promela_output = run_ltl2ba(formula)
    promela_output = parse_promela(promela_output)
    buchi = BuchiGraph(promela_output.initial_states, promela_output.accept_states)
    for (src, dst), guard_formula in promela_output.edges.items():
        guard = parse_boolean_expression(guard_formula)
        buchi.add_edge(src, dst, guard=guard)
    return buchi
