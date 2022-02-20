from pathlib import Path
from subprocess import check_output


def run_ltl2ba(ltl_formula: str) -> str:
    package_root = Path(__file__).parent
    ltl2ba_binary = package_root.parent / "ltl2ba" / "ltl2ba"
    command = [ltl2ba_binary, "-f", ltl_formula]
    ltl2ba_output = check_output(command, encoding="utf-8")
    return ltl2ba_output
