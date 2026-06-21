HOLE ?= hole1_node

.PHONY: build flash monitor

build:
	pio run -e $(HOLE)

flash:
	pio run -e $(HOLE) -t upload

monitor:
	pio device monitor
