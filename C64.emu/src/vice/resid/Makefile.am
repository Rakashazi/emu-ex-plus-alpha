## Process this file with automake to create Makefile.in

AR = @AR@

noinst_LIBRARIES = libresid.a

libresid_a_SOURCES = sid.cc voice.cc wave.cc envelope.cc filter.cc dac.cc extfilt.cc pot.cc version.cc

BUILT_SOURCES = $(noinst_DATA:.dat=.h)

noinst_HEADERS = sid.h voice.h wave.h envelope.h filter.h dac.h extfilt.h pot.h spline.h resid-config.h $(noinst_DATA:.dat=.h)

noinst_DATA = wave6581_PST.dat wave6581_PS_.dat wave6581_P_T.dat wave6581__ST.dat wave8580_PST.dat wave8580_PS_.dat wave8580_P_T.dat wave8580__ST.dat

noinst_SCRIPTS = samp2src.pl

EXTRA_DIST = $(noinst_HEADERS) $(noinst_DATA) $(noinst_SCRIPTS) README.VICE

SUFFIXES = .dat

.dat.h:
	$(PERL) $(srcdir)/samp2src.pl $* $< $(srcdir)/$@
