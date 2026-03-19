from ltlplanner.visualization import DotWriter
from ltlplanner.graph import Graph
from ltlplanner.buchi import Buchi
from ltlplanner.booleans.parser import parse as parse_expr


def test_to_string_produces_digraph_wrapper():
    w = DotWriter()
    w.add_node("A")
    output = w.to_string()
    assert output.startswith("digraph")
    assert output.strip().endswith("}")


def test_to_string_empty_dotwriter():
    w = DotWriter()
    output = w.to_string()
    assert "digraph" in output
    assert "{" in output
    assert "}" in output


def test_custom_graph_name_appears_in_output():
    w = DotWriter(name="my_automaton")
    output = w.to_string()
    assert "my_automaton" in output


def test_node_shape_appears_in_output():
    w = DotWriter()
    w.add_node("A", shape="doublecircle")
    output = w.to_string()
    assert "doublecircle" in output


def test_edge_label_appears_in_output():
    w = DotWriter()
    w.add_node("A")
    w.add_node("B")
    w.add_edge("A", "B", label="my_label")
    output = w.to_string()
    assert 'label="my_label"' in output


def test_read_graph_initial_state_gets_doublecircle():
    g = Graph()
    g.add_edge("s0", "s1")
    g.initial.add("s0")
    w = DotWriter()
    w.read_graph(g)
    output = w.to_string()
    assert "doublecircle" in output


def test_read_graph_accept_state_gets_octagon():
    g = Graph()
    g.add_edge("s0", "s1")
    g.accept.add("s1")
    w = DotWriter()
    w.read_graph(g)
    output = w.to_string()
    assert "octagon" in output


def test_read_graph_plain_state_gets_circle():
    g = Graph()
    g.add_edge("s0", "s1")
    # neither initial nor accept
    w = DotWriter()
    w.read_graph(g)
    output = w.to_string()
    assert "circle" in output


def test_read_graph_buchi_includes_guard_label():
    b = Buchi()
    b.add_edge("b0", "b1", guard=parse_expr("a && b"))
    w = DotWriter()
    w.read_graph(b)
    output = w.to_string()
    assert "a && b" in output


def test_read_graph_all_edges_present():
    g = Graph()
    g.add_edge("x", "y")
    g.add_edge("y", "z")
    w = DotWriter()
    w.read_graph(g)
    output = w.to_string()
    assert "->" in output
    # Both edges should produce arrow lines
    assert output.count("->") == 2
