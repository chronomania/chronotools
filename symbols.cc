#include <cstdio>

#include "symbols.hh"
#include "config.hh"

void Symbols::AddSym(const ucs4string &sym, ctchar c, int targets)
{
    // 16 instead of 12 because 12 = 8+4, and 8 is already used.
    if(targets&2) { symbols2[sym]=c;  rev2[c]=sym; }
    if(targets&8) { symbols8[sym]=c;  rev8[c]=sym; }
    if(targets&16){ symbols16[sym]=c; rev16[c]=sym; }
}

void Symbols::Load()
{
    int targets=0;

    // Define macro with string
    #define defrsym(sym, c) AddSym(AscToWstr(sym), static_cast<ctchar>(c), targets);

    // Define macro
    #define defsym(sym, c) defrsym(#sym,c)

    // Define bracketed macro
    #define defbsym(sym, c) defsym([sym], c)
    
    // 8pix symbols;  *xx:xxxx:Lxx
    // 16pix symbols; *xx:xxxx:Zxx
    
    targets=2+8+16;
    defbsym(end,         0x00)
    targets=16;
    // 0x01: characters 256-511
    // 0x02: characters 512-767
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
    
    defbsym(member,      0x11) // Varies, user definable
    
    // 0x12 is tech/monster, handled elseway
    defsym(Crono,        0x13) // User definable
    defsym(Marle,        0x14) // User definable
    defsym(Lucca,        0x15) // User definable
    defsym(Robo,         0x16) // User definable
    defsym(Frog,         0x17) // User definable
    defsym(Ayla,         0x18) // User definable
    defsym(Magus,        0x19) // User definable
    defbsym(crononick,   0x1A) // Is Crono, user definable
    defbsym(member1,     0x1B) // Varies, user definable
    defbsym(member2,     0x1C) // Varies, user definable
    defbsym(member3,     0x1D) // Varies, user definable
    defsym(Nadia,        0x1E) 
    defbsym(item,        0x1F)
    defsym(Epoch,        0x20) // User definable
    
    // 21..9F are used by dictionary
    // A0..FF are used by font
    // (or actually the point depends on get_num_chronochars())
    
    #undef defsym
    #undef defbsym
    
    /* Load user-definable some symbols */
    { const ConfParser::ElemVec& elems = GetConf("font", "def8sym").Fields();
      for(unsigned a=0; a<elems.size(); a+=2)
      {
          const ucs4string &sym = elems[a+1];
          unsigned value     = elems[a];
          //fprintf(stderr, "Defining sym8 '%s' as 0x%02X\n", WstrToAsc(sym).c_str(),value);
          AddSym(sym, static_cast<ctchar> (value), 2+8);
      }
    }
    
    { const ConfParser::ElemVec& elems = GetConf("font", "def12sym").Fields();
      for(unsigned a=0; a<elems.size(); a+=2)
      {
          const ucs4string &sym = elems[a+1];
          unsigned value     = elems[a];
          //fprintf(stderr, "Defining sym12 '%s' as 0x%02X\n", WstrToAsc(sym).c_str(),value);
          AddSym(sym, static_cast<ctchar> (value), 16);
      }
    }

    //fprintf(stderr, "%u 8pix symbols loaded\n", symbols8.size());
    //fprintf(stderr, "%u 16pix symbols loaded\n", symbols16.size());
}

const Symbols::type& Symbols::GetMap(unsigned ind) const
{
    switch(ind)
    {
        default:
            fprintf(stderr, "Internal error: requested symbols[%u]\n", ind);
        case 2: return symbols2;
        case 8: return symbols8;
        case 16:return symbols16;
    }
}

const Symbols::revtype& Symbols::GetRev(unsigned ind) const
{
    switch(ind)
    {
        default:
            fprintf(stderr, "Internal error: requested rev-symbols[%u]\n", ind);
        case 2: return rev2;
        case 8: return rev8;
        case 16:return rev16;
    }
}

Symbols::Symbols()
{
    Load();
    fprintf(stderr, "Built symbol converter\n");
}

class Symbols Symbols;
