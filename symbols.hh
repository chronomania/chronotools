#include <map>

#include "wstring.hh"
#include "ctcset.hh"

using std::map;

class Symbols
{
public:
    typedef map<wstring, ctchar> type;
    typedef map<ctchar, wstring> revtype;
private:
    type symbols2, symbols8, symbols16;
    revtype rev2, rev8, rev16;

    void AddSym(const wstring& sym, ctchar c, int targets);
    void Load();
public:
    Symbols();
    const type&    GetMap(unsigned ind) const;
    const revtype& GetRev(unsigned ind) const;
};

extern Symbols Symbols;
