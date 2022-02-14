from .utils import BidirectionalEdgeMap

class Graph:
    def __init__(self):
        self.__edges = BidirectionalEdgeMap()

    def add_edge(self, src, dst):
        return self.__edges.add(src, dst)

    def post(self, src):
        return self.__edges.post(src)

    def pre(self, dst):
        return self.__edges.pre(dst)

    def edges(self):
        return self.__edges.edges()