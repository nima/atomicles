INSTALL_DIR := /opt/nima/atomicles
INSTALL := atomicles daemonize lock
PWD := $(shell pwd)

build:
	@$(MAKE) -C src all

clean:
	@$(MAKE) -C src clean

#.PHONY: $(INSTALL:%=test-%)
test: $(INSTALL:%=test-%)
	echo $^;
test-%: src/%
	@echo $@ $^;

install: build $(INSTALL:%=/opt/bin/%)
/opt/bin/%: src/%
	ln -s $(PWD)/$< $@

uninstall:
	@rm -f $(INSTALL:%=/opt/bin/%)
