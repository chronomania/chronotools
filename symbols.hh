#include <map>
#include <string>

using std::map;
using std::string;

class Symbols
{
    map<string, char> symbols2, symbols8, symbols16;

    void AddSym(const char *sym, char c, int targets);
    void Load();
public:
    Symbols();
    const map<string, char> &operator[] (unsigned ind) const;
};

extern Symbols Symbols;
