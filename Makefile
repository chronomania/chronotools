include Makefile.sets

# Building for Windows:
#HOST=/usr/local/mingw32/bin/i586-mingw32msvc-
#CFLAGS += -Iwinlibs
#CPPFLAGS += -Iwinlibs
#CXXFLAGS += -Iwinlibs
#LDOPTS = -L/usr/local/mingw32/lib
#LDFLAGS += -Lwinlibs -liconv

# Building for native:
HOST=


# Which compiler to use
CXX=$(HOST)g++
CC=$(HOST)gcc
CPP=$(HOST)gcc


DEPDIRS = utils/

# VERSION 1.0.0  first archived version. dumper works.
# VERSION 1.0.1  working with recompressor, added tools.
# VERSION 1.0.2  updates to patcher
# VERSION 1.0.3  bugfixes to patcher - first working version!
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
# VERSION 1.0.15 (patch to ctfin, which is now separate)
# VERSION 1.0.16 added taipus.rb, fixed homepage urls and fixed mmap error checking.
# VERSION 1.0.17 working again; uses space better; little modularized
# VERSION 1.0.18 more of above
# VERSION 1.0.19 code organising...
# VERSION 1.0.20 binpacker changes
# VERSION 1.0.21 (patch to ctfin, which is now separate)
# VERSION 1.0.22 autowrapping support, conjugation detection code
# VERSION 1.1.0  did some assembly hacking, support for code patching
# VERSION 1.1.1  conjugating conjugating conjugating... work goes on
# VERSION 1.1.2  and so on
# VERSION 1.1.3  and so on... almost working! "case" still doesn't work.
# VERSION 1.1.4  conjugating finally works!
# VERSION 1.1.5  some bugfixes
# VERSION 1.1.6  fixed an allocation bug and optimized the code generator a bit
# VERSION 1.1.7  more asm changes
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
# VERSION 1.2.11 compression optimizations
# VERSION 1.3.0  new compression options, font reorganizer, generic typeface engine
# VERSION 1.4.0  image patching support, more font reorganizing support
# VERSION 1.4.1  (patch to ctfin, which is now separate)
# VERSION 1.5.0  end of the compiler project; using assembler (xa65) now.
# VERSION 1.5.1  vwf8 optimizations, assembly experiments
# VERSION 1.5.2  compressed graphics support: decompressor and compressor
# VERSION 1.5.3  better graphics compressor
# VERSION 1.5.4  (patch to ctfin, which is now separate)
# VERSION 1.6.0  signature support (custom compressed image on startup screen)
# VERSION 1.6.1  some remodularizing of code
# VERSION 1.6.2  fixed the vwf8 scrolling problems and some other bugs
# VERSION 1.6.3  battle item lister fixed - no vwf8 problems there now
# VERSION 1.6.4  battle tech lister almost done; dumper: partial jap ROM support
# VERSION 1.6.5  is working on an assembler
# VERSION 1.6.6  has an almost working assembler
# VERSION 1.6.7  has a complete assembler, doesn't require xa65 anymore
# VERSION 1.6.8  (patch to ctfin, which is now separate)
# VERSION 1.6.9  conjugater now partially asm; its compiler is a separate program
# VERSION 1.7.0  some error checking; windows build of the assembler
# VERSION 1.8.0  is GPL
# VERSION 1.8.1  requires separate snescom (not bundled anymore)
# VERSION 1.9.0  unified some configuration parts; added crononick-code support
# VERSION 1.9.1  improved the signature feature; added checksum and ROM name feature
# VERSION 1.9.2  has only documentation updates
# VERSION 1.9.3  includes the forgotten snescode and dictionary modules.
# VERSION 1.10.0 implemented various assembly optimization techniques
# VERSION 1.10.1 updated the docs and the conj.code generator
# VERSION 1.10.2 creates more useful information when dumping
# VERSION 1.10.3 has technological updates but broken VWF8
# VERSION 1.11.0 has technological updates and new item list code with VWF8
# VERSION 1.11.1 adds the documentation core
# VERSION 1.11.2 is a backup before anything catastrophic happens
# VERSION 1.11.3 is another backup
# VERSION 1.11.4 is another backup again
# VERSION 1.11.5 another... big structural changes
# VERSION 1.11.6 backup copy... this version has bugs
# VERSION 1.11.7 unwraps the script when dumping, if configured so
# VERSION 1.11.8 has much more documentation than before
# VERSION 1.11.9 is faster than the few recent versions
# VERSION 1.12.0 allows some strings to be moved between pages
# VERSION 1.12.1 is an upgrade to support the new o65 extension...
# VERSION 1.12.2 fixes a bug in ctdump (battle message list not dumped properly)
# VERSION 1.12.3 fixes a bug that caused utils/codegen.cc not compile

