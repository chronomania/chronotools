include Makefile.sets

#CXX=i586-mingw32msvc-g++
#CC=i586-mingw32msvc-gcc
#CPP=i586-mingw32msvc-gcc
#CFLAGS += -Ilibiconv
#CPPFLAGS += -Ilibiconv
#CXXFLAGS += -Ilibiconv
#LDFLAGS += -Llibiconv -liconv

# VERSION 1.0.3  was the first working! :D
# VERSION 1.0.4  handled fixed strings too
# VERSION 1.0.5  found item descriptions
# VERSION 1.0.6  compressed better
# VERSION 1.0.7  compressed more carefully
# VERSION 1.0.8  documented the script
# VERSION 1.0.9  fixed "..." handling and located the font
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
# VERSION 1.1.1  conjugating conjugating conjugating... work goes on
# VERSION 1.1.2  and so on
# VERSION 1.1.3  and so on... almost working! "case" still doesn't work.
# VERSION 1.1.4  conjugating finally works!
# VERSION 1.1.5  some bugfixes
# VERSION 1.1.6  fixed an allocation bug and optimized the code generator a bit
# VERSION 1.1.7  some translation, more asm changes
# VERSION 1.1.8  syntax changes in the compiler, optimizations
# VERSION 1.1.9  support for font/dictionary size skew
# VERSION 1.1.10 new configuration system. Time to squash bugs.
# VERSION 1.1.11 configuration works, font-enhancement works.
# VERSION 1.2.0  variable-width 8pix font has stepped in, but has many many bugs.
# VERSION 1.2.1  more vwf stuff, backup before doing big changes
# VERSION 1.2.2  vwf stability++, also techniques now vwf. Scrolling bugs.
# VERSION 1.2.3  lots of more translation
# VERSION 1.2.4  8pix system deciphered, more bugs introduced
# VERSION 1.2.5  characterset enlarged by 512, only vwf8 bugs still
# VERSION 1.2.6  using nonstandard hash_map for greatly improved performance
# VERSION 1.2.7  creating another compiler
# VERSION 1.2.8  improved dictionary compression
# VERSION 1.2.9  compiler progress, first windows binaries are working
# VERSION 1.2.10 cursive font support
# VERSION 1.2.11 some translation, compression optimizations
# VERSION 1.3.0  new compression options, font reorganizer

OPTIM=-O3
#OPTIM=-O0
#OPTIM=-O0 -pg -fprofile-arcs
#LDFLAGS += -pg -fprofile-arcs

VERSION=1.3.0
ARCHFILES=xray.c xray.h \
          viewer.c \
          ctcset.cc ctcset.hh \
          miscfun.cc miscfun.hh \
          space.cc space.hh \
          crc32.cc crc32.h \
          wstring.cc wstring.hh \
          readin.cc wrap.cc writeout.cc \
          settings.hh \
          rom.cc rom.hh \
          rommap.cc rommap.hh \
          strload.cc strload.hh \
          fonts.cc fonts.hh \
          logfiles.cc logfiles.hh \
          conjugate.cc conjugate.hh \
          stringoffs.cc stringoffs.hh \
          compiler.cc compiler.hh \
          symbols.cc symbols.hh \
          tgaimage.cc tgaimage.hh \
          ctdump.cc \
          ctinsert.cc ctinsert.hh \
          makeips.cc unmakeips.cc \
          vwf8.cc vwftest.cc \
          config.cc config.hh \
          confparser.cc confparser.hh \
          extras.cc extras.hh \
          typefaces.cc typefaces.hh \
          taipus.rb ct.code \
          progdesc.php \
          spacefind.cc base62.cc sramdump.cc \
          binpacker.tcc binpacker.hh \
          compiler2.cc compiler2-parser.inc \
          facegenerator.cc \
          hash.hh \
          README transnotes.txt Makefile.sets \
          libiconv/iconv.h libiconv/libiconv.a 

