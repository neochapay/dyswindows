include $(top_srcdir)/build-misc/common.mk

SUBDIRS = build-misc Y libYc++ config modules clients data doc libYconfig

noinst_SCRIPTS = startY.devel

ACLOCAL_AMFLAGS = -I autoconf

EXTRA_DIST = autogen.sh autoconf/test.Versions

MAINTAINERCLEANFILES = aclocal.m4 configure

doc/doxygen-Y.cfg: doc/doxygen-Y.cfg.in
	make -C doc doxygen-Y.cfg

doc/doxygen-libYc++.cfg: doc/doxygen-libYc++.cfg.in
	make -C doc doxygen-libYc++.cfg

doxygen-Y: doc/doxygen-Y.cfg
	rm -rf doc/Y
	doxygen doc/doxygen-Y.cfg

doxygen-libYc++: doc/doxygen-libYc++.cfg
	rm -rf doc/libYc++
	doxygen doc/doxygen-libYc++.cfg

doxygen: doxygen-Y doxygen-libYc++

doc: doxygen

.PHONY: doc doxygen doxygen-Y doxygen-libYc++
