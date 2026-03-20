from pathlib import Path
from subprocess import check_output


def run_ltl2ba(ltl_formula: str) -> str:
    package_root = Path(__file__).parent
    ltl2ba_binary = package_root.parent / "ltl2ba" / "ltl2ba"
    command = [ltl2ba_binary, "-f", ltl_formula]
    ltl2ba_output = check_output(command, encoding="utf-8")
    return ltl2ba_output


def run_ltl2ba_cffi(ltl_formula: str) -> dict:
    """Run ltl2ba via CFFI and return structured result.

    Returns a dict with keys:
        states: list of dicts with 'name', 'is_initial', 'is_accept'
        edges: list of dicts with 'src', 'dst', 'guard'
    """
    from .ltl2ba_cffi import translate

    return translate(ltl_formula)
