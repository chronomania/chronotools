#include <vector>
#include <map>

using std::vector;
using std::map;

#include "ctcset.hh"

struct stringoffsdata
{
    ctstring str;
    unsigned offs;
};
class stringoffsmap: public vector<stringoffsdata>
{
public:
    // parasite -> host
    typedef map<unsigned, unsigned> neederlist_t;
    neederlist_t neederlist;
    
    void GenerateNeederList();
    void DumpNeederList() const;
};

