include Makefile.sets

# VERSION 1.0.3 was the first working! :D
# VERSION 1.0.4 handled fixed strings too
# VERSION 1.0.5 found item descriptions
# VERSION 1.0.6 compressed better
# VERSION 1.0.7 compressed more carefully
# VERSION 1.0.8 documented the script
# VERSION 1.0.9 fixed "..." handling and located the font
# VERSION 1.0.10 had knowledge of character sets
# VERSION 1.0.11 had a working font insertor
# VERSION 1.0.12 had better knowledge of special codes
# VERSION 1.0.13 used 62-base numbers
# VERSION 1.0.14 added taipus.cc
# VERSION 1.0.15 updated FIN/README and ct_fin.txt, but neither are archived
# VERSION 1.0.16 added taipus.rb, fixed homepage urls and fixed mmap error checking.
# VERSION 1.0.17 working again; uses space better; little modularized
# VERSION 1.0.18 more of above
# VERSION 1.0.19 code organising... improved 'i' in 8x8 font.
# VERSION 1.0.20 binpacker changes, some translation done too
# VERSION 1.0.21 more translation, some documentation, font palette changes.
# VERSION 1.0.22 more translation, autowrapping support, conjugation detection code
# VERSION 1.1.0  did some assembly hacking, support for code patching

VERSION=1.1.0
ARCHFILES=xray.c xray.h \
          viewer.c \
          ctcset.cc ctcset.hh \
          miscfun.cc miscfun.hh \
          space.cc space.hh \
          wstring.cc wstring.hh \
          readin.cc rom.hh \
          fonts.cc fonts.hh \
          conjugate.cc conjugate.hh \
          symbols.cc symbols.hh \
          tgaimage.cc tgaimage.hh \
          ctdump.cc ctinsert.cc \
          ctinsert.hh writeout.cc \
          makeips.cc unmakeips.cc \
          taipus.rb progdesc.php \
          spacefind.cc base62.cc sramdump.cc \
          binpacker.tcc binpacker.hh \
          README transnotes.txt
EXTRA_ARCHFILES=\
          ct_eng.txt \
          ct8fnFI.tga ct16fnFI.tga

ARCHNAME=chronotools-$(VERSION)
ARCHDIR=archives/

PROGS=xray viewer ctdump ctinsert makeips unmakeips spacefind

all: $(PROGS)

xray: xray.o
	$(CC) -o $@ $^ $(LDFLAGS) -lggi

viewer: viewer.o
	$(CC) -o $@ $^ $(LDFLAGS) -lslang

ctdump: ctdump.o ctcset.o miscfun.o wstring.o
	$(CXX) -o $@ $^

ctinsert: \
		ctinsert.o miscfun.o readin.o \
		tgaimage.o space.o writeout.o \
		dictionary.o fonts.o rom.o \
		conjugate.o symbols.o ctcset.o wstring.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lm

spacefind: spacefind.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lm

makeips: makeips.cc
	$(CXX) -o $@ $^
unmakeips: unmakeips.cc
	$(CXX) -g -O -Wall -W -pedantic -o $@ $^

sramdump: sramdump.cc wstring.o
	$(CXX) -g -O -Wall -W -pedantic -o $@ $^
base62: base62.cc
	$(CXX) -g -O -Wall -W -pedantic -o $@ $^

ct_eng.txt: ctdump chrono-dumpee.smc
	./ctdump >ct_eng.txt
ctpatch-hdr.ips ctpatch-nohdr.ips: \
		ctinsert ct_eng.txt \
		ct16fnFI.tga ct8fnFI.tga
	./ctinsert
chrono-patched.smc: unmakeips ctpatch-hdr.ips chrono-dumpee.smc
	./unmakeips ctpatch-hdr.ips <chrono-dumpee.smc >chrono-patched.smc

snes9xtest: chrono-patched.smc FORCE
	#~/src/snes9x/bisq-1.39/Gsnes9x -stereo -alt -m 256x256[C32/32] -r 7 chrono-patched.smc
	~/snes9x -stereo -alt -y 4 -r 7 chrono-patched.smc

snes9xtest2: chrono-patched.smc FORCE
	~/snes9x -r 0 chrono-patched.smc

clean: FORCE
	rm -f *.o $(PROGS)
distclean: clean
	rm -f *~
realclean: distclean
	rm -f ct_eng.txt ctpatch-hhr.ips ctpatch-nohdr.ips chrono-patched.smc

include depfun.mak

.PHONY: all clean distclean realclean snes9xtest
FORCE: ;
