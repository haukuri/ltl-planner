"""Tests for the CFFI-based ltl2ba binding."""

import pytest
from ltlplanner.ltl2ba_cffi import translate, Ltl2baError
from ltlplanner.ltl2ba_wrapper import run_ltl2ba_cffi
from ltlplanner.buchi import Buchi


class TestTranslate:
    def test_simple_formula(self):
        result = translate("[]<>a")
        assert "states" in result
        assert "edges" in result
        assert len(result["states"]) > 0
        assert len(result["edges"]) > 0

    def test_states_have_required_keys(self):
        result = translate("[]<>a")
        for state in result["states"]:
            assert "name" in state
            assert "is_initial" in state
            assert "is_accept" in state

    def test_edges_have_required_keys(self):
        result = translate("[]<>a")
        for edge in result["edges"]:
            assert "src" in edge
            assert "dst" in edge
            assert "guard" in edge

    def test_has_initial_state(self):
        result = translate("[]<>a")
        initial_states = [s for s in result["states"] if s["is_initial"]]
        assert len(initial_states) >= 1

    def test_has_accept_state(self):
        result = translate("[]<>a")
        accept_states = [s for s in result["states"] if s["is_accept"]]
        assert len(accept_states) >= 1

    def test_conjunction_formula(self):
        result = translate("[]<>a && []<>b")
        state_names = {s["name"] for s in result["states"]}
        edge_srcs = {e["src"] for e in result["edges"]}
        edge_dsts = {e["dst"] for e in result["edges"]}
        # All edge endpoints should reference valid states
        assert edge_srcs <= state_names
        assert edge_dsts <= state_names

    def test_eventually_formula(self):
        result = translate("<>a")
        assert len(result["states"]) > 0

    def test_invalid_formula_raises(self):
        with pytest.raises(Ltl2baError):
            translate("[] &&& !!!")

    def test_guards_are_strings(self):
        result = translate("[]<>a && []<>b")
        for edge in result["edges"]:
            assert isinstance(edge["guard"], str)
            assert len(edge["guard"]) > 0


class TestRunLtl2baCffi:
    def test_returns_dict(self):
        result = run_ltl2ba_cffi("[]<>a")
        assert isinstance(result, dict)
        assert "states" in result
        assert "edges" in result


class TestBuchiFromCffi:
    def test_basic_construction(self):
        buchi = Buchi.from_cffi("[]<>a && []<>b")
        assert len(buchi.initial) >= 1
        assert len(buchi.accept) >= 1

    def test_guard_evaluation(self):
        buchi = Buchi.from_cffi("[]<>a && []<>b")
        guard = buchi.guard("T0_init", "accept_S1")
        assert guard.check(["a", "b"]) is True
        assert guard.check(["a"]) is False
        assert guard.check(["b"]) is False

    def test_matches_from_ltl(self):
        """CFFI path should produce the same automaton as from_ltl."""
        buchi_cffi = Buchi.from_cffi("[]<>a && []<>b")
        buchi_ltl = Buchi.from_ltl("[]<>a && []<>b")
        assert buchi_cffi.initial == buchi_ltl.initial
        assert buchi_cffi.accept == buchi_ltl.accept
        assert buchi_cffi.nodes() == buchi_ltl.nodes()

    def test_edges_have_guards(self):
        buchi = Buchi.from_cffi("[]<>a")
        for src in buchi.nodes():
            for dst in buchi.post(src):
                guard = buchi.guard(src, dst)
                assert guard is not None

    def test_eventually_c(self):
        """Test <> c produces a valid automaton."""
        buchi = Buchi.from_cffi("<>c")
        assert len(buchi.initial) >= 1
        # Should have at least one accepting state
        assert len(buchi.accept) >= 1

    def test_complex_formula(self):
        buchi = Buchi.from_cffi("[]<>(a && b) && <>c")
        assert len(buchi.initial) >= 1
        assert len(buchi.accept) >= 1
        assert len(buchi.nodes()) >= 2
