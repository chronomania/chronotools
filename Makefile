include Makefile.sets

#CXX=i586-mingw32msvc-g++
#CC=i586-mingw32msvc-gcc
#CPP=i586-mingw32msvc-gcc
#CFLAGS += -Ilibiconv
#CPPFLAGS += -Ilibiconv
#CXXFLAGS += -Ilibiconv
#LDFLAGS += -Llibiconv -liconv

DEPDIRS = utils/ utils/asm/

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
# VERSION 1.3.0  new compression options, font reorganizer, generic typeface engine
# VERSION 1.4.0  image patching support, more font reorganizing support
# VERSION 1.4.1  lots of more translation (I'm archiving it here for my convenience)
# VERSION 1.5.0  end of the compiler project; using assembler (xa65) now.
# VERSION 1.5.1  vwf8 optimizations, assembly experiments
# VERSION 1.5.2  compressed graphics support: decompressor and compressor
# VERSION 1.5.3  better graphics compressor
# VERSION 1.5.4  another archive-only version
# VERSION 1.6.0  signature support (custom compressed image on startup screen)
# VERSION 1.6.1  some remodularizing of code
# VERSION 1.6.2  fixed the vwf8 scrolling problems and some other bugs
# VERSION 1.6.3  battle item lister fixed - no vwf8 problems there now
# VERSION 1.6.4  battle tech lister almost done; dumper: partial jap ROM support
# VERSION 1.6.5  is working on an assembler
# VERSION 1.6.6  has an almost working assembler

OPTIM=-O3
#OPTIM=-O0
#OPTIM=-O0 -pg -fprofile-arcs
#LDFLAGS += -pg -fprofile-arcs

CXXFLAGS += -I.

VERSION=1.6.6
ARCHFILES=utils/xray.cc utils/xray.h \
          utils/viewer.c \
          utils/vwftest.cc \
          utils/spacefind.cc \
          utils/base62.cc \
          utils/sramdump.cc \
          utils/facegenerator.cc \
          utils/makeips.cc \
          utils/unmakeips.cc \
          utils/comprtest.cc \
          \
          ctcset.cc ctcset.hh \
          miscfun.cc miscfun.hh \
          compress.cc compress.hh \
          space.cc space.hh \
          crc32.cc crc32.h \
          hash.hh \
          wstring.cc wstring.hh \
          readin.cc wrap.cc writeout.cc \
          settings.cc settings.hh \
          rom.cc rom.hh \
          rommap.cc rommap.hh \
          strload.cc strload.hh \
          images.cc images.hh \
          fonts.cc fonts.hh \
          o65.cc o65.hh \
          o65linker.cc o65linker.hh \
          logfiles.cc logfiles.hh \
          conjugate.cc conjugate.hh \
          stringoffs.cc stringoffs.hh \
          compiler.cc compiler.hh \
          symbols.cc symbols.hh \
          tgaimage.cc tgaimage.hh \
          ctdump.cc ctdump.hh \
          ctinsert.cc ctinsert.hh \
          signature.cc \
          vwf8.cc \
          ct-vwf8.a65 \
          config.cc config.hh \
          confparser.cc confparser.hh \
          extras.cc extras.hh \
          typefaces.cc typefaces.hh \
          taipus.rb ct.code \
          progdesc.php \
          binpacker.tcc binpacker.hh \
          tristate \
          \
          utils/compiler2.cc utils/compiler2-parser.inc utils/ct.code2 \
          utils/o65test.cc utils/dumpo65.cc \
          utils/asm/assemble.cc utils/asm/insgen.cc \
          utils/asm/tristate \
          utils/asm/expr.cc utils/asm/expr.hh \
          utils/asm/insdata.cc utils/asm/insdata.hh \
          utils/asm/parse.cc utils/asm/parse.hh \
          utils/asm/object.cc utils/asm/object.hh \
          \
          README transnotes.txt Makefile.sets \
          \
          libiconv/iconv.h libiconv/libiconv.a \
          \
          dist/ct.cfg dist/ct.code \
          \
          etc/ct.cfg etc/ct.code
          

EXTRA_ARCHFILES=\
          ct.cfg ct_try.txt \
          ct-moglogo.a65 \
          FIN/ct8fn.tga \
          FIN/ct16fn.tga \
          FIN/ct8fnV.tga \
          FIN/ct.txt \
          FIN/face1.tga FIN/face2.tga FIN/face3.tga FIN/face4.tga \
          FIN/face5.tga FIN/face6.tga FIN/face7.tga FIN/face8.tga \
          FIN/elem1.tga FIN/elem2.tga FIN/elem3.tga FIN/elem4.tga \
          FIN/active1.tga FIN/active2.tga \
          FIN/titlegfx.tga FIN/epochtimes.tga FIN/eratimes.tga \
          FIN/moglogo.tga \
          FIN/README

