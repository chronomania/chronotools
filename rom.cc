#include "rom.hh"
#include "config.hh"
#include "logfiles.hh"
#include "rommap.hh"

ROM::ROM(unsigned siz): length(siz)
{
    unsigned romsize = GetConf("general", "romsize");
}

ROM::~ROM()
{
    
}

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




void ROM::Write(unsigned pos, unsigned char value, const std::string& why)
{
    Data.WriteByte(pos, value);
    MarkProt(pos, 1, why);
}


void ROM::AddPatch(const vector<unsigned char> &code, unsigned addr, const string& what)
{
    if(code.empty()) return;
    
    FILE *log = GetLogFile("mem", "log_addrs");
    
    /* & 0x3FFFFF removed from here */
    const unsigned rompos = addr;
    
    for(unsigned a=0; a<code.size(); ++a)
        Write(rompos+a, code[a]);

    MarkProt(rompos, code.size(), what);
}

void ROM::SetZero(unsigned addr, unsigned len, const std::string& why)
{
    for(unsigned a=0; a < len; ++a) Write(addr + a, 0);
}
