#ifndef ctDump_rommapHH
#define ctDump_rommapHH

#include <cstdio>
#include "wstring.hh"

using std::FILE;

extern unsigned char *ROM;

void ShowProtMap();  // from dumper's view
void ShowProtMap2(); // from insertor's view

void MarkFree(unsigned begin, unsigned length, const std::wstring& reason = L"");
void MarkProt(unsigned begin, unsigned length, const std::wstring& reason = L"");
void UnProt(unsigned begin, unsigned length);

void LoadROM(FILE *fp);

void FindEndSpaces(void);
void ListSpaces(void);

unsigned long GetROMsize();

#endif
