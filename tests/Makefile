NODE=$(shell which node)
UNITS=$(wildcard *.js)

all: test
	@echo Done
clean:
	@echo Done

test:
	@for unit in ${UNITS}; do \
	 echo ""; \
	 echo [38m[1mRunning $$unit: [0m ; \
	 (${NODE} $$unit);\
	done
