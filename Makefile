CC=g++
CXX=g++
CPPFLAGS=-Wall -W -pedantic -g -DVERSION=\"$(VERSION)\"
LDFLAGS=-L/usr/lib/graphics

# VERSION 1.0.3 was the first working! :D
# VERSION 1.0.4 handled fixed strings too
# VERSION 1.0.5 found item descriptions
# VERSION 1.0.6 compressed better
# VERSION 1.0.7 compressed more carefully
# VERSION 1.0.8 documented the script
VERSION=1.0.8
ARCHFILES=xray.c xray.h \
          viewer.c \
          ctcset.cc \
          ctcset.hh miscfun.cc miscfun.hh \
          ctdump.cc ctinsert.cc \
          makeips.cc unmakeips.cc \
          README \
          dictionary1 dictionary2 dictionary3 dictionary4 \
          ct_eng.txt

ARCHNAME=chronotools-$(VERSION)
ARCHDIR=archives/

PROGS=xray viewer ctdump ctinsert makeips unmakeips

all: $(PROGS)

xray: xray.o
	$(CC) -o $@ $^ $(LDFLAGS) -lggi

viewer: viewer.o
	$(CC) -o $@ $^ $(LDFLAGS) -lslang

ctdump: ctdump.o ctcset.o miscfun.o
	$(CXX) -o $@ $^

ctinsert: ctinsert.o ctcset.o miscfun.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lm

makeips: makeips.cc
	$(CXX) -o $@ $^
unmakeips: unmakeips.cc
	$(CXX) -g -O -Wall -W -pedantic -o $@ $^

ct_eng.txt: ctdump chrono-uncompressed.smc
	./ctdump >ct_eng.txt
ctpatch-hdr.ips ctpatch-nohdr.ips: ctinsert ct_eng.txt
	./ctinsert
chrono-patched.smc: unmakeips ctpatch-hdr.ips chrono-uncompressed.smc
	./unmakeips ctpatch-hdr.ips <chrono-uncompressed.smc >chrono-patched.smc

clean:
	rm -f *.o $(PROGS)
distclean: clean
	rm -f *~
realclean: distclean
	rm -f ct_eng.txt

include depfun.mak

.PHONY: all clean distclean realclean
FORCE: ;
