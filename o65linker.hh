#include <vector>
#include <utility>

using std::pair;
using std::vector;

#include "o65.hh"
#include "refer.hh"

class O65linker
{
public:
    O65linker();
    ~O65linker();
    
    void AddObject(const O65& object, const string& what);
    
    const vector<unsigned> GetSizeList() const;
    const vector<unsigned> GetAddrList() const;
    void PutAddrList(const vector<unsigned>& addrs);
    
    const vector<unsigned char>& GetCode(unsigned objno) const;
    const string& GetName(unsigned objno) const;
    
    void DefineSymbol(const string& name, unsigned value);
    
    void AddReference(const string& name, const ReferMethod& reference);
    const vector<pair<ReferMethod, pair<string, unsigned> > >& GetReferences() const;

    void Link();
    
    // Release the memory allocated by given obj
    void Release(unsigned objno); // no range checks

private:
    void LinkSymbol(const string& name, unsigned value);

    // No copies
    O65linker(const O65linker& );
    void operator= (const O65linker& );
    
    vector<struct Object* > objects;
    vector<pair<string, pair<unsigned, bool> > > defines;
    vector<pair<ReferMethod, pair<string, unsigned> > > referers;
    bool linked;
};
