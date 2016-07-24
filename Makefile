#Makefile for MacBot
MAKE = make

all:
	$(MAKE) -C src

sim:
	$(MAKE) -C src sim

clean:
	$(MAKE) -C src clean
