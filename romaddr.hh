#ifndef bqtRomAddrHH
#define bqtRomAddrHH

unsigned char ROM2SNESpage(unsigned char page);
unsigned char SNES2ROMpage(unsigned char page);
unsigned long ROM2SNESaddr(unsigned long addr);
unsigned long SNES2ROMaddr(unsigned long addr);

bool IsSNESbased(unsigned long addr);

#endif
