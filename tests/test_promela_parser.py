from ltlplanner import promela

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


def test_promela_parser():
    parsed = promela.parse(sample_promela)
    expected = promela.PromelaOutput(
        edges={
            ("T0_init", "T0_init"): "(1)",
            ("T0_init", "T1_S1"): "(a)",
            ("T0_init", "accept_S1"): "(a && b)",
            ("T1_S1", "T1_S1"): "(1)",
            ("T1_S1", "accept_S1"): "(b)",
            ("accept_S1", "T0_init"): "(1)",
            ("accept_S1", "T1_S1"): "(a)",
            ("accept_S1", "accept_S1"): "(a && b)",
        },
        states={"T0_init", "T1_S1", "accept_S1"},
        initial_states = {'T0_init'},
        accept_states={"accept_S1"}
    )
    assert parsed == expected
