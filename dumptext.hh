#include <string>

#include "wstring.hh"
#include "ctcset.hh"

const ucs4string Disp8Char(ctchar k);
const ucs4string Disp12Char(ctchar k);

void LoadDict(unsigned offs, unsigned len);

void DumpDict();

void DumpZStrings(const unsigned offs,
                  const string& what,
                  unsigned len,
                  bool dolf=true);

void DumpRZStrings(
                  const string& what,
                  unsigned len,
                  bool dolf,
                  ...);

void Dump8Strings(const unsigned offs,
                  const string& what,
                  unsigned len);

void DumpFStrings(unsigned offs,
                  const string& what,
                  unsigned len,
                  unsigned maxcount);
