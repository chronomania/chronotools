#include <vector>

using std::vector;

#include "ctcset.hh"
#include "hash.hh"

struct stringoffsdata
{
    ctstring str;
    unsigned offs;
};

class stringoffsmap: public vector<stringoffsdata>
{
public:
    // parasite -> host
    typedef hash_map<unsigned, unsigned> neederlist_t;
    neederlist_t neederlist;
    
    void GenerateNeederList();
    void DumpNeederList() const;
};

