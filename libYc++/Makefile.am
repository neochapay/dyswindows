include $(top_srcdir)/build-misc/common.mk
include Makefile.ymk

INCLUDES += $(LIBSIGC_CPPFLAGS) -I$(top_srcdir)/libYc++

lib_LTLIBRARIES = libYc++.la

nobase_nodist_include_HEADERS = $(Y_class_headers)

nobase_include_HEADERS = \
Y/c++.h \
Y/c++/exception.h \
Y/c++/connection.h \
Y/c++/class.h \
Y/c++/object.h \
Y/c++/message.h \
Y/c++/reply.h \
Y/c++/value.h \
Y/c++/timer.h \
Y/c++/ticker.h \
Y/c++/widget.h \
Y/c++/window.h \
Y/c++/canvas.h \
Y/c++/label.h \
Y/c++/console.h \
Y/c++/gridlayout.h \
Y/c++/objects/yradiobutton.h \
Y/c++/objects/yradiogroup.h \
Y/c++/objects/ytogglebutton.h \
Y/c++/objects/ybutton.h \
Y/c++/objects/ycheckbox.h \
Y/c++/objects/ybin.h \
Y/c++/objects/ycontainer.h \
Y/c++/objects/yrowlayout.h

noinst_HEADERS = \
Y/c++/thread_support.h

libYc___la_LIBADD  = $(LIBSIGC_LIBS)
nodist_libYc___la_SOURCES = $(Y_class_sources)
libYc___la_SOURCES = \
Y/c++/connection.cc \
Y/c++/connection_io.cc \
Y/c++/connection_objects.cc \
Y/c++/connection_unix.cc \
Y/c++/connection_threads.cc \
Y/c++/connection_nothreads.cc \
Y/c++/thread_support.cc \
Y/c++/class.cc \
Y/c++/object.cc \
Y/c++/message.cc \
Y/c++/reply.cc \
Y/c++/value.cc \
Y/c++/timer.cc \
Y/c++/ticker.cc \
Y/c++/widget.cc \
Y/c++/window.cc \
Y/c++/canvas.cc \
Y/c++/label.cc \
Y/c++/console.cc \
Y/c++/gridlayout.cc \
Y/c++/objects/ybutton.cc \
Y/c++/objects/ytogglebutton.cc \
Y/c++/objects/yradiobutton.cc \
Y/c++/objects/yradiogroup.cc \
Y/c++/objects/ycheckbox.cc \
Y/c++/objects/ybin.cc \
Y/c++/objects/ycontainer.cc \
Y/c++/objects/yrowlayout.cc

libYc___la_LDFLAGS = -Wl,--no-undefined
libYc___la_DEPENDENCIES = Versions
EXTRA_libYc___la_SOURCES = Versions

MAINTAINERCLEANFILES = $(srcdir)/Makefile.ymk
BUILT_SOURCES = $(Y_class_headers)
CLEANFILES += $(Y_class_sources) $(Y_class_headers)
EXTRA_DIST = Makefile.ycl

if CXX_VERSIONED_SYMBOLS
libYc___la_LDFLAGS += -Wl,--version-script=$(srcdir)/Versions -version-info 0:0:0
else
## Cute abuse of -release to break binary compatibility when versioned
## symbols are not available.
libYc___la_LDFLAGS += -release unversioned
endif

$(srcdir)/Makefile.ymk: @MAINTAINER_MODE_TRUE@ $(srcdir)/Makefile.ycl $(top_srcdir)/build-misc/yclmakemake
	$(top_srcdir)/build-misc/yclmakemake -Y -c c++ -o $@ $(srcdir)/Makefile.ycl

yclpp = $(top_builddir)/build-misc/yclpp
yclpp_lib = $(top_srcdir)/build-misc
yh_dir = Y/c++
ycd_dir = $(top_builddir)/Y
