CC=g++
CXX=g++
CPPFLAGS=-Wall -W -pedantic -g
LDFLAGS=-L/usr/lib/graphics

VERSION=1.0.0
ARCHFILES=xray.c xray.h \
          viewer.c \
          ctcset.cc ctcset.hh miscfun.cc miscfun.hh \
          ctdump.cc ctinsert.cc
ARCHNAME=chronotools-$(VERSION)
ARCHDIR=archives/

all: xray viewer ctdump ctinsert

xray: xray.o
	$(CC) -o $@ $^ $(LDFLAGS) -lggi

viewer: viewer.o
	$(CC) -o $@ $^ $(LDFLAGS) -lslang

ctdump: ctdump.o ctcset.o miscfun.o
	$(CXX) -o $@ $^

ctinsert: ctinsert.o ctcset.o miscfun.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lm

clean:
	rm -f *.o ctdump ctinsert viewer xray
distclean: clean
	rm -f *~
realclean: distclean
	rm -f ct_eng.txt

include depfun.mak

.PHONY: all clean distclean realclean
FORCE: ;
