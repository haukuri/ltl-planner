PYTHON = ./env/bin/python

dependencies:
	$(MAKE) -C ltl2ba

clean: pyclean envclean
	$(MAKE) -C ltl2ba clean

test: dependencies env
	$(PYTHON) -m pytest

lint: env
	$(PYTHON) -m flake8 ltlplanner

pyclean:
	rm -r ltlplanner/**/__pycache__

envclean:
	rm -r env

dependency_diagram: env
	./env/bin/pydeps -o output/dependency_graph.png -T png ltlplanner

env:
	python3.10 -m venv env --prompt "ltlplanner"
	$(PYTHON) -m pip install --upgrade pip
	$(PYTHON) -m pip install -e .
	$(PYTHON) -m pip install -r requirements.txt

default: dependencies
