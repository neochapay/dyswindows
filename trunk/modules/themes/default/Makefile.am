include $(top_srcdir)/build-misc/common.mk
include $(top_srcdir)/modules/module.mk

themelibdir = ${pkglibdir}/theme
themelib_LTLIBRARIES = dustinTheme.la

noinst_HEADERS = window.h draw.h font.h

dustinTheme_la_SOURCES = default.c window.c draw.c font.c
dustinTheme_la_LDFLAGS = -module
# dustinTheme_la_LIBADD  = $(CAIRO_LIBS)

# INCLUDES += $(CAIRO_CPPFLAGS)
