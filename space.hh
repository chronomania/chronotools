#include <map>
#include <set>

using namespace std;

#define NOWHERE 0x10000


/* pos->len */
typedef pair<unsigned, unsigned> freespacerec;

typedef set<freespacerec> freespaceset;

/* page->list */
class freespacemap : public map<unsigned, freespaceset>
{
public:
    void Report() const;
    void DumpPageMap(unsigned pagenum) const;
    
    unsigned Find(unsigned page, unsigned length);
    
    unsigned Size() const;
    unsigned Size(unsigned page) const;
};
