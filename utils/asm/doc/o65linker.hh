#include <vector>
#include <utility>
#include "o65.hh"

class O65linker
{
public:
    O65linker();
    ~O65linker();
    
    void AddObject(const O65& object);
    
    const vector<unsigned> GetSizeList() const;
    void PutAddrList(const vector<unsigned>& addrs);
    const vector<unsigned char>& GetCode(unsigned num) const;
    
    void DefineSymbol(const string& name, unsigned value);

    void Link();

private:
    void LinkSymbol(const string& name, unsigned value);

    // No copies
    O65linker(const O65linker& );
    void operator= (const O65linker& );
    
    vector<struct Object* > objects;
    vector<pair<string, unsigned> > defines;
};
