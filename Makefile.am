SUBDIRS = include src man

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xcwm.pc

MAINTAINERCLEANFILES = ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL

.PHONY: ChangeLog INSTALL

