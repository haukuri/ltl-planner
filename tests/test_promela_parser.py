from ltlplanner import promela

sample_promela = """never { /* []<>(a && b) && <> c */
T0_init:
    if
    :: (1) -> goto T0_init
    :: (c) -> goto T0_S4
    fi;
T0_S4:
    if
    :: (1) -> goto T0_S4
    :: (a && b) -> goto accept_S4
    fi;
accept_S4:
    if
    :: (1) -> goto T0_S4
    :: (a && b) -> goto accept_S4
    fi;
}"""


def test_basic_parsing():
    actual = promela.parse(sample_promela)
    expected = promela.PromelaOutput(
        edges={
            ("T0_init", "T0_init"): "(1)",
            ("T0_init", "T0_S4"): "(c)",
            ("T0_S4", "T0_S4"): "(1)",
            ("T0_S4", "accept_S4"): "(a && b)",
            ("accept_S4", "T0_S4"): "(1)",
            ("accept_S4", "accept_S4"): "(a && b)",
        },
        states={"T0_init", "T0_S4", "accept_S4"},
        initial_states={"T0_init"},
        accept_states={"accept_S4"}
    )
    assert actual == expected
