#include "rom.hh"

// Far call takes four bytes:
//     22 63 EA C0 = JSL $C0:$EA63

// Near jump takes three bytes:
//     4C 23 58    = JMP db:$5823    (db=register)

// Short jump takes two bytes:
//     80 2A       = JMP (IP + 2 + $2A)

void ROM::AddCall(unsigned codeaddress, unsigned target)
{
    unsigned rompos = codeaddress & 0x3FFFFF;
    
    target |= 0xC00000; // Ensure we're jumping correctly
    
    fprintf(stderr, "Adding subroutine %02X:%04X call at %02X:%04X\n",
        target>>16, target&0xFFFF,
        0xC0 | (rompos>>16),
        rompos & 0xFFFF);
    
    Write(rompos++, 0x22);
    Write(rompos++, target & 255);
    Write(rompos++, (target >> 8) & 255);
    Write(rompos  , target >> 16);
}

void ROM::AddSubRoutine(unsigned target, const vector<unsigned char> &code)
{
    fprintf(stderr, "Adding %u bytes long subroutine at %02X:%04X\n",
        code.size(),
        0xC0 | (target >> 16), target & 0xFFFF);
    
    unsigned rompos = target & 0x3FFFFF;
    
    for(unsigned a=0; a<code.size(); ++a)
        Write(rompos++, code[a]);
}

void ROM::AddPatch(const SNEScode &code)
{
    AddSubRoutine(code.GetAddress(), code);
    
    const set<unsigned> &addrlist = code.GetCalls();
    for(set<unsigned>::const_iterator
        i = addrlist.begin();
        i != addrlist.end();
        ++i)
    {
        AddCall(*i, code.GetAddress());
    }
}
