#include "rom.hh"
#include "config.hh"
#include "logfiles.hh"

// Far call takes four bytes:
//     22 63 EA C0 = JSL $C0:$EA63

// Near jump takes three bytes:
//     4C 23 58    = JMP db:$5823    (db=register)

// Short jump takes two bytes:
//     80 2A       = JMP (IP + 2 + $2A)

void ROM::AddReference(const ReferMethod& reference, unsigned target, const string& what)
{
    unsigned rompos = reference.from_addr & 0x3FFFFF;
    
    FILE *log = GetLogFile("mem", "log_addrs");
    if(log)
    {
        fprintf(log,
                "- Add ref at $%06X: ",
                0xC00000 | rompos);
        
        fprintf(log, "(");
        if(reference.shr_by) fprintf(log, "(");
        fprintf(log, "%X", target);
        if(reference.shr_by > 0) fprintf(log, " >> %d", reference.shr_by);
        if(reference.shr_by < 0) fprintf(log, " << %d", -reference.shr_by);
        if(reference.shr_by) fprintf(log, ")");
        
        if(reference.or_mask) fprintf(log, " | %X", reference.or_mask);
        
        fprintf(log, ") & 0x");
        for(unsigned n=0; n<reference.num_bytes; ++n) fprintf(log, "FF");
        
        fprintf(log, " (%s)\n", what.c_str());
    }
    
    unsigned value = target;
    if(reference.shr_by > 0) value >>= reference.shr_by;
    if(reference.shr_by < 0) value <<= -reference.shr_by;
    
    value |= reference.or_mask;
    
    for(unsigned n=0; n<reference.num_bytes; ++n)
    {
        Write(rompos++, value & 255);
        value >>= 8;
    }
}

void ROM::AddPatch(const vector<unsigned char> &code, unsigned addr, const string& what)
{
    if(code.empty()) return;
    
    FILE *log = GetLogFile("mem", "log_addrs");
    
    if(log)
        fprintf(log, "- Add obj at $%06X (%u bytes) (%s)\n",
            0xC00000 | addr,
            code.size(),
            what.c_str());

    unsigned rompos = addr & 0x3FFFFF;
    
    for(unsigned a=0; a<code.size(); ++a)
        Write(rompos++, code[a]);
}

void ROM::AddPatch(const SNEScode &code)
{
    AddPatch(code, code.GetAddress(), code.GetName());
    
    const list<ReferMethod>& referers = code.GetReferers();
    for(list<ReferMethod>::const_iterator
        i = referers.begin();
        i != referers.end();
        ++i)
    {
        AddReference(*i, code.GetAddress(), code.GetName());
    }
}
