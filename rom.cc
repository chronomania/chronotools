#include "rom.hh"
#include "config.hh"
#include "logfiles.hh"
#include "rommap.hh"

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
    
    unsigned rompos = addr & 0x3FFFFF;
    
    for(unsigned a=0; a<code.size(); ++a)
        Write(rompos+a, code[a]);

    MarkProt(rompos, code.size(), what);
}

void ROM::SetZero(unsigned addr, unsigned len, const std::string& why)
{
    for(unsigned a=0; a < len; ++a) Write(addr + a, 0);
}


void PutAscii(const string&) {}
void StartBlock(const char*, const string&, unsigned) {}
void StartBlock(const string&, const string& ) {}
void EndBlock() {}