#OPTIM=-Os
# -fshort-enums
# -fpack-struct
#OPTIM=-O0
#OPTIM=-O0 -pg
#OPTIM=-O3 -pg
#LDFLAGS += -pg
OPTIM=-O3

CXXFLAGS += -I.

VERSION=1.12.3
ARCHFILES=utils/xray.cc utils/xray.h \
          utils/viewer.c \
          utils/vwftest.cc \
          utils/spacefind.cc \
          utils/base62.cc \
          utils/sramdump.cc \
          utils/facegenerator.cc \
          utils/makeips.cc \
          utils/unmakeips.cc \
          utils/fixchecksum.cc \
          utils/comprtest.cc \
          utils/rearrange.cc \
          utils/compiler.cc \
          utils/codegen.cc utils/codegen.hh \
          utils/casegen.cc utils/casegen.hh \
          utils/macrogenerator.cc utils/macrogenerator.hh \
          utils/deasm.cc utils/assemble.hh \
          utils/insdata.cc utils/insdata.hh \
          \
          autoptr \
          \
          ctcset.cc ctcset.hh \
          miscfun.hh miscfun.tcc \
          compress.cc compress.hh \
          scriptfile.cc scriptfile.hh \
          pageptrlist.cc pageptrlist.hh \
          dataarea.cc dataarea.hh \
          rangemap.hh rangemap.tcc \
          rangeset.hh rangeset.tcc \
          range.hh \
          space.cc space.hh \
          crc32.cc crc32.h \
          hash.hh \
          wstring.cc wstring.hh \
          script.cc wrap.cc writeout.cc \
          settings.cc settings.hh \
          snescode.cc \
          dictionary.cc \
          rom.cc rom.hh \
          rommap.cc rommap.hh \
          strload.cc strload.hh \
          images.cc images.hh \
          fonts.cc fonts.hh \
          o65.cc o65.hh relocdata.hh \
          o65linker.cc o65linker.hh \
          refer.hh \
          logfiles.cc logfiles.hh \
          symbols.cc symbols.hh \
          tgaimage.cc tgaimage.hh \
          ctdump.cc \
          msgdump.cc msgdump.hh \
          msginsert.cc msginsert.hh \
          ctinsert.cc ctinsert.hh \
          dumptext.cc dumptext.hh \
          dumpfont.cc dumpfont.hh \
          dumpgfx.cc dumpgfx.hh \
          config.cc config.hh \
          confparser.cc confparser.hh \
          extras.cc extras.hh \
          typefaces.cc typefaces.hh \
          binpacker.tcc binpacker.hh \
          tristate \
          \
          conjugate.cc conjugate.hh \
          \
          ct-conj.a65 \
          ct-vwf8.a65 \
          timebox.a65 \
          \
          ct-crononick.a65 \
          \
          utils/compiler2.cc utils/compiler2-parser.inc utils/ct.code2 \
          utils/deasm-disasm.cc utils/deasm-disasm.hh \
          utils/deasm-expr.hh \
          utils/o65test.cc utils/dumpo65.cc \
          \
          utils/ctxtview.cc \
          \
          README COPYING Makefile.sets \
          \
          winlibs/iconv.h winlibs/libiconv.a \
          \
          etc/ct.cfg \
          \
          DOCS/README.html progdesc.php \
          DOCS/Makefile         DOCS/docmaker.php \
          DOCS/VWF8.html        DOCS/source/VWF8.php \
          DOCS/compression.html DOCS/source/compression.php \
          DOCS/crononick.html   DOCS/source/crononick.php \
          DOCS/conjugation.html DOCS/source/conjugation.php \
          DOCS/signature.html   DOCS/source/signature.php \
          DOCS/imageformat.html DOCS/source/imageformat.php \
          DOCS/ct-moglogo.a65 \
          DOCS/ct-conj.code DOCS/ct-crononick.code \
          DOCS/ct8fnV.tga

