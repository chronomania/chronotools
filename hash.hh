#ifndef bqtHashHH
#define bqtHashHH

#include <string>
#include <utility>

#include <unordered_map>
#include <unordered_set>

struct BitSwapHashFon
{
  std::size_t operator() (unsigned n) const
  {
    unsigned rot = n&31;
    return (n << rot) | (n >> (32-rot));
  }
};

namespace BisqHash
{
  template<typename T>
  struct hash
  {
    std::size_t operator() (const T& t) const { return t; }
  };

  template<typename T>
  struct hash<std::basic_string<T>>
  {
    std::size_t operator() (const std::basic_string<T> &s) const
    {
        unsigned h=0;
        for(std::size_t a=0,b=s.size(); a<b; ++a)
        {
            unsigned c = s[a];
            c=h^c;
            h^=(c*707106);
        }
        return h;
    }
  };
#if 1
#ifndef WIN32
  template<> struct hash<std::wstring>
  {
    std::size_t operator() (const std::wstring &s) const
    {
        std::size_t h=0;
        for(std::size_t a=0,b=s.size(); a<b; ++a)
        {
            std::size_t c = s[a];
            c=h^c;
            h^=(c*707106);
        }
        return h;
    }
  };
#endif
  template<> struct hash<wchar_t>
  {
    std::size_t operator() (wchar_t n) const
    {
        /* Since values of n<128 are the most common,
         * values of n<256 the second common
         * and big values of n are rare, we rotate some
         * bits to make the distribution more even.
         * Multiplying n by 33818641 (a prime near 2^32/127) scales
         * the numbers nicely to fit the whole range and keeps the
         * distribution about even.
         */
        return (n * 33818641UL);
    }
  };
#endif
  template<> struct hash<unsigned long long>
  {
    std::size_t operator() (unsigned long long n) const
    {
        return (n * 33818641UL);
    }
  };

  template<typename A,typename B> struct hash<std::pair<A,B> >
  {
     std::size_t operator() (const std::pair<A,B>& p) const
     {
         return hash<A>() (p.first)
              ^ hash<B>() (p.second);
     }
  };
}

template<typename K,typename V>
using hash_map = std::unordered_map<K,V,BisqHash::hash<K>>;

template<typename K>
using hash_set = std::unordered_set<K, BisqHash::hash<K>>;

#if 0
# include <map>
# include <set>
# define hash_map std::map
# define hash_multimap std::multimap
# define hash_set std::set
# define hash_multiset std::multiset
#endif

#endif // bqtHashHH
