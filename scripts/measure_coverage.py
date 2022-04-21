import sys
from pathlib import Path
from subprocess import PIPE, CalledProcessError, check_output

project_dir = Path(__file__).parent.parent
coverage_exe = project_dir / "env" / "bin" / "coverage"
output_dir = project_dir / "output" / "coverage"


def run_cmd(command):
    try:
        output = check_output(command, stderr=PIPE)
        print(output.decode("utf-8"))
    except CalledProcessError as e:
        print(f"Error calling {command}", file=sys.stderr)
        print(e.stderr.decode("utf-8"), file=sys.stderr)
        print(e.stdout.decode("utf-8"), file=sys.stdout)
        sys.exit(1)


run_cmd([coverage_exe, "run", "--source=ltlplanner", "-m", "pytest"])
run_cmd([coverage_exe, "report"])
run_cmd([coverage_exe, "html", f"--directory={output_dir}"])
