/* xa65 object file loader and linker for C++
 * For loading and linking 65816 object files
 * Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)
 *
 * Version 1.1.0 - Aug 18 2003, Sep 4 2003
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
    
    template<typename T>
    struct Relocdata
    {
        vector<T> Fixups;
        vector<pair<T, unsigned> > Relocs; // fixup,undef_indx
    };
    
    Relocdata<unsigned> R16;                   // addr
    Relocdata<unsigned> R16lo;                 // addr
    Relocdata<pair<unsigned,unsigned> > R16hi; // addr,lowpart
    Relocdata<pair<unsigned,unsigned> > R24seg;// addr,offspart
    Relocdata<unsigned> R24;                   // addr

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
        if(temp == (unsigned)EOF)return temp;
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
                        {
                            seg.R16lo.Relocs.push_back(make_pair(addr, undef_idx));
                            break;
                        }
                        case 0x40:
                        {
                            pair<unsigned,unsigned> tmp(addr, fgetc(fp));
                            seg.R16hi.Relocs.push_back(make_pair(tmp, undef_idx));
                            break;
                        }
                        case 0x80:
                        {
                            seg.R16.Relocs.push_back(make_pair(addr, undef_idx));
                            break;
                        }
                        case 0xA0:
                        {
                            pair<unsigned,unsigned> tmp(addr, LoadWord(fp));
                            seg.R24seg.Relocs.push_back(make_pair(tmp, undef_idx));
                            break;
                        }
                        case 0xC0:
                        {
                            seg.R24.Relocs.push_back(make_pair(addr, undef_idx));
                            break;
                        }
                        default:
                        {
                            fprintf(stderr,
                                "Error: External reloc type %02X not supported yet\n",
                                type);
                        }
                    }
                    break;
                }
                case 2: // code
                {
                    switch(type)
                    {
                        case 0x20:
                        {
                            seg.R16lo.Fixups.push_back(addr);
                            break;
                        }
                        case 0x40:
                        {
                            pair<unsigned,unsigned> tmp(addr, fgetc(fp));
                            seg.R16hi.Fixups.push_back(tmp);
                            break;
                        }
                        case 0x80:
                        {
                            seg.R16.Fixups.push_back(addr);
                            break;
                        }
                        case 0xA0:
                        {
                            pair<unsigned,unsigned> tmp(addr, LoadWord(fp));
                            seg.R24seg.Fixups.push_back(tmp);
                            break;
                        }
                        case 0xC0:
                        {
                            seg.R24.Fixups.push_back(addr);
                            break;
                        }
                        default:
                        {
                            fprintf(stderr,
                                "Error: Fixup type %02X not supported yet\n",
                                type);
                        }
                    }
                    break;
                }
                default:
                {
                    fprintf(stderr,
                        "Error: Reloc area type %02X not supported yet\n",
                            area);
                    // others are ignored
                    // FIXME: 3 would be data
                }
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

O65::O65(const O65& b): undefines(b.undefines)
{
    text = b.text ? new segment(*b.text) : NULL;
    data = b.data ? new segment(*b.data) : NULL;
    undefines = b.undefines;
}
void O65::operator= (const O65& b)
{
    if(&b == this) return;
    delete text; text = b.text ? new segment(*b.text) : NULL;
    delete data; data = b.data ? new segment(*b.data) : NULL;
    undefines = b.undefines;
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
        while(int c = fgetc(fp)) { if(c==EOF)break; varname += (char) c; }
        
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
        while(int c = fgetc(fp)) { if(c==EOF)break; varname += (char) c; }
        
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
    for(unsigned a=0; a<R16.Fixups.size(); ++a)
    {
        unsigned addr = R16.Fixups[a];
        unsigned oldvalue = space[addr] | (space[addr+1] << 8);
        unsigned newvalue = oldvalue + diff;
        
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %04X with %04X at %06X\n", oldvalue,newvalue&65535, addr);
#endif
        space[addr] = newvalue&255;
        space[addr+1] = (newvalue>>8) & 255;
    }
    for(unsigned a=0; a<R16lo.Fixups.size(); ++a)
    {
        unsigned addr = R16lo.Fixups[a];
        unsigned oldvalue = space[addr];
        unsigned newvalue = oldvalue + diff;
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %02X with %02X at %06X\n", oldvalue,newvalue&255, addr);
#endif
        space[addr] = newvalue & 255;
    }
    for(unsigned a=0; a<R16hi.Fixups.size(); ++a)
    {
        unsigned addr = R16hi.Fixups[a].first;
        unsigned oldvalue = (space[addr] << 8) | R16hi.Fixups[a].second;
        unsigned newvalue = oldvalue + diff;
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %02X with %02X at %06X\n", space[addr],(newvalue>>8)&255, addr);
#endif
        space[addr] = (newvalue>>8) & 255;
    }
    for(unsigned a=0; a<R24.Fixups.size(); ++a)
    {
        unsigned addr = R24.Fixups[a];
        unsigned oldvalue = space[addr] | (space[addr+1] << 8) | (space[addr+2] << 16);
        unsigned newvalue = oldvalue + diff;
        
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %06X with %06X at %06X\n", oldvalue,newvalue, addr);
#endif
        space[addr] = newvalue&255;
        space[addr+1] = (newvalue>>8) & 255;
        space[addr+2] = (newvalue>>16) & 255;
    }
    for(unsigned a=0; a<R24seg.Fixups.size(); ++a)
    {
        unsigned addr = R24seg.Fixups[a].first;
        unsigned oldvalue = (space[addr] << 16) | R24seg.Fixups[a].second;
        unsigned newvalue = oldvalue + diff;
        
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %02X with %02X at %06X\n", space[addr],newvalue>>16, addr);
#endif
        space[addr] = (newvalue>>16) & 255;
    }
    base = newaddress;
}

void O65::segment::LocateSym(unsigned symno, unsigned value)
{
    for(unsigned a=0; a<R16.Relocs.size(); ++a)
    {
        if(R16.Relocs[a].second != symno) continue;
        unsigned addr = R16.Relocs[a].first;
        unsigned oldvalue = space[addr] | (space[addr+1] << 8);
        unsigned newvalue = oldvalue + value;
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %04X with %04X for sym %u\n", oldvalue,newvalue&65535, symno);
#endif
        space[addr] = newvalue&255;
        space[addr+1] = (newvalue>>8) & 255;
    }
    for(unsigned a=0; a<R16lo.Relocs.size(); ++a)
    {
        if(R16lo.Relocs[a].second != symno) continue;
        unsigned addr = R16lo.Relocs[a].first;
        unsigned oldvalue = space[addr];
        unsigned newvalue = oldvalue + value;
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %02X with %02X for sym %u\n", oldvalue,newvalue&255, symno);
#endif
        space[addr] = newvalue & 255;
    }
    for(unsigned a=0; a<R16hi.Relocs.size(); ++a)
    {
        if(R16hi.Relocs[a].second != symno) continue;
        unsigned addr = R16hi.Relocs[a].first.first;
        unsigned oldvalue = (space[addr] << 8) | R16hi.Relocs[a].first.second;
        unsigned newvalue = oldvalue + value;
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %02X with %02X for sym %u\n",
            space[addr],(newvalue>>8)&255, symno);
#endif
        space[addr] = (newvalue>>8) & 255;
    }
    for(unsigned a=0; a<R24.Relocs.size(); ++a)
    {
        if(R24.Relocs[a].second != symno) continue;
        unsigned addr = R24.Relocs[a].first;
        unsigned oldvalue = space[addr] | (space[addr+1] << 8) | (space[addr+2] << 16);
        unsigned newvalue = oldvalue + value;
        
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %06X with %06X for sym %u\n", oldvalue,newvalue, symno);
#endif
        space[addr] = newvalue&255;
        space[addr+1] = (newvalue>>8) & 255;
        space[addr+2] = (newvalue>>16) & 255;
    }
    for(unsigned a=0; a<R24seg.Relocs.size(); ++a)
    {
        if(R24seg.Relocs[a].second != symno) continue;
        unsigned addr = R24seg.Relocs[a].first.first;
        unsigned oldvalue = (space[addr] << 16) | R24seg.Relocs[a].first.second;
        unsigned newvalue = oldvalue + value;
        
#if DEBUG_FIXUPS
        fprintf(stderr, "Replaced %02X with %02X for sym %u\n", space[addr],newvalue>>16, symno);
#endif
        space[addr] = (newvalue>>16) & 255;
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

bool O65::HasSym(const string& name) const
{
    return text->symbols.find(name) != text->symbols.end();
}

const vector<string> O65::GetSymbolList() const
{
    vector<string> result;
    for(map<string, unsigned>::const_iterator
        i = text->symbols.begin();
        i != text->symbols.end();
        ++i)
    {
        result.push_back(i->first);
    }
    return result;
}

const vector<string> O65::GetExternList() const
{
    vector<string> result;
    
    for(unsigned a=0; a<undefines.size(); ++a)
    {
        if(undefines[a].second.first) continue;
        result.push_back(undefines[a].first);
    }
    
    return result;
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

const string O65::GetByteAt(unsigned addr) const
{
#if 0
    for(unsigned a=0; a<text->R16lo.Relocs.size(); ++a)
    {
        if(text->R16lo.Relocs[a].first != addr) continue;
        
        unsigned symno = text->R16lo.Relocs[a].second;
        string result = undefines[symno].first;
        
        unsigned Target = undefines[symno].second.second;

        signed char diff = text->space[addr] - Target;
        if(diff)
        {
            char Buf[64]; sprintf(Buf, "%+d", diff);
            result += Buf;
        }
        return result;
    }
    for(unsigned a=0; a<text->R16lo.Fixups.size(); ++a)
    {
        if(text->R16lo.Fixups[a].first != addr) continue;
        
        unsigned symno = text->R16lo.Fixups[a].second;
        string result = undefines[symno].first;

        signed char diff = text->space[addr] - undefines[symno].second.second;
        if(diff)
        {
            char Buf[64]; sprintf(Buf, "%+d", diff);
            result += Buf;
        }
        return result;
    }
#endif
    char Buf[4]; sprintf(Buf, "$%02X", text->space[addr]);
    return Buf;
}

const string O65::GetWordAt(unsigned addr) const
{
    char Buf[6]; sprintf(Buf, "$%02X%02X", text->space[addr+1], text->space[addr]);
    return Buf;
}

const string O65::GetLongAt(unsigned addr) const
{
    char Buf[8]; sprintf(Buf, "$%02X%02X%02X", text->space[addr+2], text->space[addr+1], text->space[addr]);
    return Buf;
}
