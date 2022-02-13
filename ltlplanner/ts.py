from .graph import ConcreteGraph


def rectworld(rows, columns):
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
	g = ConcreteGraph()
	for r in range(rows):
		for c in range(columns):
			node = (r, c)
			for neighbor in neighbors_of(r, c):
				g.add_edge(node, neighbor)
	return g



