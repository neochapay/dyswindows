include $(top_srcdir)/build-misc/common.mk
include $(top_srcdir)/modules/module.mk

videolibdir = ${pkglibdir}/driver/video
videolib_LTLIBRARIES = xlib.la

xlib_la_SOURCES = xlib.c
xlib_la_LDFLAGS = -module
xlib_la_LIBADD = $(CAIRO_LIBS)

INCLUDES += $(CAIRO_CPPFLAGS)
