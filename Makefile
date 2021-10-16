dependencies:
	$(MAKE) -C ltl2ba

clean:
	$(MAKE) -C ltl2ba clean

default: dependencies
