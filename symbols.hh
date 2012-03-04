#include <map>

#include "wstring.hh"
#include "ctcset.hh"

using std::map;

class Symbols
{
public:
    // upper_bound, lower_bound needed; thus hash_map doesn't qualify
    typedef map<std::wstring, ctchar> type;
    // no visible benefit using hash_map here
    typedef map<ctchar, std::wstring> revtype;
private:
    type symbols2, symbols8, symbols16;
    revtype rev2, rev8, rev16;

    void AddSym(const std::wstring& sym, ctchar c, int targets);
public:
    Symbols();
    void Load();

    const type&    GetMap(unsigned ind) const;
    const revtype& GetRev(unsigned ind) const;
};

extern Symbols Symbols;
