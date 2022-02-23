from .booleans.expressions import Expression
from .promela import parse as parse_promela
from .booleans.parser import parse as parse_boolean_expression
from .ltl2ba_wrapper import run_ltl2ba
from .graph import Graph

empty_set = frozenset()


class Buchi(Graph):
    def guard(self, src, dst) -> Expression:
        """
        Returns the guard expression for the edge between src and dst.
        Throws KeyError if the edge does not exist.
        """
        return self.edge_attrs[(src, dst)]["guard"]

    @classmethod
    def from_promela(cls, promela):
        buchi = cls()
        buchi.initial |= promela.initial_states
        buchi.accept |= promela.accept_states
        for (src, dst), guard_formula in promela.edges.items():
            guard = parse_boolean_expression(guard_formula)
            buchi.add_edge(src, dst, guard=guard)
        return buchi

    @classmethod
    def from_ltl(cls, formula: str) -> "Buchi":
        promela_output = run_ltl2ba(formula)
        promela_output = parse_promela(promela_output)
        return cls.from_promela(promela_output)
