PYTHON = ./env/bin/python
COVERAGE = ./env/bin/coverage

dependencies:
	$(MAKE) -C ltl2ba

clean: pyclean envclean
	$(MAKE) -C ltl2ba clean

test: dependencies
	uv run coverage run -m pytest

coverage: test
	uv run coverage report --omit=tests/*
	uv run coverage html --omit=tests/* --directory=output/coverage/

lint:
	uv run ruff check

pyclean:
	rm -r ltlplanner/**/__pycache__

dependency_diagram:
	uv run pydeps -o output/dependency_graph.png -T png ltlplanner

default: dependencies
