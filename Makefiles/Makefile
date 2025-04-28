# Main Makefile to build everything

all: app module

app:
	$(MAKE) -f Makefile.chat_app all

module:
	$(MAKE) -f Makefile.char_device all

clean:
	$(MAKE) -f Makefile.chat_app clean
	$(MAKE) -f Makefile.char_device clean

install: install-app install-module

install-app:
	$(MAKE) -f Makefile.chat_app install

install-module:
	$(MAKE) -f Makefile.char_device install

uninstall: uninstall-app uninstall-module

uninstall-app:
	$(MAKE) -f Makefile.chat_app uninstall

uninstall-module:
	$(MAKE) -f Makefile.char_device uninstall

.PHONY: all app module clean install install-app install-module uninstall uninstall-app uninstall-module
