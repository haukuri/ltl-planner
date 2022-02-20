import random

from collections import defaultdict

from .graph import Graph


class TransitionSystem(Graph):
    def __init__(self):
        super().__init__()
        self.__labels = defaultdict(set)

    def labels(self, node):
        return self.__labels[node]


def rectworld(rows, columns, initial_state=None):
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
    if initial_state:
        g.initial.add(initial_state)
    return g


# taken from Wikipedia https://en.wikipedia.org/wiki/Maze_generation_algorithm
def maze(width=81, height=51, complexity=0.75, density=0.75):
    # Only odd shapes
    height = (height // 2) * 2 + 1
    width = (width // 2) * 2 + 1
    # Adjust complexity and density relative to maze size
    complexity = int(complexity * (5 * (height + width)))
    density = int(density * (height // 2 * width // 2))
    # Build actual maze
    Z = [[False for _ in range(width)] for _ in range(height)]
    # Fill borders
    Z[0, :] = Z[-1, :] = 1
    Z[:, 0] = Z[:, -1] = 1
    # Make isles
    for i in range(density):
        x, y = random.randint(0, width // 2) * 2, random.randint(0, height // 2) * 2
        Z[y, x] = 1
        for j in range(complexity):
            neighbours = []
            if x > 1:
                neighbours.append((y, x - 2))
            if x < width - 2:
                neighbours.append((y, x + 2))
            if y > 1:
                neighbours.append((y - 2, x))
            if y < height - 2:
                neighbours.append((y + 2, x))
            if neighbours:
                y_, x_ = random.choice(neighbours)
                if Z[y_, x_] == 0:
                    Z[y_, x_] = 1
                    Z[y_ + (y - y_) // 2, x_ + (x - x_) // 2] = 1
                    x, y = x_, y_
    return Z


def shrunk_maze(rows, cols):
    z = maze(cols + 2, rows + 2)
    return z[1:-1, 1:-1]


def board_to_ts(board, obstacle_label="wall"):
    num_rows = len(board)
    num_cols = len(board[0])
    ts = rectworld(num_rows, num_cols)
    for row in range(len(board)):
        for col in range(len(board[0])):
            obstacle = board[row][col]
            if obstacle:
                ts.labels((row, col)).add(obstacle_label)
    return ts
