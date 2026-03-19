from ltlplanner.graph import BidirectionalEdgeMap, Graph


# --- BidirectionalEdgeMap ---

def test_bidirectional_add_populates_post_and_pre():
    m = BidirectionalEdgeMap()
    m.add("a", "b")
    assert "b" in m.post("a")
    assert "a" in m.pre("b")


def test_bidirectional_add_does_not_cross_pollinate():
    m = BidirectionalEdgeMap()
    m.add("a", "b")
    assert "a" not in m.post("b")
    assert "b" not in m.pre("a")


def test_bidirectional_remove_cleans_both_directions():
    m = BidirectionalEdgeMap()
    m.add("a", "b")
    m.remove("a", "b")
    assert "b" not in m.post("a")
    assert "a" not in m.pre("b")


def test_bidirectional_edges_yields_all_added_edges():
    m = BidirectionalEdgeMap()
    m.add("a", "b")
    m.add("a", "c")
    m.add("b", "c")
    edges = set(m.edges())
    assert edges == {("a", "b"), ("a", "c"), ("b", "c")}


def test_bidirectional_edges_empty():
    m = BidirectionalEdgeMap()
    assert list(m.edges()) == []


# --- Graph ---

def test_graph_add_edge_creates_post_and_pre():
    g = Graph()
    g.add_edge("x", "y")
    assert "y" in g.post("x")
    assert "x" in g.pre("y")


def test_graph_add_edge_stores_attributes():
    g = Graph()
    g.add_edge("x", "y", weight=5, color="red")
    assert g.edge_attrs[("x", "y")]["weight"] == 5
    assert g.edge_attrs[("x", "y")]["color"] == "red"


def test_graph_add_edge_without_attributes():
    g = Graph()
    g.add_edge("x", "y")
    assert g.edge_attrs[("x", "y")] == {}


def test_graph_nodes_returns_all_endpoints():
    g = Graph()
    g.add_edge("a", "b")
    g.add_edge("b", "c")
    assert g.nodes() == {"a", "b", "c"}


def test_graph_nodes_empty_graph():
    g = Graph()
    assert g.nodes() == set()


def test_graph_initial_and_accept_are_independent():
    g = Graph()
    g.add_edge("s0", "s1")
    g.initial.add("s0")
    g.accept.add("s1")
    assert g.initial == {"s0"}
    assert g.accept == {"s1"}
    assert "s1" not in g.initial
    assert "s0" not in g.accept


def test_graph_edges_yields_all_added():
    g = Graph()
    g.add_edge("p", "q")
    g.add_edge("q", "r")
    assert set(g.edges()) == {("p", "q"), ("q", "r")}


def test_graph_post_unknown_node_returns_empty():
    g = Graph()
    assert g.post("nonexistent") == set()


def test_graph_pre_unknown_node_returns_empty():
    g = Graph()
    assert g.pre("nonexistent") == set()
