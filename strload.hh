#ifndef ctDump_strloadHH
#define ctDump_strloadHH

#include "wstring.hh"
#include "ctcset.hh"
#include "extras.hh"

const ctstring LoadZString(unsigned offset,
                           unsigned &bytes,
                           const std::wstring& what,
                           const extrasizemap_t& extrasizes);

// Load an array of pascal style strings
const std::vector<ctstring>
    LoadPStrings(unsigned offset,
      unsigned count,
      const std::wstring& what
   );

// Load an array of C style strings
const std::vector<ctstring>
    LoadZStrings(unsigned offset, unsigned count,
      const std::wstring& what,
      const extrasizemap_t& extrasizes
     );

// Load an array of fixed length strings
const std::vector<ctstring>
    LoadFStrings(unsigned offset, unsigned len,
      const std::wstring& what,
      unsigned maxcount=0);

#endif
