from .booleans.expressions import Expression
from .promela import PromelaOutput, parse as parse_promela
from .booleans.parser import parse as parse_boolean_expression
from .ltl2ba_wrapper import run_ltl2ba
from .graph import ConcreteGraph

empty_set = frozenset()


class Buchi(ConcreteGraph):
    def __init__(self, promela_output: PromelaOutput):
        super().__init__()
        self.initial = set(promela_output.initial_states)
        self.accept = set(promela_output.accept_states)
        guards = {}
        for (src, dst), guard_formula in promela_output.edges.items():
            guards[(src, dst)] = parse_boolean_expression(guard_formula)
            self.add_edge(src, dst)
        self.__guards = guards

    def guard(self, src, dst) -> Expression:
        """
        Returns the guard expression for the edge between src and dst.
        Throws KeyError if the edge does not exist.
        """
        return self.__guards[(src, dst)]


def from_ltl(formula: str) -> Buchi:
    promela_output = run_ltl2ba(formula)
    promela_output = parse_promela(promela_output)
    buchi = Buchi(promela_output)
    return buchi
