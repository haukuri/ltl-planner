from ltlplanner import ltl


def test_run_ltl2ba():
    formula = '[]<>(a && b) && <> c'
    output = ltl._run_ltl2ba(formula)
    assert output.strip() == """never { /* []<>(a && b) && <> c */
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