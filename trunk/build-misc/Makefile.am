include $(top_srcdir)/build-misc/common.mk

## Automake kinda sucks at installing perl modules
yclppdir = $(pkgdatadir)/yclpp

nobase_dist_yclpp_DATA = YCL/YCD.pm YCL/Server/C.pm YCL/Client/Cxx.pm YCL/Client/Cxxh.pm
nodist_bin_SCRIPTS = yclpp

do_subst = sed \
  -e 's,[@]pkgdatadir[@],$(pkgdatadir),g'

MAINTAINERCLEANFILES = compile config.guess config.sub depcomp install-sh ltmain.sh missing
CLEANFILES += yclpp
EXTRA_DIST = yclpp.in gentreeinfo

yclpp: $(srcdir)/yclpp.in Makefile
	rm -f $@ $@.tmp
	$(do_subst) $< >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv -f $@.tmp $@
