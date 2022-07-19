PYTHON = ./env/bin/python

dependencies:
	$(MAKE) -C ltl2ba

clean: pyclean envclean
	$(MAKE) -C ltl2ba clean

test: dependencies env
	$(PYTHON) -m pytest

lint: env
	$(PYTHON) -m flake8 ltlplanner --count --select=E9,F63,F7,F82 --show-source --statistics
	$(PYTHON) -m flake8 ltlplanner --count --exit-zero --max-complexity=10 --max-line-length=127 --statistics

pyclean:
	rm -r ltlplanner/**/__pycache__

envclean:
	rm -r env

dependency_diagram: env
	./env/bin/pydeps -o output/dependency_graph.png -T png ltlplanner

env: requirements.txt
	rm -r env || true
	python3.10 -m venv env --prompt "ltlplanner"
	$(PYTHON) -m pip install --upgrade pip
	$(PYTHON) -m pip install -e .
	$(PYTHON) -m pip install -r requirements.txt

default: dependencies
