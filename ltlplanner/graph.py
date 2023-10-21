from collections import defaultdict
from typing import Iterator


def _defaultdict_of_dict():
    return defaultdict(dict)


class BidirectionalEdgeMap:
    __post: defaultdict[str, set[str]]
    __pre: defaultdict[str, set[str]]

    def __init__(self):
        self.__post = defaultdict(set)
        self.__pre = defaultdict(set)

    def add(self, a: str, b: str):
        self.post(a).add(b)
        self.pre(b).add(a)

    def remove(self, a: str, b: str):
        self.post(a).remove(b)
        self.pre(b).remove(a)

    def post(self, a: str) -> set[str]:
        return self.__post[a]

    def pre(self, b: str) -> set[str]:
        return self.__pre[b]

    def edges(self) -> Iterator[tuple[str, str]]:
        for a, post in self.__post.items():
            for b in post:
                yield (a, b)


class Graph:
    edge_attrs: defaultdict[tuple[str, str], dict]
    node_attrs: defaultdict[str, dict]
    initial: set[str]
    accept: set[str]
    __edges: BidirectionalEdgeMap

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

    def post(self, src) -> set[str]:
        return self.__edges.post(src)

    def pre(self, dst) -> set[str]:
        return self.__edges.pre(dst)

    def edges(self) -> Iterator[tuple[str, str]]:
        return self.__edges.edges()

    def nodes(self) -> set[str]:
        nodes = set()
        for (s, d) in self.edges():
            nodes.add(s)
            nodes.add(d)
        return nodes
