#include <cstdio>

#include "symbols.hh"
#include "config.hh"

void Symbols::AddSym(const char *sym, char c, int targets)
{
    // 16 instead of 12 because 12 = 8+4, and 8 is already used :)
    if(targets&2)symbols2[sym]=c;
    if(targets&8)symbols8[sym]=c;
    if(targets&16)symbols16[sym]=c;
}

void Symbols::Load()
{
    int targets=0;

    // Define macro
    #define defsym(sym, c) AddSym(#sym,static_cast<char>(c),targets);
                           
    // Define bracketed macro
    #define defbsym(sym, c) defsym([sym], c)
    
    // 8pix symbols;  *xx:xxxx:Lxx
    // 16pix symbols; *xx:xxxx:Zxx
    
    targets=2+8+16;
    defbsym(end,         0x00)
    targets=2;
    defbsym(nl,          0x01)
    targets=16;
    // 0x01 seems to be garbage
    // 0x02 seems to be garbage too
    // 0x03 is delay, handled elseway
    // 0x04 seems to do nothing
    defbsym(nl,          0x05)
    defbsym(nl3,         0x06)
    defbsym(pausenl,     0x07)
    defbsym(pausenl3,    0x08)
    defbsym(cls,         0x09)
    defbsym(cls3,        0x0A)
    defbsym(pause,       0x0B)
    defbsym(pause3,      0x0C)
    defbsym(num8,        0x0D)
    defbsym(num16,       0x0E)
    defbsym(num32,       0x0F)
    
    // 0x10 is used for special meaning, but what?
    // (See $C2:58CB in assembler code)
    
    defbsym(member,      0x11)
    defbsym(tech,        0x12)
    defsym(Crono,        0x13)
    defsym(Marle,        0x14)
    defsym(Lucca,        0x15) // HUOMIO Lucca -> Lucan/Lucasta/Lucalle
    defsym(Robo,         0x16)
    defsym(Frog,         0x17)
    defsym(Ayla,         0x18)
    defsym(Magus,        0x19) // HUOMIO Magus -> Maguksen
    defbsym(crononick,   0x1A)
    defbsym(member1,     0x1B)
    defbsym(member2,     0x1C)
    defbsym(member3,     0x1D)
    defsym(Nadia,        0x1E)
    defbsym(item,        0x1F)
    defsym(Epoch,        0x20) // HUOMIO Epoch -> Epochin
    
    // 21..9F are used by dictionary
    // A0..FF are used by font
    // (or actually the point depends on get_num_chronochars())
    
    #undef defsym
    #undef defbsym
    
    /* Load user-definable some symbols */
    { const ConfParser::ElemVec& elems = GetConf("font", "def8sym").Fields();
    for(unsigned a=0; a<elems.size(); a+=2)
        AddSym(WstrToAsc(elems[a+1].SField).c_str(), (char)elems[a].IField, 8);
    }
    
    { const ConfParser::ElemVec& elems = GetConf("font", "def16sym").Fields();
    for(unsigned a=0; a<elems.size(); a+=2)
        AddSym(WstrToAsc(elems[a+1].SField).c_str(), (char)elems[a].IField, 16);
    }

    //fprintf(stderr, "%u 8pix symbols loaded\n", symbols8.size());
    //fprintf(stderr, "%u 16pix symbols loaded\n", symbols16.size());
}

const map<string, char> &Symbols::operator[] (unsigned ind) const
{
    switch(ind)
    {
        case 2: return symbols2;
        case 8: return symbols8;
        case 16:
        default: return symbols16;
    }
}

Symbols::Symbols()
{
    Load();
    fprintf(stderr, "Built symbol converter\n");
}

class Symbols Symbols;
