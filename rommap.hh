#ifndef ctDump_rommapHH
#define ctDump_rommapHH

#include <cstdio>
#include <string>

using std::FILE;
using std::string;

extern unsigned char *ROM;

void ShowProtMap();  // from dumper's view
void ShowProtMap2(); // from insertor's view

void MarkFree(unsigned begin, unsigned length, const string& reason="");
void MarkProt(unsigned begin, unsigned length, const string& reason="");
void UnProt(unsigned begin, unsigned length);

void LoadROM(FILE *fp);

void FindEndSpaces(void);
void ListSpaces(void);

#endif
