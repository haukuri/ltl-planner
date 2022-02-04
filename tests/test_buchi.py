from ltlplanner.buchi import Buchi, from_ltl
from ltlplanner.promela import parse as parse_promela

sample_promela = """
never { /* []<>a && []<>b */
T0_init:
    if
    :: (1) -> goto T0_init
    :: (a) -> goto T1_S1
    :: (a && b) -> goto accept_S1
    fi;
T1_S1:
    if
    :: (1) -> goto T1_S1
    :: (b) -> goto accept_S1
    fi;
accept_S1:
    if
    :: (1) -> goto T0_init
    :: (a) -> goto T1_S1
    :: (a && b) -> goto accept_S1
    fi;
}
"""

def test_buchi_from_ltl():
    ltl = '[]<>a && []<>b '
    buchi_graph = from_ltl(ltl)
    actual_guard = buchi_graph.guard("T0_init", "accept_S1")
    assert actual_guard.check(["a", "b"]) == True
    assert actual_guard.check(["a"]) == False
    assert actual_guard.check(["b"]) == False

def test_buchi_from_promela():
    promela_nda = parse_promela(sample_promela)
    buchi = Buchi(promela_nda)
    assert buchi.post('T0_init') == {'T0_init', 'T1_S1', 'accept_S1'}
    assert buchi.pre('T0_init') == {'T0_init', 'accept_S1'}
    assert buchi.guard('T0_init', 'accept_S1').check(['a', 'b'])
    assert not buchi.guard('T0_init', 'accept_S1').check(['a'])
    assert not buchi.guard('T0_init', 'accept_S1').check(['b'])