NOGZIPARCHIVES=1

ARCHNAME=chronotools-$(VERSION)
ARCHDIR=archives/

PROGS=\
	ctdump ctinsert \
	utils/makeips \
	utils/unmakeips \
	utils/fixchecksum \
	utils/compile \
	utils/deasm \
	utils/base62 \
	utils/viewer \
	utils/xray

all: $(PROGS)

# Chrono Trigger data dumping program
ctdump: \
		ctdump.o scriptfile.o rommap.o strload.o \
		dumptext.o dumpfont.o dumpgfx.o msgdump.o \
		 tgaimage.o extras.o compress.o \
		 symbols.o logfiles.o settings.o \
		 config.o confparser.o ctcset.o wstring.o
	$(CXX) $(LDOPTS) -o $@ $^ $(LDFLAGS)

# Chrono Trigger patch generator program
ctinsert: \
		ctinsert.o script.o wrap.o msginsert.o \
		space.o writeout.o \
		dictionary.o images.o fonts.o typefaces.o \
		rom.o dataarea.o snescode.o pageptrlist.o \
		conjugate.o o65.o o65linker.o rommap.o \
		 tgaimage.o extras.o compress.o \
		 symbols.o logfiles.o settings.o \
		 config.o confparser.o ctcset.o wstring.o
	$(CXX) $(LDOPTS) -o $@ $^ $(LDFLAGS) -lm

# Conjugator/crononick code compiler
utils/compile: \
		utils/compiler.o utils/casegen.o \
		utils/codegen.o utils/macrogenerator.o \
		symbols.o \
		config.o confparser.o ctcset.o wstring.o
	$(CXX) $(LDOPTS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
utils/compiler: FORCE
	@echo Make utils/compile instead\!


# Patch generator
utils/makeips: utils/makeips.cc
	$(CXX) $(LDOPTS) -o $@ $^

# Patch applier
utils/unmakeips: utils/unmakeips.cc
	$(CXX) $(LDOPTS) $(CXXFLAGS) -o $@ $^

# ROM checksum fixer in the patch file
utils/fixchecksum: utils/fixchecksum.cc
	$(CXX) $(LDOPTS) $(CXXFLAGS) -o $@ $^


# ROM graphics viewer
utils/xray: utils/xray.o compress.o
	$(CXX) $(LDOPTS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) -lggi

# ROM text viewer
utils/viewer: utils/viewer.o
	$(CC) $(LDOPTS) -o $@ $^ $(LDFLAGS) -lslang

# A certain disassembler (not generic)
utils/deasm: utils/deasm.o utils/deasm-disasm.o utils/insdata.o rommap.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# Cursive/bold typeface font generator (obsolete)
utils/facegenerator: utils/facegenerator.o tgaimage.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# Base62 - base10 converter
utils/base62: utils/base62.cc
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# SRAM dumping program (obsolete)
utils/sramdump: utils/sramdump.o config.o confparser.o ctcset.o wstring.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)



# Script reformatter (not generic)
utils/rearrange: utils/rearrange.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# O65 tester (not generic)
utils/o65test: utils/o65test.o o65.o wstring.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# O65 dumper (not generic)
utils/dumpo65: utils/dumpo65.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# ROM script text viewer, intended to view the jap. ROM (incomplete)
utils/ctxtview: utils/ctxtview.o settings.o rommap.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# Compression tests (not generic, obsolete)
utils/comprtest: utils/comprtest.o rommap.o compress.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)
utils/comprtest2: utils/comprtest2.o compress.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# Second compiler version (not generic, incomplete, obsolete)
utils/compiler2.o: utils/compiler2.cc
	$(CXX) $(LDOPTS) -g -o $@ -c $< $(CXXFLAGS) -O0 -fno-default-inline
utils/comp2test: utils/compiler2.cc config.o confparser.o ctcset.o wstring.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# Binpacker test (obsolete)
utils/spacefind: utils/spacefind.o
	$(CXX) $(LDOPTS) -o $@ $^ $(LDFLAGS) -lm

