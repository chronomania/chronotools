#ifndef bqtctSpaceHH
#define bqtctSpaceHH

#include <map>
#include <set>
#include <vector>

using namespace std;

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

typedef set<freespacerec> freespaceset;

/* page->list */
class freespacemap : public map<unsigned, freespaceset>
{
public:
    void Report() const;
    void DumpPageMap(unsigned pagenum) const;
    
    // Returns segment-relative address (16-bit)
    unsigned Find(unsigned page, unsigned length);
    // Returns abbsolute address (24-bit)
    unsigned FindFromAnyPage(unsigned length);
    
    unsigned Size() const;
    unsigned Size(unsigned page) const;
    
    const set<unsigned> GetPageList() const;
    const freespaceset GetList(unsigned pagenum) const;
    
    // Uses segment-relative addresses (16-bit)
    void Add(unsigned page, unsigned begin, unsigned length);
    void Del(unsigned page, unsigned begin, unsigned length);
    
    // Uses segment-relative addresses (16-bit)
    void Organize(vector<freespacerec> &blocks, unsigned pagenum);

    // Uses abbsolute addresses (24-bit)
    void OrganizeToAnyPage(vector<freespacerec> &blocks);
};

#endif
