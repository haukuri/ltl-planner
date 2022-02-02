PYTHON = ./env/bin/python

dependencies:
	$(MAKE) -C ltl2ba

clean:
	$(MAKE) -C ltl2ba clean

test:
	$(PYTHON) -m pytest

default: dependencies