# VWF8 test program (obsolete)
utils/vwftest: \
		utils/vwftest.o tgaimage.o \
		fonts.o typefaces.o extras.o conjugate.o o65.o settings.o \
		snescode.o symbols.o space.o logfiles.o o65linker.o \
		config.o confparser.o ctcset.o wstring.o
	$(CXX) $(LDOPTS) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)




# Generic rule to create .o65 out from .a65
%.o65: %.a65
	snescom -J -Wall -o $@ $< 

# Generic rule to create .ips out from .a65
%.ips: %.a65
	snescom -I -J -Wall -o $@ $< 


# Rules for creating ct-conj.o65
ct-conj1.a65: ct-conj.code utils/compile
	utils/compile $< $@
# ct-conj.o65 is build in a strange way.
ct-conj.o65: ct-conj1.a65 ct-conj.a65
	sed 's/#\([^a-z]\)/§\1/g;s/;.*//' < ct-conj1.a65 > .tmptmp
	sed 's§<CONJUGATER>§#include ".tmptmp"§' < ct-conj.a65 | \
		snescom -E - | sed 's/§/#/g' > .tmptmp2
	snescom -J -Wall -o $@ .tmptmp2 && rm -f .tmptmp .tmptmp2

# Rules for creating ct-crononick.o65
ct-crononick1.a65: ct-crononick.code utils/compile
	utils/compile $< $@
ct-crononick.o65: ct-crononick1.a65 ct-crononick.a65
	sed 's/#\([^a-z]\)/§\1/g;s/;.*//' < ct-crononick1.a65 > .tmptmp
	sed 's§<CONJUGATER>§#include ".tmptmp"§' < ct-crononick.a65 | \
		snescom -E - | sed 's/§/#/g' > .tmptmp2
	snescom -J -Wall -o $@ .tmptmp2 && rm -f .tmptmp .tmptmp2



DOCS/%: FORCE
	@make -s "ARCHNAME=${ARCHNAME}" -C DOCS `echo $@|sed 's|^[^/]*/||'`

#ct.txt: ctdump chrono-dumpee.smc
#	./ctdump chrono-dumpee.smc >ct_tmp.txt || rm -f ct_tmp.txt && false
#	mv ct_tmp.txt ct.txt

# Build a very small windows archive (ctdump only)
winzip: ctdump
	rm -f ctdump.exe
	ln ctdump ctdump.exe
	i586-mingw32msvc-strip ctdump.exe
	zip -9 $(ARCHNAME)-ctdump-win32.zip ctdump.exe
	rm -f ctdump.exe
	mv -f $(ARCHNAME)-ctdump-win32.zip /WWW/src/arch/

# Build a windows archive
fullzip: \
		ctdump ctinsert \
		utils/unmakeips utils/makeips utils/fixchecksum \
		utils/base62 utils/compile \
		utils/facegenerator \
		utils/o65test utils/dumpo65 \
		utils/deasm \
		etc/ct.cfg etc/ct.code \
		DOCS/README.html \
		README.TXT
	@rm -rf $(ARCHNAME)
	- mkdir $(ARCHNAME){,/utils,/etc}
	for s in $^;do ln "$$s" $(ARCHNAME)/"$$s"; done
	for dir in . utils etc; do (\
	 cd $(ARCHNAME)/$$dir; \
	 /bin/ls|while read s;do echo "$$s"|grep -qF . || test -d "$$s" || mv -v "$$s" "$$s".exe;done; \
	                           ); done
	$(HOST)strip $(ARCHNAME)/*.exe $(ARCHNAME)/*/*.exe
	- upx --overlay=strip -9 $(ARCHNAME)/*.exe $(ARCHNAME)/*/*.exe
	zip -r9 $(ARCHNAME)-win32.zip $(ARCHNAME)
	rm -rf $(ARCHNAME)
	mv -f $(ARCHNAME)-win32.zip archives/
	- ln -f archives/$(ARCHNAME)-win32.zip /WWW/src/arch/

clean: FORCE
	rm -f *.o $(PROGS) utils/*.o
distclean: clean
	rm -f *~ utils/*~
realclean: distclean
	rm -f ct_eng.txt ctpatch-hdr.ips ctpatch-nohdr.ips chrono-patched.smc

include depfun.mak

.PHONY: all clean distclean realclean snes9xtest
FORCE: ;
