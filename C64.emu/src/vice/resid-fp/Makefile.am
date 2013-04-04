## Process this file with automake to create Makefile.in

AR = @AR@

noinst_LIBRARIES = libresidfp.a

libresidfp_a_SOURCES = sid.cc voice.cc wave.cc envelope.cc filter.cc extfilt.cc pot.cc version.cc convolve.cc

noinst_HEADERS = sid.h voice.h wave.h envelope.h filter.h extfilt.h pot.h residfp-config.h

EXTRA_DIST = $(noinst_HEADERS) README.VICE convolve-sse.cc

SUFFIXES = .dat

if USE_SSE
convolve-sse.o: convolve-sse.cc
	$(CXXCOMPILE) -msse -c -o $@ `test -f '$<' || echo '$(srcdir)/'`$<

libresidfp_a_LIBADD = convolve-sse.o
endif
