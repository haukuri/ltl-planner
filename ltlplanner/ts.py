from collections import defaultdict

from .graph import Graph

class TransitionSystem(Graph):
	def __init__(self):
		super().__init__()
		self.__labels = defaultdict(set)

	def labels(self, node):
		return self.__labels[node]

def rectworld(rows, columns, initial_state):
	def neighbors_of(r, c):
		delta = (-1, 0, 1)
		for dr in delta:
			nr = r + dr
			if nr < 0 or nr >= rows:
				continue
			for dc in delta:
				nc = c + dc
				if nc < 0 or nc >= columns:
					continue
				if nr == r and nc == c:
					continue
				yield nr, nc
	g = TransitionSystem()
	for r in range(rows):
		for c in range(columns):
			node = (r, c)
			for neighbor in neighbors_of(r, c):
				g.add_edge(node, neighbor)
	g.initial.add(initial_state)
	return g



