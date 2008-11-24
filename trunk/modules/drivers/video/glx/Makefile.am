include $(top_srcdir)/build-misc/common.mk
include $(top_srcdir)/modules/module.mk

if WANT_GLITZ

videolibdir = ${pkglibdir}/driver/video
videolib_LTLIBRARIES = glx.la

glx_la_SOURCES = glx.c
glx_la_LDFLAGS = -module
glx_la_LIBADD = $(CAIRO_LIBS) $(GLITZ_GLX_LIBS)

INCLUDES += $(CAIRO_CPPFLAGS) $(GLITZ_GLX_CFLAGS)

endif
