#ifndef ctDump_rommapHH
#define ctDump_rommapHH

#include <cstdio>

using std::FILE;

extern unsigned char *ROM;

void ShowProtMap();

void MarkFree(unsigned begin, unsigned length);
void MarkProt(unsigned begin, unsigned length);
void UnProt(unsigned begin, unsigned length);

void LoadROM(FILE *fp);

void FindEndSpaces(void);
void ListSpaces(void);

#endif
