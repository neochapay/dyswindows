include $(top_srcdir)/build-misc/common.mk
include $(top_srcdir)/modules/module.mk

themelibdir = ${pkglibdir}/theme
themelib_LTLIBRARIES = eauTheme.la

noinst_HEADERS = windowDeco.h yLabel.h yButton.h yCheckBox.h yRadioButton.h font.h

eauTheme_la_SOURCES = eau.c windowDeco.c yLabel.c yButton.c yCheckBox.c yRadioButton.c font.c

eauTheme_la_LDFLAGS = -module

#eauTheme_la_LIBADD  = $(CAIRO_LIBS)

# INCLUDES += $(CAIRO_CPPFLAGS)