ARCHNAME=chronotools-$(VERSION)
ARCHDIR=archives/

PROGS=\
	ctdump ctinsert \
	utils/makeips \
	utils/unmakeips \
	utils/facegenerator \
	utils/base62 \
	utils/sramdump \
	utils/viewer \
	utils/xray \
	utils/spacefind \
	utils/comp2test \
        utils/comprtest \
	utils/vwftest \
	utils/dumpo65 \
	utils/o65test \
	utils/assemble

all: $(PROGS)

ctdump: \
		ctdump.o rommap.o strload.o extras.o \
		compress.o settings.o \
		tgaimage.o symbols.o miscfun.o config.o \
		confparser.o ctcset.o wstring.o
	$(CXX) -o $@ $^ $(LDFLAGS)

ctinsert: \
		ctinsert.o miscfun.o readin.o wrap.o \
		tgaimage.o space.o writeout.o stringoffs.o \
		dictionary.o images.o compress.o \
		fonts.o typefaces.o extras.o \
		rom.o snescode.o signature.o \
		conjugate.o vwf8.o o65.o compiler.o symbols.o \
		logfiles.o settings.o \
		config.o confparser.o ctcset.o wstring.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lm

ct-vwf8.o65: ct-vwf8.a65
	./xa -o $@ $< -R -c -w

ct-moglogo.o65: ct-moglogo.a65
	./xa -o $@ $< -R -c -w

utils/makeips: utils/makeips.cc
	$(CXX) -o $@ $^
utils/unmakeips: utils/unmakeips.cc
	$(CXX) -g -O -Wall -W -pedantic -o $@ $^

utils/spacefind: utils/spacefind.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lm


utils/viewer: utils/viewer.o
	$(CC) -o $@ $^ $(LDFLAGS) -lslang

utils/sramdump: utils/sramdump.o config.o confparser.o ctcset.o wstring.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)
utils/base62: utils/base62.cc
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)
utils/vwftest: \
		utils/vwftest.o tgaimage.o \
		fonts.o typefaces.o extras.o conjugate.o \
		snescode.o symbols.o space.o logfiles.o compiler.o \
		config.o confparser.o ctcset.o wstring.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)
utils/facegenerator: \
		utils/facegenerator.o tgaimage.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)

utils/compiler2.o: utils/compiler2.cc
	$(CXX) -g -o $@ -c $< $(CXXFLAGS) -O0 -fno-default-inline
	
utils/comp2test: utils/compiler2.cc config.o confparser.o ctcset.o wstring.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)

utils/comprtest: utils/comprtest.o rommap.o compress.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)

utils/comprtest2: utils/comprtest2.o compress.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)

utils/xray: utils/xray.o compress.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS) -lggi

utils/o65test: utils/o65test.o o65.o wstring.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)

utils/dumpo65: utils/dumpo65.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)

utils/ctxtview: utils/ctxtview.o settings.o rommap.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)

utils/assemble: \
		utils/asm/assemble.o \
		utils/asm/expr.o \
		utils/asm/parse.o \
		utils/asm/insdata.o \
		utils/asm/object.o
	$(CXX) $(CXXFLAGS) -g -O -Wall -W -pedantic -o $@ $^ $(LDFLAGS)


#ct.txt: ctdump chrono-dumpee.smc
#	./ctdump chrono-dumpee.smc >ct_tmp.txt || rm -f ct_tmp.txt && false
#	mv ct_tmp.txt ct.txt

ctpatch-hdr.ips ctpatch-nohdr.ips: \
		ctinsert \
		ct.txt ct.code ct.cfg ct-vwf8.o65 \
		FIN/ct8fn.tga \
		FIN/ct16fn.tga \
		FIN/ct8fnV.tga \
		FIN/ct.txt \
		FIN/ct16fn.tga \
		FIN/ct8fn.tga \
		FIN/ct8fnV.tga \
		FIN/face1.tga FIN/face2.tga FIN/face3.tga FIN/face4.tga \
		FIN/face5.tga FIN/face6.tga FIN/face7.tga FIN/face8.tga \
		FIN/elem1.tga FIN/elem2.tga FIN/elem3.tga FIN/elem4.tga \
		FIN/active1.tga FIN/active2.tga \
		FIN/titlegfx.tga \
		FIN/moglogo.tga ct-moglogo.o65
	time ./ctinsert

chrono-patched.smc: utils/unmakeips ctpatch-hdr.ips chrono-uncompressed.smc
	$< ctpatch-hdr.ips chrono-uncompressed.smc chrono-patched.smc 2>/dev/null

snes9xtest: chrono-patched.smc FORCE
	#~/src/snes9x/bisq-1.39/Gsnes9x -stereo -alt -m 256x256[C32/32] -r 7 chrono-patched.smc
	./snes9x-debug -stereo -alt -y5 -r 7 chrono-patched.smc

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
