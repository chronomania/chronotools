/* xa65 object file loader and linker for C++
 * For loading and linking 65816 object files
 * Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)
 *
 * Version 1.0 - Aug 18 2003
 */

#define DEBUG_FIXUPS 0

#include <map>

#include "o65.hh"

using std::map;
using std::make_pair;
using std::fprintf;
#ifndef stderr
using std::stderr;
#endif

class O65::segment
{
public:
    vector<unsigned char> space;

    // where it is assumed to start
    unsigned base;
    
    // absolute addresses of all symbols
    map<string, unsigned> symbols;
    
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

namespace
{
    unsigned LoadWord(FILE *fp)
    {
        unsigned temp = fgetc(fp);
        return temp | (fgetc(fp) << 8);
    }
    
    void LoadRelocations(FILE *fp, O65::segment& seg)
    {
        int addr = -1;
        for(;;)
        {
            int c = fgetc(fp);
            if(!c || c == EOF)break;
            if(c == 255) { addr += 254; continue; }
            addr += c;
            c = fgetc(fp);
            unsigned type = c & 0xE0;
            unsigned area = c & 0x07;
            
            switch(area)
            {
                case 0: // external
                {
                    unsigned undef_idx = LoadWord(fp);
                    switch(type)
                    {
                        case 0x20:
                            seg.Relocs_16lo.push_back(make_pair(addr, undef_idx));
                            break;
                        case 0x40:
                        {
                            O65::segment::Fixup16hi tmp = make_pair(addr, fgetc(fp));
                            seg.Relocs_16hi.push_back(make_pair(tmp, undef_idx));
                            break;
                        }
                        case 0x80:
                            seg.Relocs_16.push_back(make_pair(addr, undef_idx));
                            break;
                    }
                    break;
                }
                case 2: // code
                {
                    switch(type)
                    {
                        case 0x20:
                            seg.Fixups_16lo.push_back(addr);
                            break;
                        case 0x40:
                        {
                            O65::segment::Fixup16hi tmp = make_pair(addr, fgetc(fp));
                            seg.Fixups_16hi.push_back(tmp);
                            break;
                        }
                        case 0x80:
                            seg.Fixups_16.push_back(addr);
                            break;
                    }
                    break;
                }
                // others are ignored
                // FIXME: 3 would be data
            }
        }
    }
}

O65::O65(): text(NULL), data(NULL)
{
}

O65::~O65()
{
    delete text;
    delete data;
}

void O65::Load(FILE *fp)
{
    rewind(fp);
    
    /* FIXME: No validity checks here */
    // Skip header
    LoadWord(fp);LoadWord(fp);LoadWord(fp);
    
    /* FIXME: No validity checks here */
    // Skip mode
    LoadWord(fp);
    
    if(this->text) delete this->text;
    if(this->data) delete this->data;

    this->text = new segment;
    this->data = new segment;
    
    this->text->base = LoadWord(fp);
    this->text->space.resize(LoadWord(fp));

    this->data->base = LoadWord(fp);
    this->data->space.resize(LoadWord(fp));
    
    LoadWord(fp); // Skip bss_base
    LoadWord(fp); // Skip bss_len
    LoadWord(fp); // Skip zero_base
    LoadWord(fp); // Skip zero_len
    LoadWord(fp); // Skip stack_len
    
    // Skip some headers
    for(;;)
    {
        unsigned len = fgetc(fp);
        if(!len)break;
        /* FIXME: These could be valuable */
        for(unsigned a=0; a<len; ++a) fgetc(fp);
    }
    
    fread(&this->text->space[0], this->text->space.size(), 1, fp);
    fread(&this->data->space[0], this->data->space.size(), 1, fp);
    
    unsigned num_und = LoadWord(fp);
    for(unsigned a=0; a<num_und; ++a)
    {
        string varname;
        while(int c = fgetc(fp)) varname += (char) c;
        
        undefines.push_back(make_pair(varname, make_pair(false, 0)));
    }
    
    LoadRelocations(fp, *this->text);
    LoadRelocations(fp, *this->data);
    
    if(!this->data->space.empty())
    {
        fprintf(stderr, "Warning: Nonempty data segment completely ignored.\n");
    }
    
    unsigned num_ext = LoadWord(fp);
    for(unsigned a=0; a<num_ext; ++a)
    {
        string varname;
        while(int c = fgetc(fp)) varname += (char) c;   
        
        unsigned seg = fgetc(fp);
        unsigned value = LoadWord(fp);
        
        switch(seg)
        {
            case 2:
                text->symbols[varname] = value;
                break;
            case 3:
                data->symbols[varname] = value;
                break;
        }
    }
}

void O65::LocateCode(unsigned newaddress)
{
    text->Locate(newaddress);
}

void O65::LinkSym(const string& name, unsigned value)
{
    bool found = false;
    
    for(unsigned a=0; a<undefines.size(); ++a)
    {
        if(undefines[a].first != name) continue;
        
        found = true;
        
        if(undefines[a].second.first)
        {
            fprintf(stderr, "Attempt to redefine symbol '%s' as %X, old value %X\n",
                name.c_str(),
                value,
                undefines[a].second.second
                   );
        }
        text->LocateSym(a, value);
        undefines[a].second.first  = true;  // mark defined
        undefines[a].second.second = value; // as this value
    }
    
    if(!found)
        fprintf(stderr, "Attempt to define unknown symbol '%s' as %X\n",
            name.c_str(), value);
}

void O65::segment::Locate(unsigned newaddress)
{
    unsigned diff = newaddress - base;
    map<string, unsigned>::iterator i;
    for(i = symbols.begin(); i != symbols.end(); ++i)
    {
        i->second += diff;
    }
    for(unsigned a=0; a<Fixups_16.size(); ++a)
    {
        unsigned addr = Fixups_16[a];
        unsigned oldvalue = space[addr] | (space[addr+1] << 8);
        unsigned newvalue = oldvalue + diff;
        
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %04X with %04X at %06X\n", oldvalue,newvalue&65535, newaddress+addr+0xC00000);
#endif
        space[addr] = newvalue&255;
        space[addr+1] = (newvalue>>8) & 255;
    }
    for(unsigned a=0; a<Fixups_16lo.size(); ++a)
    {
        unsigned addr = Fixups_16lo[a];
        unsigned oldvalue = space[addr];
        unsigned newvalue = oldvalue + diff;
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %02X with %02X at %06X\n", oldvalue,newvalue&255, newaddress+addr+0xC00000);
#endif
        space[addr] = newvalue & 255;
    }
    for(unsigned a=0; a<Fixups_16hi.size(); ++a)
    {
        unsigned addr = Fixups_16hi[a].first;
        unsigned oldvalue = (space[addr] << 8) | Fixups_16hi[a].second;
        unsigned newvalue = oldvalue + diff;
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %02X with %02X at %06X\n", space[addr],(newvalue>>8)&255, newaddress+addr+0xC00000);
#endif
        space[addr] = (newvalue>>8) & 255;
    }
    base = newaddress;
}

void O65::segment::LocateSym(unsigned symno, unsigned value)
{
    for(unsigned a=0; a<Relocs_16.size(); ++a)
    {
        if(Relocs_16[a].second != symno) continue;
        unsigned addr = Relocs_16[a].first;
        unsigned oldvalue = space[addr] | (space[addr+1] << 8);
        unsigned newvalue = oldvalue + value;
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %04X with %04X for sym %u\n", oldvalue,newvalue&65535, symno);
#endif
        space[addr] = newvalue&255;
        space[addr+1] = (newvalue>>8) & 255;
    }
    for(unsigned a=0; a<Relocs_16lo.size(); ++a)
    {
        if(Relocs_16lo[a].second != symno) continue;
        unsigned addr = Relocs_16lo[a].first;
        unsigned oldvalue = space[addr];
        unsigned newvalue = oldvalue + value;
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %02X with %02X for sym %u\n", oldvalue,newvalue&255, symno);
#endif
        space[addr] = newvalue & 255;
    }
    for(unsigned a=0; a<Relocs_16hi.size(); ++a)
    {
        if(Relocs_16hi[a].second != symno) continue;
        unsigned addr = Relocs_16hi[a].first.first;
        unsigned oldvalue = (space[addr] << 8) | Relocs_16hi[a].first.second;
        unsigned newvalue = oldvalue + value;
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %02X with %02X for sym %u\n",
            space[addr],(newvalue>>8)&255, symno);
#endif
        space[addr] = (newvalue>>8) & 255;
    }
}

const vector<unsigned char>& O65::GetCode() const
{
    return text->space;
}

unsigned O65::GetCodeSize() const
{
    return text->space.size();
}

unsigned O65::GetSymAddress(const string& name) const
{
    return text->symbols.find(name)->second;
}

void O65::Verify() const
{
    for(unsigned a=0; a<undefines.size(); ++a)
        if(!undefines[a].second.first)
        {
            fprintf(stderr, "Symbol %s is still not defined\n",
                undefines[a].first.c_str());
        }
}
