#include <string>

#include "wstring.hh"
#include "ctcset.hh"

const std::wstring Disp8Char(ctchar k);
const std::wstring Disp12Char(ctchar k);

void LoadDict(unsigned offs, unsigned len);

void DumpDict();

void DumpZStrings(const unsigned offs,
                  const std::wstring& what,
                  unsigned len,
                  bool dolf=true);

void DumpMZStrings(const unsigned offs,
                   const std::wstring& what,
                   unsigned len,
                   bool dolf=true);

void DumpRZStrings(
                  const std::wstring& what,
                  unsigned len,
                  bool dolf,
                  ...);

void Dump8Strings(const unsigned offs,
                  const std::wstring& what,
                  unsigned len);

void DumpFStrings(unsigned offs,
                  const std::wstring& what,
                  unsigned len,
                  unsigned maxcount);
