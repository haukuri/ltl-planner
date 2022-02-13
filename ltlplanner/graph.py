from __future__ import annotations

from abc import ABC, abstractmethod

from .utils import BidirectionalEdgeMap

class Graph(ABC):
    @abstractmethod
    def post(self, src) -> set[str]:
        pass

    @abstractmethod
    def pre(self, dst) -> set[str]:
        pass

class ConcreteGraph(Graph):
    def __init__(self):
        self.__edges = BidirectionalEdgeMap()

    def add_edge(self, src, dst):
        return self.__edges.add(src, dst)

    def post(self, src):
        return self.__edges.post(src)

    def pre(self, dst):
        return self.__edges.pre(dst)
