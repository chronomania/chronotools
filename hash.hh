#ifndef bqtHashHH
#define bqtHashHH

#define USE_HASH 1

#if USE_HASH

#include <string>

#include <ext/hash_map>
using namespace __gnu_cxx;

using std::basic_string;

namespace __gnu_cxx
{
  template<typename T>
  struct hash<basic_string<T> >
  {
    size_t operator() (const basic_string<T> &s) const
    {
        unsigned h=0;
        for(unsigned a=0,b=s.size(); a<b; ++a)
        {
            unsigned c = s[a];
            c=h^c;
            h^=(c*707106);
        }
        return h;
    }
  };
}

#else

#include <map>

#define hash_map std::map

#endif // USE_HASH

#endif // bqtHashHH
