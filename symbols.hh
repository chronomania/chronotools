#include <map>
#include "wstring.hh"

using std::map;

class Symbols
{
    map<wstring, char> symbols2, symbols8, symbols16;

    void AddSym(const wstring& sym, char c, int targets);
    void Load();
public:
    Symbols();
    const map<wstring, char>& operator[] (unsigned ind) const;
};

extern Symbols Symbols;
