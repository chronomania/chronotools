#ifndef bqtctSpaceHH
#define bqtctSpaceHH

#include <map>
#include <vector>

using namespace std;

#include "rangeset.hh"

#define NOWHERE 0x10000

struct freespacerec
{
    unsigned pos;
    unsigned len;
    
    freespacerec() : pos(NOWHERE), len(0) {}
    freespacerec(unsigned l) : pos(NOWHERE), len(l) {}
    freespacerec(unsigned p,unsigned l) : pos(p), len(l) {}
    
    bool operator< (const freespacerec &b) const
    {
        if(pos != b.pos) return pos < b.pos;
        return len < b.len;
    }
};

typedef rangeset<unsigned> freespaceset;

/* page->list */
class freespacemap : public map<unsigned, freespaceset>
{
    bool quiet;
public:
    freespacemap();
    
    void Report() const;
    void DumpPageMap(unsigned pagenum) const;
    
    // Returns segment-relative address (16-bit)
    unsigned Find(unsigned page, unsigned length);
    // Returns abbsolute address (24-bit)
    unsigned FindFromAnyPage(unsigned length);
    
    unsigned Size() const;
    unsigned Size(unsigned page) const;
    unsigned GetFragmentation(unsigned page) const;
    
    const set<unsigned> GetPageList() const;
    const freespaceset& GetList(unsigned pagenum) const;
    
    // Uses segment-relative addresses (16-bit)
    void Add(unsigned page, unsigned begin, unsigned length);
    void Del(unsigned page, unsigned begin, unsigned length);
    
    // Uses absolute addresses (24-bit) (no segment wrapping!!)
    void Add(unsigned longaddr, unsigned length);
    void Del(unsigned longaddr, unsigned length);
    
    // Uses segment-relative addresses (16-bit)
    bool Organize(vector<freespacerec> &blocks, unsigned pagenum);
    // Return value: errors-flag

    // Uses absolute addresses (24-bit)
    bool OrganizeToAnyPage(vector<freespacerec> &blocks);
    // Return value: errors-flag

    // Uses segment-relative addresses (16-bit), sets page
    bool OrganizeToAnySamePage(vector<freespacerec> &blocks, unsigned &page);
    // Return value: errors-flag
    
    void Compact();
};

#endif
