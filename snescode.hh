#ifndef bqtSNEScodeHH
#define bqtSNEScodeHH

#include <vector>
#include <list>
#include "wstring.hh"
#include "refer.hh"

using namespace std;

class SNEScode: public vector<unsigned char>
{
public:
    typedef list<unsigned> AddrList;
    
    explicit SNEScode(const vector<unsigned char>&);
    SNEScode(const string& n);
    SNEScode();
    ~SNEScode();
    
    // If you don't locate the code anywhere, it won't work!
    void YourAddressIs(unsigned addr);
    
    // These are for ROM::AddPatch() to use
    unsigned GetAddress() const { return address; }
    bool HasAddress() const { return located; }
    
    void Add(const ReferMethod& reference) { referers.push_back(reference); }
    const list<ReferMethod>& GetReferers() const { return referers; }
    void SetReferers(const list<ReferMethod>& list) { referers = list; }

    void SetName(const string& n) { name=n; }
    const string& GetName() const { return name; }

private:
    typedef unsigned char byte;
    unsigned address;
    string name;
    bool located;

    list<ReferMethod> referers;
};

const class O65 LoadObject(const string& filename, const string& what);
const class O65 CreateObject(const vector<unsigned char>& code, const string& name);

#endif
