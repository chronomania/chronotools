#include "rom.hh"
#include "config.hh"
#include "logfiles.hh"

// Far call takes four bytes:
//     22 63 EA C0 = JSL $C0:$EA63

// Near jump takes three bytes:
//     4C 23 58    = JMP db:$5823    (db=register)

// Short jump takes two bytes:
//     80 2A       = JMP (IP + 2 + $2A)

void ROM::Write(unsigned pos, unsigned char value)
{
    Data.WriteByte(pos, value);
}

unsigned ROM::FindNextBlob(unsigned where, unsigned& length) const
{
    return Data.FindNextBlob(where, length);
}

const std::vector<unsigned char> ROM::GetContent() const
{
    return Data.GetContent();
}

const std::vector<unsigned char> ROM::GetContent(unsigned a, unsigned l) const
{
    return Data.GetContent(a, l);
}




void ROM::AddPatch(const vector<unsigned char> &code, unsigned addr, const string& what)
{
    if(code.empty()) return;
    
    FILE *log = GetLogFile("mem", "log_addrs");
    
    if(log)
        fprintf(log, "$%06X-%06X: Write %6u bytes: %s\n",
            0xC00000 | addr,
            0xC00000 | (addr + code.size() - 1),
            code.size(),
            what.c_str());

    unsigned rompos = addr & 0x3FFFFF;
    
    for(unsigned a=0; a<code.size(); ++a)
        Write(rompos++, code[a]);
}
