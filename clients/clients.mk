## Every client wants this stuff

INCLUDES += $(LIBSIGC_CPPFLAGS) \
            -I$(top_srcdir)/libY -I$(top_srcdir)/libYc++ \
            -I$(top_builddir)/libYc++

Ycxx_libs = $(top_builddir)/libYc++/libYc++.la $(LIBSIGC_LIBS)
