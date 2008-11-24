include $(top_srcdir)/build-misc/common.mk

keymapdir = $(sysconfdir)/Y/keymaps
ykbcompdir = $(pkgdatadir)/ykbcomp

CLEANFILES += ykbcomp YKB/YKBGrammar.pm

keymaps = gb us test jp106
extra_ykb_files = basic.ykb pc.ykb qwerty.ykb iso9995.ykb deadkeys.ykb

ykb_files :=  $(foreach keymap,$(keymaps),$(keymap).ykb) $(extra_ykb_files)
ykm_files :=  $(foreach keymap,$(keymaps),$(keymap).ykm)
Pykm_files := $(foreach keymap,$(keymaps),.deps/$(keymap).Pykm)

CLEANFILES += $(ykm_files) $(Pykm_files)

dist_keymap_DATA = us.Ykeymap gb.Ykeymap $(ykb_files)
nodist_keymap_DATA = $(ykm_files)
nobase_dist_ykbcomp_DATA = YKB/YKB.pm YKB/YKM.pm
nobase_nodist_ykbcomp_DATA = YKB/YKBGrammar.pm
nodist_bin_SCRIPTS = ykbcomp
EXTRA_DIST = YKB/YKBGrammar ykbcomp.in

do_subst = sed \
  -e 's,[@]pkgdatadir[@],$(pkgdatadir),g'

ykbcomp: $(srcdir)/ykbcomp.in Makefile
	rm -f $@ $@.tmp
	$(do_subst) $< >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv -f $@.tmp $@

YKB/YKBGrammar.pm: $(srcdir)/YKB/YKBGrammar Makefile
	mkdir -p YKB
	perl -MParse::RecDescent - $< YKB::YKBGrammar
	mv -f YKBGrammar.pm YKB/YKBGrammar.pm

%.ykm: %.ykb ykbcomp $(srcdir)/YKB/YKB.pm $(srcdir)/YKB/YKM.pm Makefile ykbcomp YKB/YKBGrammar.pm
	@$(mkdir_p) .deps
	ykbcomp_libdir=".:$(srcdir)" ./ykbcomp -M .deps/$*.Pykm -o $@ $<

-include $(Pykm_files)
