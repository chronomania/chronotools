#CXX=g++
CXX=/usr/gcc3/bin/i586-pc-linux-gnu-g++
CC=$(CXX)
CPPFLAGS=-Wall -W -pedantic -g -DVERSION=\"$(VERSION)\"
LDFLAGS=-L/usr/lib/graphics

# VERSION 1.0.3 was the first working! :D
# VERSION 1.0.4 handled fixed strings too
# VERSION 1.0.5 found item descriptions
# VERSION 1.0.6 compressed better
# VERSION 1.0.7 compressed more carefully
# VERSION 1.0.8 documented the script
# VERSION 1.0.9 fixed "..." handling and located the font
# VERSION 1.0.10 had knowledge of character sets
VERSION=1.0.10
ARCHFILES=xray.c xray.h \
          viewer.c \
          ctcset.cc \
          ctcset.hh miscfun.cc miscfun.hh \
          wstring.cc wstring.hh \
          ctdump.cc ctinsert.cc \
          makeips.cc unmakeips.cc \
          README
EXTRA_ARCHFILES=ct_eng.txt \
          dictionary1 dictionary2 dictionary3 dictionary4 dictionary5

ARCHNAME=chronotools-$(VERSION)
ARCHDIR=archives/

PROGS=xray viewer ctdump ctinsert makeips unmakeips

all: $(PROGS)

xray: xray.o
	$(CC) -o $@ $^ $(LDFLAGS) -lggi

viewer: viewer.o
	$(CC) -o $@ $^ $(LDFLAGS) -lslang

ctdump: ctdump.o ctcset.o miscfun.o wstring.o
	$(CXX) -o $@ $^
	# -liconv

ctinsert: ctinsert.o ctcset.o miscfun.o wstring.o
	$(CXX) -o $@ $^ $(LDFLAGS) -lm
	# -liconv

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

snes9xtest: chrono-patched.smc FORCE
	~/src/snes9x/bisq-1.39/Gsnes9x -stereo -alt -m 256x256[C32/32] -r 7 chrono-patched.smc

clean: FORCE
	rm -f *.o $(PROGS)
distclean: clean
	rm -f *~
realclean: distclean
	rm -f ct_eng.txt ctpatch-hhr.ips ctpatch-nohdr.ips chrono-patched.smc

include depfun.mak

.PHONY: all clean distclean realclean snes9xtest
FORCE: ;
