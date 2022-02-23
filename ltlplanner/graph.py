from .utils import BidirectionalEdgeMap


class Graph:
    def __init__(self):
        self.__edges = BidirectionalEdgeMap()
        self.edge_attrs = {}
        self.initial = set()
        self.accept = set()

    def add_edge(self, src, dst, **attributes):
        self.__edges.add(src, dst)
        if attributes:
            self.edge_attrs[(src, dst)] = attributes

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
