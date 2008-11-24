## These are the rules used for building every module

AM_LDFLAGS = -avoid-version -Wl,-Bsymbolic -Wl,--version-script=$(top_srcdir)/modules/Versions $(CAIRO_LIBS)

INCLUDES += $(CAIRO_CPPFLAGS)
