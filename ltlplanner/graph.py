from collections import defaultdict

from .utils import BidirectionalEdgeMap


def _defaultdict_of_dict():
    return defaultdict(dict)


class Graph:
    def __init__(self):
        self.__edges = BidirectionalEdgeMap()
        self.edge_attrs = defaultdict(_defaultdict_of_dict)
        self.node_attrs = defaultdict(_defaultdict_of_dict)
        self.initial = set()
        self.accept = set()

    def add_edge(self, src, dst, **attributes):
        self.__edges.add(src, dst)
        if attributes:
            self.edge_attrs[(src, dst)].update(attributes)

    def post(self, src):
        return self.__edges.post(src)

    def pre(self, dst):
        return self.__edges.pre(dst)

    def edges(self):
        return self.__edges.edges()

    def nodes(self):
        nodes = set()
        for (s, d) in self.edges():
            nodes.add(s)
            nodes.add(d)
        return nodes
