#include <string>

using std::string;

void DumpGFX_2bit(unsigned addr,
                  unsigned xtile, unsigned ytile,
                  const string& what,
                  const string& fn);

void DumpGFX_4bit(unsigned addr,
                  unsigned xtile, unsigned ytile,
                  const string& what, 
                  const string& fn,
                  const unsigned *palette = NULL);

void DumpGFX_Compressed_4bit(unsigned addr,
                             unsigned xtile,
                             const string& what,
                             const string& fn,
                             const unsigned *palette = NULL);
