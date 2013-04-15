NODE=$(shell which node)
UNITS=$(wildcard *.js)

all: tests
	@echo Done
clean:
	@echo Done

tests:
	@for unit in ${UNITS}; do \
	 echo ""; \
	 echo [38m[1mRunning $$unit: [0m ; \
	 (${NODE} $$unit);\
	done
