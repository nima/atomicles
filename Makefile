MAKEFLAGS := --no-print-directory
INSTALL_DIR := /opt/nima/atomicles
INSTALL := atomicles daemonize lock
PWD := $(shell pwd)

build:
	@$(MAKE) -C src all

purge clean:
	@$(MAKE) -C src $@

install: build $(INSTALL:%=/opt/bin/%)
/opt/bin/%: src/%
	ln -s $(PWD)/$< $@

uninstall:
	@rm -f $(INSTALL:%=/opt/bin/%)

.PHONY: build clean test install uninstall