EXTRA_ARCHFILES=\
          ct.cfg ct_try.txt ct8fnFI.tga ct16fn.tga ct8fnV.tga \
          FIN/ct.txt FIN/ct16fn.tga FIN/ct8fn.tga FIN/ct8fnV.tga FIN/README

ARCHNAME=chronotools-$(VERSION)
ARCHDIR=archives/

PROGS=\
	ctdump ctinsert \
	makeips unmakeips \
	facegenerator \
	base62 sramdump \
	viewer xray \
	spacefind comp2test vwftest

all: $(PROGS)

xray: xray.o
	$(CC) -o $@ $^ $(LDFLAGS) -lggi

viewer: viewer.o
	$(CC) -o $@ $^ $(LDFLAGS) -lslang

ctdump: \
		ctdump.o rommap.o strload.o extras.o \
		tgaimage.o symbols.o miscfun.o config.o \
		confparser.o ctcset.o wstring.o
	$(CXX) -o $@ $^ $(LDFLAGS)

ctinsert: \
		ctinsert.o miscfun.o readin.o wrap.o \
		tgaimage.o space.o writeout.o stringoffs.o \
		dictionary.o \
		fonts.o typefaces.o extras.o \
		rom.o snescode.o \
		conjugate.o vwf8.o compiler.o symbols.o \
		logfiles.o \
		config.o confparser.o ctcset.o wstring.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lm

makeips: makeips.cc
	$(CXX) -o $@ $^
unmakeips: unmakeips.cc
	$(CXX) -g -O -Wall -W -pedantic -o $@ $^

spacefind: spacefind.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lm

sramdump: sramdump.cc config.o confparser.o wstring.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)
base62: base62.cc
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)
vwftest: \
		vwftest.cc tgaimage.o \
		fonts.o typefaces.o extras.o conjugate.o \
		snescode.o symbols.o space.o logfiles.o compiler.o \
		config.o confparser.o ctcset.o wstring.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)
facegenerator: \
		facegenerator.cc tgaimage.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)

compiler2.o: compiler2.cc
	$(CXX) -g -o $@ -c $< $(CXXFLAGS) -O0 -fno-default-inline
	
comp2test: compiler2.o config.o confparser.o ctcset.o wstring.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)

#ct.txt: ctdump chrono-dumpee.smc
#	./ctdump chrono-dumpee.smc >ct_tmp.txt || rm -f ct_tmp.txt && false
#	mv ct_tmp.txt ct.txt

ctpatch-hdr.ips ctpatch-nohdr.ips: \
		ctinsert \
		ct.txt ct.code ct.cfg \
		ct16fn.tga ct8fn.tga ct8fnFIv.tga
	time ./ctinsert

chrono-patched.smc: unmakeips ctpatch-hdr.ips chrono-uncompressed.smc
	./unmakeips ctpatch-hdr.ips chrono-uncompressed.smc chrono-patched.smc 2>/dev/null

snes9xtest: chrono-patched.smc FORCE
	#~/src/snes9x/bisq-1.39/Gsnes9x -stereo -alt -m 256x256[C32/32] -r 7 chrono-patched.smc
	./snes9x-debug -stereo -alt -y 1 -r 7 chrono-patched.smc

snes9xtest2: chrono-patched.smc FORCE
	./snes9x-debug -r 0 chrono-patched.smc

winzip: ctdump ctinsert
	rm -f ctdump.exe
	ln ctdump ctdump.exe
	i586-mingw32msvc-strip ctdump.exe
	zip -9 $(ARCHNAME)-ctdump-win32.zip ctdump.exe
	rm -f ctdump.exe
	mv -f $(ARCHNAME)-ctdump-win32.zip /WWW/src/arch/

clean: FORCE
	rm -f *.o $(PROGS)
distclean: clean
	rm -f *~
realclean: distclean
	rm -f ct_eng.txt ctpatch-hhr.ips ctpatch-nohdr.ips chrono-patched.smc

include depfun.mak

.PHONY: all clean distclean realclean snes9xtest
FORCE: ;
