#include "wstring.hh"
#include <string>

void DumpGFX_2bit(unsigned addr,
                  unsigned xtile, unsigned ytile,
                  const std::wstring& what,
                  const std::string& fn);

void DumpGFX_4bit(unsigned addr,
                  unsigned xtile, unsigned ytile,
                  const std::wstring& what, 
                  const std::string& fn,
                  const unsigned *palette = NULL);

void DumpGFX_Compressed_4bit(unsigned addr,
                             unsigned xtile,
                             const std::wstring& what,
                             const std::string& fn,
                             const unsigned *palette = NULL);
