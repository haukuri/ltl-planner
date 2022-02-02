PYTHON = ./env/bin/python

dependencies:
	$(MAKE) -C ltl2ba

clean: pyclean
	$(MAKE) -C ltl2ba clean

test:
	$(PYTHON) -m pytest

pyclean:
	rm -r ltlplanner/**/__pycache__

default: dependencies
