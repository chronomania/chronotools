#ifndef ctDump_strloadHH
#define ctDump_strloadHH

#include <vector>
#include <string>
#include <map>

using std::vector;
using std::string;
using std::map;

#include "ctcset.hh"
#include "extras.hh"

const ctstring LoadZString(unsigned offset,
                           unsigned &bytes,
                           const string& what,
                           const extrasizemap_t& extrasizes);

// Load an array of pascal style strings
const vector<ctstring> LoadPStrings(unsigned offset,
                                    unsigned count,
                                    const string& what
                                   );

// Load an array of C style strings
const vector<ctstring> LoadZStrings(unsigned offset, unsigned count,
                                    const string& what,
                                    const extrasizemap_t& extrasizes
                                   );

// Load an array of fixed length strings
const vector<ctstring> LoadFStrings(unsigned offset, unsigned len,
                                    const string& what,
                                    unsigned maxcount=0);

#endif
