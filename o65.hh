#include <cstdio>
#include <utility>
#include <vector>

#include "wstring.hh"
#include "hash.hh"

using std::FILE;
using std::pair;
using std::vector;

class O65
{
public:
    struct segment
    {
        vector<unsigned char> space;

        // where it is assumed to start
        unsigned base;
        
        // absolute addresses of all symbols
        hash_map<ucs4string, unsigned> symbols;
        
        typedef unsigned Fixup16; //addr
        vector<Fixup16> Fixups_16;
        vector<pair<Fixup16, unsigned> > Relocs_16; // fixup,undef_indx
        
        typedef unsigned Fixup16lo; //addr
        vector<Fixup16lo> Fixups_16lo;
        vector<pair<Fixup16lo, unsigned> > Relocs_16lo; // fixup,undef_indx
        
        typedef pair<unsigned,unsigned> Fixup16hi; // addr,lowpart
        vector<Fixup16hi> Fixups_16hi;
        vector<pair<Fixup16hi, unsigned> > Relocs_16hi; // fixup,undef_indx
    private:
        friend class O65;
        void Locate(unsigned newaddress);
        void LocateSym(unsigned symno, unsigned newaddress);
    };
    
    void Load(FILE *fp);
    
    void LocateCode(unsigned newaddress);
    void LinkSym(const ucs4string& name, unsigned value);
    
    const vector<unsigned char>& GetCode() const { return text.space; }
    unsigned GetCodeSize() const { return text.space.size(); }
    
    unsigned GetSymAddress(const ucs4string& name) const
        { return text.symbols.find(name)->second; }
    
    void Verify() const;

private:
    // undefined symbols; sym -> defined_flag -> old value
    vector<pair<ucs4string, pair<bool, unsigned> > > undefines;
    
    segment text, data;
};
