include Makefile.sets

DEPDIRS = argh/

OPTIM=-O3

CXXFLAGS += -I. -Iargh

# For DYNAMIC argh-linking, use:
ARGHLINK=-Largh -largh
# For STATIC argh-linking, use:
#ARGHLINK=utils/asm/argh/libargh.a

VERSION=1.2.0

ARCHFILES=COPYING Makefile.sets progdesc.php \
          assemble.cc insgen.cc \
          tristate \
          expr.cc expr.hh \
          insdata.cc insdata.hh \
          parse.cc parse.hh \
          object.cc object.hh \
          precompile.cc precompile.hh \
          warning.cc warning.hh \
          main.cc \
          \
          doc/o65.cc doc/o65.hh \
          doc/o65linker.cc doc/o65linker.hh \
          doc/dumpo65.cc doc/testi.cc \
          doc/ct-vwf8.a65 \
          doc/ct-moglogo.a65 \
          doc/bitness.a65 \
          \
          argh/argh.cc argh/argh.hh argh/argh.h argh/argh-c.inc argh/docmaker.php \
          argh/Makefile argh/depfun.mak argh/Makefile.sets argh/progdesc.php \
          argh/COPYING argh/README.html

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

ARCHNAME=snescom-$(VERSION)
ARCHDIR=archives/

PROGS=snescom
INSTALLPROGS=snescom
INSTALL=install

all: $(PROGS)

snescom: \
		assemble.o insdata.o object.o \
		expr.o parse.o precompile.o \
		main.o warning.o \
		argh/libargh.a
	$(CXX) $(CXXFLAGS) -g -o $@ $^ \
		$(LDFLAGS) -pthread $(ARGHLINK)

argh/libargh.a: FORCE
	$(MAKE) -C argh libargh.a

clean: FORCE
	rm -f *.o $(PROGS)
distclean: clean
	rm -f *~
realclean: distclean


#../assemble: FORCE
#	$(MAKE) -C ../.. utils/assemble
#
#%.o: %.cc FORCE
#	$(MAKE) -C ../.. utils/asm/$@

include depfun.mak

.PHONY: all clean distclean realclean

FORCE: ;
