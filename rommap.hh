#ifndef ctDump_rommapHH
#define ctDump_rommapHH

#include <cstdio>
#include "wstring.hh"

using std::FILE;

extern unsigned char *ROM;

void ShowProtMap();  // from dumper's view
void ShowProtMap2(); // from insertor's view

void MarkFree(size_t begin, size_t length, const std::wstring& reason = L"");
void MarkProt(size_t begin, size_t length, const std::wstring& reason = L"");
void UnProt(size_t begin, size_t length);

void LoadROM(FILE *fp);

void FindEndSpaces(void);
void ListSpaces(void);

size_t GetROMsize();

#endif
