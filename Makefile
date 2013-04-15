GYP=$(shell which node-gyp)
NODE=$(shell which node)
NODEVERSION=$(shell $(NODE) --version)
OSNAME=$(shell uname | sed y/ABCDEFGHIJKLMNOPQRSTUVXY/abcdefghijklmnopqrstuvxy/)
OSARCH=$(shell uname -m)

all: build

build: config
	@$(GYP) $(GYPFLAGS) --verbose build	

config: $(GYP)
	@$(GYP) $(GYPFLAGS) configure

clean:
	@$(GYP) $(GYPFLAGS) clean

distclean: clean
	@rm -rf build node_modules

test: build
	@$(MAKE) -C tests
