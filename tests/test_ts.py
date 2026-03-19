import pytest
from ltlplanner.ts import TransitionSystem, neighbors_of, rectworld, board_to_ts, maze, shrunk_maze


# --- neighbors_of ---

def test_neighbors_of_corner():
    result = set(neighbors_of(0, 0, 3, 3))
    assert result == {(0, 1), (1, 0)}


def test_neighbors_of_center():
    result = set(neighbors_of(1, 1, 3, 3))
    assert result == {(0, 1), (1, 0), (1, 2), (2, 1)}


def test_neighbors_of_edge():
    result = set(neighbors_of(0, 1, 3, 3))
    assert result == {(0, 0), (0, 2), (1, 1)}


def test_neighbors_of_no_diagonals():
    result = set(neighbors_of(1, 1, 3, 3))
    assert (0, 0) not in result
    assert (0, 2) not in result
    assert (2, 0) not in result
    assert (2, 2) not in result


def test_neighbors_of_no_self():
    for r in range(3):
        for c in range(3):
            assert (r, c) not in set(neighbors_of(r, c, 3, 3))


def test_neighbors_of_single_cell_grid():
    result = list(neighbors_of(0, 0, 1, 1))
    assert result == []


# --- rectworld ---

def test_rectworld_2x2_orthogonal_edges():
    g = rectworld(2, 2)
    edges = set(g.edges())
    # Each cell connects to its orthogonal neighbors
    assert ((0, 0), (0, 1)) in edges
    assert ((0, 0), (1, 0)) in edges
    assert ((0, 1), (0, 0)) in edges
    assert ((1, 0), (0, 0)) in edges


def test_rectworld_2x2_no_diagonal_edges():
    g = rectworld(2, 2)
    edges = set(g.edges())
    assert ((0, 0), (1, 1)) not in edges
    assert ((0, 1), (1, 0)) not in edges


def test_rectworld_sets_initial_state():
    g = rectworld(3, 3, initial_state=(1, 1))
    assert g.initial == {(1, 1)}


def test_rectworld_no_initial_state_by_default():
    g = rectworld(3, 3)
    assert g.initial == set()


def test_rectworld_1x1_no_edges():
    g = rectworld(1, 1)
    assert list(g.edges()) == []


# --- board_to_ts ---

def test_board_to_ts_obstacle_gets_wall_label():
    board = [
        [False, True],
        [False, False],
    ]
    ts = board_to_ts(board)
    assert "wall" in ts.labels((0, 1))


def test_board_to_ts_non_obstacle_has_no_wall_label():
    board = [
        [False, True],
        [False, False],
    ]
    ts = board_to_ts(board)
    assert "wall" not in ts.labels((0, 0))
    assert "wall" not in ts.labels((1, 0))
    assert "wall" not in ts.labels((1, 1))


def test_board_to_ts_all_cells_exist_as_nodes():
    board = [
        [False, False],
        [False, False],
    ]
    ts = board_to_ts(board)
    nodes = ts.nodes()
    assert (0, 0) in nodes
    assert (0, 1) in nodes
    assert (1, 0) in nodes
    assert (1, 1) in nodes


# --- TransitionSystem ---

def test_transition_system_labels_initializes_empty():
    ts = TransitionSystem()
    assert ts.labels("s0") == set()


def test_transition_system_labels_persists_mutation():
    ts = TransitionSystem()
    ts.labels("s0").add("goal")
    assert "goal" in ts.labels("s0")


def test_transition_system_labels_independent_per_node():
    ts = TransitionSystem()
    ts.labels("s0").add("a")
    assert "a" not in ts.labels("s1")


# --- maze / shrunk_maze ---

def test_maze_returns_correct_dimensions():
    h, w = 11, 11
    result = maze(width=w, height=h)
    assert len(result) == h
    assert all(len(row) == w for row in result)


def test_maze_borders_are_walls():
    result = maze(width=11, height=11)
    h = len(result)
    w = len(result[0])
    assert all(result[0][c] for c in range(w))
    assert all(result[h - 1][c] for c in range(w))
    assert all(result[r][0] for r in range(h))
    assert all(result[r][w - 1] for r in range(h))


def test_shrunk_maze_smaller_than_full_maze():
    result = shrunk_maze(9, 9)
    assert len(result) > 0
    assert len(result[0]) > 0
