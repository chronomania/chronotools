#include <vector>
#include <utility>
#include <cstdio>

using std::pair;
using std::vector;
using std::FILE;

#include "o65.hh"
#include "refer.hh"

#define IPS_ADDRESS_EXTERN 0x01
#define IPS_ADDRESS_GLOBAL 0x02
#define IPS_EOF_MARKER     0x454F46

class O65linker
{
public:
    struct LinkageWish
    {
        enum type
        {
            LinkAnywhere,
            LinkHere,
            LinkInGroup,
            LinkThisPage
            
            /* FIXME: This isn't honored by OrganizeO65linker() */
        } type;
        unsigned param;
    public:
        LinkageWish(): type(LinkAnywhere), param(0) {}
        
        unsigned GetAddress() const { return type!=LinkHere ? 0 : param | 0xC00000; }
        unsigned GetPage() const { return param; }
        unsigned GetGroup() const { return param; }
        
        void SetAddress(unsigned addr) { param=addr; type=LinkHere; }
        void SetLinkageGroup(unsigned num) { param=num; type=LinkInGroup; }
        void SetLinkagePage(unsigned page) { param=page; type=LinkThisPage; }
    };
    
    O65linker();
    ~O65linker();
    
    void LoadIPSfile(FILE* fp, const string& what);
    
    void AddObject(const O65& object, const string& what, LinkageWish linkage = LinkageWish());
    void AddLump(const vector<unsigned char>&, unsigned address,
                 const string& what, const string& name="");
    void AddLump(const vector<unsigned char>&,
                 const string& what, const string& name="");
    
    unsigned CreateLinkageGroup() { return ++num_groups_used; }
    
    const vector<unsigned> GetSizeList() const;
    const vector<unsigned> GetAddrList() const;
    const vector<LinkageWish> GetLinkageList() const;
    void PutAddrList(const vector<unsigned>& addrs);
    
    const vector<unsigned char>& GetCode(unsigned objno) const;
    const string& GetName(unsigned objno) const;
    
    void DefineSymbol(const string& name, unsigned value);
    
    void AddReference(const string& name, const ReferMethod& reference);

    void Link();
    
    void SortByAddress();
    
    // Release the memory allocated by given obj
    void Release(unsigned objno); // no range checks

private:
    void LinkSymbol(const string& name, unsigned value);
    
    void FinishReference(const ReferMethod& reference, unsigned target, const string& what);

    // No copying!
    O65linker(const O65linker& );
    void operator= (const O65linker& );
    
    class SymCache *symcache;
    
    vector<struct Object* > objects;
    vector<pair<string, pair<unsigned, bool> > > defines;
    vector<pair<ReferMethod, string> > referers;
    unsigned num_groups_used;
    bool linked;
};
