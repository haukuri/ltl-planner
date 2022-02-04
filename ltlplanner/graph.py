from __future__ import annotations

from abc import ABC, abstractmethod

from .booleans.expressions import TrueExpression

true_expression = TrueExpression()


class Graph(ABC):
    @abstractmethod
    def post(self, src) -> set[str]:
        pass

    @abstractmethod
    def pre(self, dst) -> set[str]:
        pass
