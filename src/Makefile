#Makefile for MacBot
MAKE = make

all:
	$(MAKE) -C CORE
	$(MAKE) -C client
	$(MAKE) -C drivers

sim:
	$(MAKE) -C CORE
	$(MAKE) -C client
	$(MAKE) -C drivers sim

clean:
	$(MAKE) -C CORE clean
	$(MAKE) -C client clean
	$(MAKE) -C drivers clean

