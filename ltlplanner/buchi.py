from ltlplanner.booleans.expressions import Expression

from .promela import PromelaOutput, parse as parse_promela, find_states
from .booleans.parser import parse as parse_boolean_expression
from .ltl2ba_wrapper import run_ltl2ba

class Buchi:
    def __init__(self, promela_output: PromelaOutput):
        self.initial = set(promela_output.initial_states)
        self.accept = set(promela_output.accept_states)
        self._guards_from = {}
        for (src, dst), guard_formula in promela_output.edges.items():
            try:
                guards = self._guards_from[src]
            except KeyError:
                guards = {}
                self._guards_from[src] = guards
            guard = parse_boolean_expression(guard_formula)
            guards[dst] = guard
    
    def guard(self, src, dst) -> Expression:
        """
        Returns the guard expression for the edge between src and dst.
        Throws KeyError if the edge does not exist.
        """
        return self._guards_from[src][dst]


def from_ltl(formula: str) -> Buchi:
    promela_output = run_ltl2ba(formula)
    promela_output = parse_promela(promela_output)
    buchi = Buchi(promela_output)
    return buchi
